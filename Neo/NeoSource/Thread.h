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
    ThreadGUID_GILTasks,
    ThreadGUID_AssetManager,
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

    // get the name of a specific thread
    static string GetThreadNameByGUID(int guid);

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
    ~WorkerThread() { StopAndWait(); }

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

    virtual int Go();
    virtual void Terminate() { m_terminate = true; m_taskSignals.Signal(); }
    void SetName();
};


class WorkerFarmWorker : public Thread
{
    fifo<std::pair<GenericCallback, GenericCallback>> m_tasks;
    Mutex m_taskLock;
    Semaphore m_taskSignals;

public:
    WorkerFarmWorker(int guid, const string& name) : Thread(guid, name) { Start(); }
    ~WorkerFarmWorker() { StopAndWait(); }
    virtual int Go();

    // add a task to run on this worker - it will be queued and run when able
    // onComplete will be run when the worker is finished. this is needed to inform the WorkerFarm that the work is done
    void AddTask(GenericCallback task, GenericCallback onComplete)
    {
        m_taskLock.Lock();
        m_tasks.emplace_back(task, onComplete);
        m_taskLock.Release();
        m_taskSignals.Signal();
    }

    int TaskListSize() { return (int)m_tasks.size(); }
    virtual void Terminate() { m_terminate = true; m_taskSignals.Signal(); }
};

class WorkerFarm
{
    // active threads
    vector<WorkerFarmWorker*> m_workers;
    std::atomic<int> m_activeTasks;

    bool m_startWork = false;
    vector<GenericCallback> m_taskQueue;
    Mutex m_taskLock;

    GenericCallback m_onComplete = [this]() { m_activeTasks--; };

public:
    // individual Guids -> if set, each thread gets a unique guid (range is guid..guid+maxThreads)
    // this is useful if you want the profiler to have a unique row for each thread
    WorkerFarm(int guid, const string& name, int maxThreads, bool individualGuids);
    ~WorkerFarm() { KillWorkers(); }
    bool AllTasksComplete() { return m_activeTasks.load() == 0; }

    // kill and wait for all the workers to terminate
    void KillWorkers();

    // allow the workers to start work now
    // until this is called, tasks are just queued in the WorkerFarm and not dished out to workers
    void StartWork();

    // send a task to one of the workers. If StartWork hasn't been called, just queue the task locally
    // tasks are not guaranteed to finish in order, since there can be multiple workers
    void AddTask(GenericCallback task);
};


