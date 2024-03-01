#include "neo.h"
#include "thread.h"

#if defined(PLATFORM_Switch)
#include "nn/nn_Assert.h"
#include "nn/nn_Common.h"
#include "nn/nn_TimeSpan.h"
#include "nn/oe.h"
#include "nn/os.h"
#include "nn/time.h"
#include "nn/profiler.h"
#endif

Thread::Thread(int guid, const string &name)
{
    m_guid = guid;
    m_name = name;
    m_terminate = false;
    m_result = 0;
    m_finished = 0;
}

Thread::~Thread()
{
    StopAndWait();
}

void Thread::Start(bool lowPriority)
{
    m_thread = std::thread(&Thread::Begin, this);
}

void Thread::WaitForThreadCompletion()
{
    if (m_thread.joinable())
        m_thread.join();
}
 
void Thread::StopAndWait()
{
    Terminate();
    WaitForThreadCompletion();
}

void Thread::SetName()
{
    // optional per-platform code to set thread name, and core
    // if not implemented, will just use default names/any core
#if defined(PLATFORM_Switch)
    auto threadType = nn::os::GetCurrentThread();
    nn::os::SetThreadName(threadType, m_name.CStr());
    // allow threads to run on any core except the main core - we don't want the main core interrupted
    i64 coreMask = nn::os::GetThreadAvailableCoreMask() & ~1;
    nn::os::SetThreadCoreMask(threadType, nn::os::IdealCoreDontCare, coreMask);
#elif defined(PLATFORM_Windows)
#define MS_VC_EXCEPTION 0x406d1388
    typedef struct tagTHREADNAME_INFO
    {
        DWORD dwType;        // must be 0x1000
        LPCSTR szName;       // pointer to name (in same addr space)
        DWORD dwThreadID;    // thread ID (-1 caller thread)
        DWORD dwFlags;       // reserved for future use, most be zero
    } THREADNAME_INFO;
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = m_name.c_str();
    info.dwThreadID = GetCurrentThreadId();
    info.dwFlags = 0;
    __try
    {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD), (const ULONG_PTR*)&info);
    }
    __except (EXCEPTION_CONTINUE_EXECUTION)
    {
    }
#endif
}

void Thread::Begin()
{
    SetName();
    RegisterThread(m_guid, m_name);
    Go();
    m_finished = true;
}

ThreadID Thread::CurrentThreadID()
{
    return std::this_thread::get_id();
}

Semaphore::Semaphore(int initialCount)
{
    m_count = initialCount;
}

Semaphore::~Semaphore()
{
}

void Semaphore::Wait()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_count == 0)
        m_variable.wait(lock);
    --m_count;
}

void Semaphore::Signal()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    ++m_count;
    m_variable.notify_one();
}

Mutex::Mutex()
{
};

Mutex::~Mutex()
{
}

void Mutex::Lock()
{
    m_mutex.lock();
}

void Mutex::Release()
{
    m_mutex.unlock();
}


// THREAD registry code
// just a simple map of  threadId -> guid,name
// useful for if we need certain things to happen on certain threads (typical main or render thread)
struct ThreadInfo
{
    int guid;
    string name;
};
static std::map<ThreadID, ThreadInfo> s_threadRegistry;
Mutex s_threadRegistryLock;

void Thread::RegisterThread(int guid, const string& name)
{
    ScopedMutexLock lock(s_threadRegistryLock);
    auto threadID = CurrentThreadID();
    Assert(!s_threadRegistry.contains(threadID), "Register of thread ID twice!");
    s_threadRegistry[threadID] = { guid,name };
}

int Thread::GetCurrentThreadGUID()
{
    ScopedMutexLock lock(s_threadRegistryLock);
    auto threadID = Thread::CurrentThreadID();
    auto it = s_threadRegistry.find(threadID);
    return (it != s_threadRegistry.end()) ? it->second.guid : -1;
}

string Thread::GetThreadNameByGUID(int guid)
{
    for (auto& it : s_threadRegistry)
    {
        if (it.second.guid == guid)
            return it.second.name;
    }
    return "";
}

string Thread::GetCurrentThreadName()
{
    ScopedMutexLock lock(s_threadRegistryLock);

    auto it = s_threadRegistry.find(Thread::CurrentThreadID());
    return (it != s_threadRegistry.end()) ? it->second.name : "";
}

