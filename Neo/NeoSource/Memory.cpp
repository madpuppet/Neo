#include "Neo.h"
#include "Memory.h"
#include "Timer.h"
#include "FileManager.h"

bool gMemTrackEnabled = false;

class PeakTimer
{
public:
    struct Data
    {
        u64 max = 0;
        u64 accum = 0;
        u64 count = 0;
    };

protected:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
    Data* m_data;

public:
    PeakTimer(PeakTimer::Data *data)
    {
        m_startTime = std::chrono::high_resolution_clock::now();
        m_data = data;
    }
    ~PeakTimer()
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        u64 duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - m_startTime).count();

        if (gMemTrackEnabled)
        {
            m_data->accum += duration;
            m_data->count++;
            if (duration > m_data->max)
                m_data->max = duration;
        }
    }
};

PeakTimer::Data sAllocTimings;
PeakTimer::Data sFreeTimings;

StackTrace gStackTrace;
MemoryTracker gMemoryTracker;

void* MemoryTracker::alloc(std::size_t size)
{
    void* mem = std::malloc(size);
    if (gMemTrackEnabled)
    {
        gMemTrackEnabled = false;
        gStackTrace.Capture();
        TrackedBlock* block = new TrackedBlock();
        block->mem = mem;
        block->size = size;
        m_debugOverhead += sizeof(TrackedBlock);

#if NEO_STACK_TRACING
        block->stackTrace = (char *)std::malloc(gStackTrace.DataSize());
        block->stackTraceSize = gStackTrace.DataSize();
        memcpy((char*)block->stackTrace, gStackTrace.Data(), gStackTrace.DataSize());
        m_debugOverhead += block->stackTraceSize;
#endif

        block->group = m_activeGroup.empty() ? MemoryGroup::General : m_activeGroup.back();
        m_totalAllocated += block->size;
        m_memoryGroupAllocated[(int)block->group] += block->size;
        m_memoryGroupAllocCount[(int)block->group]++;
        m_blocks.insert(std::pair<void*,TrackedBlock*>(block->mem,block));
        gMemTrackEnabled = true;
    }
    return mem;
}

MemoryTracker::MemoryTracker()
{
    memset(m_memoryGroupAllocated, 0, sizeof(m_memoryGroupAllocated));
    memset(m_memoryGroupAllocCount, 0, sizeof(m_memoryGroupAllocCount));
}
MemoryTracker::~MemoryTracker()
{
    gMemTrackEnabled = false;
}

void MemoryTracker::EnableTracking(bool enable)
{
    gMemTrackEnabled = enable;
}

void MemoryTracker::free(void* mem)
{
    std::free(mem);
    if (gMemTrackEnabled)
    {
        gMemTrackEnabled = false;
        auto it = m_blocks.find(mem);
        if (it != m_blocks.end())
        {
//            if ((*it)->mem == mem)
            {
                auto ptr = it->second;
                m_debugOverhead -= sizeof(TrackedBlock);
#if NEO_STACK_TRACING
                m_debugOverhead -= ptr->stackTraceSize;
#endif
                m_totalAllocated -= ptr->size;
                m_memoryGroupAllocated[(int)ptr->group] -= ptr->size;
                m_memoryGroupAllocCount[(int)ptr->group]--;
                m_blocks.erase(it);
#if NEO_STACK_TRACING
                std::free(ptr->stackTrace);
#endif
                delete ptr;
            }
        }
        gMemTrackEnabled = true;
    }
}

static const char* groupName[] = { "General","Texture","Models","Animation","Props","AI" };
void MemoryTracker::Dump()
{
    auto& fm = FileManager::Instance();
    FileHandle logFile;
    bool wasEnabled = gMemTrackEnabled;
    gMemTrackEnabled = false;
    if (fm.StreamWriteBegin(logFile, "local:mem.log"))
    {
        std::string out = std::format("MEM DUMP: {} allocs, {} bytes, Alloc {} x {}/{} = {} nsecs, Free {} x {}/{} = {} nsecs, Debug Overhead {}\n", 
            m_blocks.size(), m_totalAllocated, sAllocTimings.count, sAllocTimings.accum, sAllocTimings.count, sAllocTimings.accum / sAllocTimings.count,
               sFreeTimings.count, sFreeTimings.accum, sFreeTimings.count, sFreeTimings.accum / sFreeTimings.count, m_debugOverhead);
        fm.StreamWrite(logFile, (u8*)out.c_str(), (u32)out.size());

        for (int i = 0; i < (int)MemoryGroup::MAX; i++)
        {
            if (m_memoryGroupAllocCount[i] > 0)
            {
                out = std::format("[{:10}]: {} allocs, {} bytes allocated\n", groupName[i], m_memoryGroupAllocCount[i], m_memoryGroupAllocated[i]);
                fm.StreamWrite(logFile, (u8*)out.c_str(), (u32)out.size());
            }
        }

        for (auto it : m_blocks)
        {
            auto b = it.second;
            out = std::format("\n0x{}: {} bytes [{}]", b->mem, b->size, groupName[(int)b->group]);
            fm.StreamWrite(logFile, (u8*)out.c_str(), (u32)out.size());

#if NEO_STACK_TRACING
            out = std::format("\n{}", b->stackTrace);
            fm.StreamWrite(logFile, (u8*)out.c_str(), (u32)out.size());
#endif
        }

        FileManager::Instance().StreamWriteEnd(logFile);
    }
    gMemTrackEnabled = wasEnabled;
}


