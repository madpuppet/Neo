#pragma once

/**************************************************************************
Thread  -  simple single job thread

just does one job and finishes
usage:
create a class that inherits from Thread, using whatever constructors & initialisers to prepare the data
implement a Go() function to do the work.  When Go() returns, the IsFinished() function will return true

MyThreadClass *pMyThread = new MyThreadClass( ... work data ... )
pMyThread->Start();  // creates the thread and calls the Go() function
if (pMyThread->IsFinished())
delete pMyThread;

***************************************************************************/

#include "neo.h"
#include <thread>
#include <condition_variable>
#include <mutex>
#include <functional>

#define NULL_THREAD thread::id()
typedef std::thread::id ThreadID;

enum ThreadGUID
{
    ThreadGUID_Main,
    ThreadGUID_AssetManager,
    ThreadGUID_GILTasks,
    ThreadGUID_Render,

    ThreadGUID_MAX
};

class Thread
{
public:
    static ThreadID CurrentThreadID();

    // each thread can register a static unique ID for identifying later what thread any code is running on
    static void RegisterThread(int guid, const string& name);

    // check what the current thread guid is.  -1 for unknown thread (thread wasn't registered)
    static int GetCurrentThreadGUID();

    // check what the current thread name is. empty() for unknow thread (thread wasn't registered)
    static string GetCurrentThreadName();

    // check current thread is on a specific thread
    static bool IsOnThread(int guid) { return GetCurrentThreadGUID() == guid; }

    Thread(int guid, const string &name);
    virtual ~Thread();

    /**
    * Starts this thread.
    **/

    void Start(bool lowPriority = true);
    void StopAndWait();

    // set the name of the thread for platforms that support this
    void SetName();

    /**
    * Request thread to terminate
    * child should take whatever steps necessary to signal the thread to exit
    **/
    virtual void Terminate() { m_terminate = true; }

    /**
    * Does not return until this thread exits.
    **/
    void WaitForThreadCompletion();
    bool IsFinished() const { return m_finished; }
    int  GetResult() const { return m_result; }

    /**
    * Abstract function -- your thread code goes in here, in a derived class.
    *
    * Return an integer value that can be retrieved by the host application.
    **/
    virtual int Go() = 0;

    // name of thread..
    const string &GetName() { return m_name; }

    // run the thread (called from Main thread routine 
    // - calls Go() internally, and then exits the thread with the return code from Go())
    void Begin();

protected:
    std::thread m_thread;

    int m_result;                   // thread quit result
    volatile bool m_finished;       // this is set if the thread has finished
    volatile bool m_terminate;

    int m_guid;                        // unique ID of thread
    string m_name;                // thread name - appears in dev studio list
};

class Semaphore
{
public:

    /**
    * @param initialCount nmbr of signals semaphore is initalised with.
    **/
    Semaphore(int initialCount = 0);
    ~Semaphore();

    /**
    * Block this thread's execution until this semaphore becomes available.
    * After Wait() returns, the caller is responsible for calling Signal() as soon
    * as possible, once it has finished using whatever resource the scSemaphore is
    * protecting.
    **/
    void Wait();

    /**
    * When finished using the resource protected by this semaphore, call Signal()
    * to let the system know that someone else may now make use of that resource.
    **/
    void Signal();

protected:
    std::mutex m_mutex;
    std::condition_variable m_variable;
    int m_count;
};

class Mutex
{
public:
    Mutex();
    ~Mutex();

    void Lock();
    void Release();

protected:
    std::mutex m_mutex;
};

class ScopedMutexLock
{
public:
    ScopedMutexLock(Mutex &mutex) : m_mutex(&mutex)
    {
        m_mutex->Lock();
    }
    ~ScopedMutexLock()
    {
        m_mutex->Release();
    }
protected:
    Mutex *m_mutex;
};

// a worker thread that executes one off tasks
class WorkerThread : public Thread
{
    Mutex m_tasks;
    Semaphore m_taskSignals;
    fifo<GenericCallback> m_taskList;

public:
    WorkerThread(int guid, const string& name) : Thread(guid, name) {}
    ~WorkerThread()
    {
        StopAndWait();
        LOG(Asset, STR("Worker Thread {} successfully shutdown", GetName()));
    }

    // @param task - typically a lambda function that you want to run on this thread. it will be called once only and then forgotten
    //               the task will need alert the system of its completion via other methods
    //               tasks are always run in order of being added
    void AddTask(GenericCallback task)
    {
        m_tasks.Lock();
        m_taskList.emplace_back(task);
        m_tasks.Release();
        m_taskSignals.Signal();
    }

    virtual int Go()
    {
        m_taskSignals.Wait();
        while (!m_terminate)
        {
            m_tasks.Lock();
            auto task = m_taskList.front();
            m_taskList.pop_front();
            m_tasks.Release();
            task();
            m_taskSignals.Wait();
        }
        LOG(Asset, STR("Worker Thread {} Go is exitting...", GetName()));
        return 0;
    }

    virtual void Terminate()
    {
        m_terminate = true;
        m_taskSignals.Signal();
    }

    void SetName();
};

class WorkerFarmWorker : public Thread
{
    GenericCallback m_task;

public:
    WorkerFarmWorker(int guid, const string& name, GenericCallback& task) : m_task(task), Thread(guid, name) {}
    ~WorkerFarmWorker() { StopAndWait(); }
    virtual int Go() { m_task(); return 0; }
};

class WorkerFarm : public Thread
{
    // active threads
    vector<Thread*> m_threads;

    // delayed tasks - max threads reached
    vector<GenericCallback> m_queuedTasks;

    // protect access to threads list
    Mutex m_threadLock;

    // this gets signalled every time a thread is complete
    // so on each single, look for a finished thread and join it/delete it
    Semaphore m_threadComplete;

    // maximum threads to have alive at any one time
    int m_maxThreads;

public:
    WorkerFarm(int guid, const string& name, int maxThreads) : m_maxThreads(maxThreads), Thread(guid, name) {}
    ~WorkerFarm()
    {
        StopAndWait();
    }

    void AddTask(GenericCallback task)
    {
        m_threadLock.Lock();
        if (m_threads.size() >= m_maxThreads)
        {
            m_queuedTasks.push_back(task);
        }
        else
        {
            auto thread = new WorkerFarmWorker(m_guid, m_name + "Worker", task);
            m_threads.push_back(thread);
            thread->Start();
        }
        m_threadLock.Release();
    }

    virtual void Terminate() override
    {
        m_terminate = true;
        m_threadComplete.Signal();
    }

    virtual int Go()
    {
        // wait for a thread to finish
        m_threadComplete.Wait();
        while (!m_terminate)
        {
            // find and remove the first finished thread
            m_threadLock.Lock();
            for (auto it = m_threads.begin(); it != m_threads.end(); it++)
            {
                if ((*it)->IsFinished())
                {
                    delete (*it);
                    m_threads.erase(it);
                    break;
                }
            }
            // one task finished, so there must be a space for a new one to start
            if (!m_queuedTasks.empty())
            {
                auto task = m_queuedTasks.back();
                m_queuedTasks.pop_back();
                auto thread = new WorkerFarmWorker(0, m_name, task);
                m_threads.push_back(thread);
            }
            m_threadLock.Release();

            // wait for next thread to finish
            m_threadComplete.Wait();
        }
        return 0;
    }
};