int WorkerThread::Go()
{
    m_taskSignals.Wait();
    while (!m_terminate)
    {
        m_tasks.Lock();
        auto task = std::move(m_taskList.front());
        m_taskList.pop_front();
        m_tasks.Release();
        task();
        m_taskSignals.Wait();
    }
    return 0;
}

int WorkerFarmWorker::Go()
{
    m_taskSignals.Wait();
    while (!m_terminate)
    {
        m_taskLock.Lock();
        auto task = m_tasks.front();
        m_tasks.pop_front();
        m_taskLock.Release();

        task.first();
        task.second();
        m_taskSignals.Wait();
    }
    return 0;
}

WorkerFarm::WorkerFarm(int guid, const string& name, int maxThreads, bool individualGuids)
{
    for (int i = 0; i < maxThreads; i++)
    {
        int useGuid = individualGuids ? guid + i : guid;
        m_workers.push_back(new WorkerFarmWorker(useGuid, name));
    }
}

void WorkerFarm::KillWorkers()
{
    for (auto worker : m_workers)
        delete worker;
    m_workers.clear();
}

void WorkerFarm::StartWork()
{
    // if no barriers, then just kick off all the work now
    ScopedMutexLock lock(m_taskLock);
    if (m_taskBarriers.empty())
    {
        m_startWork = true;
        for (auto& task : m_taskQueue)
        {
            auto bestWorker = FindBestWorker();
            m_activeTasks++;
            bestWorker->AddTask(task, m_onComplete);
        }
        m_taskQueue.clear();
    }

    // we have some barriers, so just kick off work up to the first barrier
    else
    {
        int barrier = m_taskBarriers.front();
        m_taskBarriers.pop_front();
        for (int i = 0; i < barrier; i++)
        {
            auto bestWorker = FindBestWorker();
            m_activeTasks++;
            bestWorker->AddTask(m_taskQueue[i], m_onComplete);
        }

        // remove the tasks we've launched
        m_taskQueue.erase(m_taskQueue.begin(), m_taskQueue.begin() + barrier);

        // update indexes of remaining barriers since we've removed tasks from the start of the list
        for (auto& b : m_taskBarriers)
            b = b - barrier;

        // launch the barrier task which will kick off the next lot once the tasks are finished
        LaunchBarrierTask();

        m_startWork = true;
    }
}

WorkerFarmWorker* WorkerFarm::FindBestWorker()
{
    // find worker with least workload
    int smallestTasks = 10000;
    WorkerFarmWorker* bestWorker = nullptr;
    for (auto worker : m_workers)
    {
        int taskSize = worker->TaskListSize();
        if (taskSize == 0)
            return worker;

        if (!bestWorker || taskSize < smallestTasks)
        {
            smallestTasks = taskSize;
            bestWorker = worker;
        }
    }
    return bestWorker;
}

void WorkerFarm::AddTask(GenericCallback task)
{
    ScopedMutexLock lock(m_taskLock);
    if (!m_startWork || m_barrierActive)
    {
        m_taskQueue.emplace_back(task);
    }
    else
    {
        // find worker with least workload
        auto bestWorker = FindBestWorker();
        m_activeTasks++;
        bestWorker->AddTask(task, m_onComplete);
    }
}

void WorkerFarm::WaitOnBarrier()
{
    LOG(WorkerFarm, "==> Waiting on barrier");

    // first we wait till all active tasks are completed
    m_activeTasks--;
    while (!AllTasksComplete());

    LOG(WorkerFarm, "==> Continuing after barrier");

    // we can start work again.. which will run all tasks up to the next barrier
    m_barrierActive = false;

    StartWork();
}

void WorkerFarm::LaunchBarrierTask()
{
    // NOTE: assume the caller has already locked the queue mutex
    m_barrierActive = true;
    auto bestWorker = FindBestWorker();
    auto nullTask = []() {};
    auto waitOnBarrier = [this]() { WaitOnBarrier(); };
    bestWorker->AddTask(nullTask, waitOnBarrier);
    m_activeTasks++;
}

void WorkerFarm::AddBarrier()
{
    ScopedMutexLock lock(m_taskLock);
    if (!m_startWork || m_barrierActive)
    {
        m_taskBarriers.push_back((int)m_taskQueue.size());
    }
    else
    {
        LaunchBarrierTask();
    }
}