#if NEO_MEMORY_TRACKING
#define MALLOC(x) gMemoryTracker.alloc(x)
#define FREE(x) gMemoryTracker.free(x)
#else
#define MALLOC(x) std::malloc(x)
#define FREE(x) std::free(x)
#endif

void* operator new(std::size_t size) {
    PeakTimer pk(&sAllocTimings);
    void* mem = MALLOC(size);
    return mem;
}

// Custom global operator delete
void operator delete(void* ptr) noexcept {
    PeakTimer pk(&sFreeTimings);
    FREE(ptr);
}

// Custom global operator new[] (array version)
void* operator new[](std::size_t size)
{
    PeakTimer pk(&sAllocTimings);
    void* mem = MALLOC(size);
    return mem;
}

// Custom global operator delete[] (array version)
void operator delete[](void* ptr) noexcept {
    PeakTimer pk(&sFreeTimings);
    FREE(ptr);
}

// Custom global nothrow operator new
void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    PeakTimer pk(&sAllocTimings);
    void* mem = MALLOC(size);
    return mem;
}

// Custom global nothrow operator delete
void operator delete(void* ptr, const std::nothrow_t&) noexcept {
    PeakTimer pk(&sFreeTimings);
    FREE(ptr);
}

#include <DbgHelp.h>

// Ensure to link against DbgHelp.lib
#if NEO_STACK_TRACING
StackTrace::StackTrace()
{
    // Initialize the symbol handler
    if (!SymInitialize(GetCurrentProcess(), nullptr, TRUE)) {
        printf("SymInitialize failed. Error code: %u\n", GetLastError());
        return;
    }
    m_initialized = true;
}
StackTrace::~StackTrace()
{
    if (m_initialized)
    {
        // Cleanup the symbol handler
        SymCleanup(GetCurrentProcess());
        m_initialized = false;
    }
}
void StackTrace::Capture()
{
    Reset();
    if (m_initialized)
    {
        // Get the context of the current thread
        CONTEXT context;
        RtlCaptureContext(&context);

        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();

        STACKFRAME64 stackFrame;
        memset(&stackFrame, 0, sizeof(stackFrame));

#ifdef _M_IX86
        DWORD machineType = IMAGE_FILE_MACHINE_I386;
        stackFrame.AddrPC.Offset = context->Eip;
        stackFrame.AddrFrame.Offset = context->Ebp;
        stackFrame.AddrStack.Offset = context->Esp;
#elif _M_X64
        DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
        stackFrame.AddrPC.Offset = context.Rip;
        stackFrame.AddrFrame.Offset = context.Rsp;
        stackFrame.AddrStack.Offset = context.Rsp;
#else
        // Add support for other architectures if needed
#endif

        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Mode = AddrModeFlat;

        while (StackWalk64(machineType,process,thread,&stackFrame,&context,nullptr,SymFunctionTableAccess64,SymGetModuleBase64,nullptr)) 
        {
            // Retrieve and print the corresponding function name using SymFromAddr
            DWORD64 displacement = 0;
            const int maxFunctionNameLength = 256;
            char buffer[sizeof(SYMBOL_INFO) + maxFunctionNameLength];
            PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = maxFunctionNameLength;

            std::string str;
            if (SymFromAddr(process, stackFrame.AddrPC.Offset, &displacement, symbol))
            {
                str = std::format("{:x}:{}\n", stackFrame.AddrPC.Offset, symbol->Name);
            }
            else
            {
                str = std::format("{:x}:???\n", stackFrame.AddrPC.Offset);
            }
            AddStr(str.c_str());
        }
    }
}
#endif

