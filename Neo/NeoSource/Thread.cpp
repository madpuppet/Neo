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

void ThreadFunc(void *threadPtr)
{
    auto thread = (Thread *)threadPtr;
    thread->Begin();
}

Thread::Thread(const String &name)
{
    m_name = name;
    m_terminate = false;
    m_result = 0;
    m_finished = 0;
}

Thread::~Thread()
{
}

void Thread::Start(bool lowPriority)
{
    m_thread = std::thread(ThreadFunc, this);
}

void Thread::WaitForThreadCompletion()
{
    if (!m_finished)
    {
        m_thread.join();
    }
}

void Thread::Stop()
{
    Terminate();
    WaitForThreadCompletion();
}

void Thread::Begin()
{
    // optional per-platform code to set thread name, and core
    // if not implemented, will just use default names/any core
#if defined(_NSWITCH)
    auto threadType = nn::os::GetCurrentThread();
    nn::os::SetThreadName(threadType, m_name.CStr());
    // allow threads to run on any core except the main core - we don't want the main core interrupted
    i64 coreMask = nn::os::GetThreadAvailableCoreMask() & ~1;
    nn::os::SetThreadCoreMask(threadType, nn::os::IdealCoreDontCare, coreMask);
#elif defined(_WINDOWS)
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
    info.szName = m_name.CStr();
    info.dwThreadID = GetCurrentThreadId();
    info.dwFlags = 0;
    __try
    {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD), (const ULONG_PTR *)&info);
    }
    __except (EXCEPTION_CONTINUE_EXECUTION)
    {
    }
#endif

    Go();
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

