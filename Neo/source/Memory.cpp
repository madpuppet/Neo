#include "Neo.h"
#include "Memory.h"
#include "FileManager.h"

bool gMemTrackEnabled = false;

StackTrace gStackTrace;
MemoryTracker gMemoryTracker;
Mutex gMemoryTrackMutex;

void* MemoryTracker::alloc(std::size_t size)
{
    void* mem = std::malloc(size);
    if (gMemTrackEnabled)
    {
        ScopedMutexLock lock(gMemoryTrackMutex);
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

        block->group = m_activeGroup.empty() ? MemoryGroup_General : m_activeGroup.back();
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

bool MemoryTracker::EnableTracking(bool enable)
{
    bool oldVal = enable;
    gMemTrackEnabled = enable;
    return oldVal;
}

void MemoryTracker::free(void* mem)
{
    std::free(mem);
    if (gMemTrackEnabled)
    {
        ScopedMutexLock lock(gMemoryTrackMutex);
        gMemTrackEnabled = false;
        auto it = m_blocks.find(mem);
        if (it != m_blocks.end())
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
        gMemTrackEnabled = true;
    }
}

static const char* groupName[] = { "General","System","Texture","Models","Animation","Props","AI","User1","User2","User3","User4" };
void MemoryTracker::Dump()
{
    auto& fm = FileManager::Instance();
    FileHandle logFile;
    bool wasEnabled = gMemTrackEnabled;
    gMemTrackEnabled = false;
    if (fm.StreamWriteBegin(logFile, "local:mem.log"))
    {
        string out = std::format("MEM DUMP: {} allocs, {} bytes, Debug Overhead {}\n", m_blocks.size(), m_totalAllocated, m_debugOverhead);
        fm.StreamWrite(logFile, (u8*)out.c_str(), (u32)out.size());

        for (int i = 0; i < (int)MemoryGroup_MAX; i++)
        {
            if (m_memoryGroupAllocCount[i] > 0)
            {
                // calc block 256/16k/256k/+
                int blocks[4] = { 0, 0, 0, 0 };
                for (auto it : m_blocks)
                {
                    auto b = it.second;
                    if ((int)b->group == i)
                    {
                        if (b->size <= 256)
                            blocks[0]++;
                        else if (b->size <= 16*1024)
                            blocks[1]++;
                        else if (b->size <= 256*1024)
                            blocks[2]++;
                        else
                            blocks[3]++;
                    }
                }

                out = std::format("-==== [{}]: {} allocs, {} bytes [{}/{}/{}/{}]====-\n", groupName[i], m_memoryGroupAllocCount[i], m_memoryGroupAllocated[i], blocks[0], blocks[1], blocks[2], blocks[3]);
                fm.StreamWrite(logFile, (u8*)out.c_str(), (u32)out.size());

                for (auto it : m_blocks)
                {
                    auto b = it.second;
                    if ((int)b->group == i)
                    {
                        out = std::format("0x{}: {} bytes\n", b->mem, b->size);
                        fm.StreamWrite(logFile, (u8*)out.c_str(), (u32)out.size());

#if NEO_STACK_TRACING
                        out = std::format("{}\n", b->stackTrace);
                        fm.StreamWrite(logFile, (u8*)out.c_str(), (u32)out.size());
#endif
                    }
                }

            }
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
    void* mem = MALLOC(size);
    return mem;
}

// Custom global operator delete
void operator delete(void* ptr) noexcept {
    FREE(ptr);
}

// Custom global operator new[] (array version)
void* operator new[](std::size_t size)
{
    void* mem = MALLOC(size);
    return mem;
}

// Custom global operator delete[] (array version)
void operator delete[](void* ptr) noexcept {
    FREE(ptr);
}

// Custom global nothrow operator new
void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    void* mem = MALLOC(size);
    return mem;
}

// Custom global nothrow operator delete
void operator delete(void* ptr, const std::nothrow_t&) noexcept {
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

Mutex m_captureMutex;
void StackTrace::Capture()
{
    m_captureMutex.Lock();
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

            // NOTE: we can save 40% of capture time using a large char buffer instead of a std::format
            //       but dynamic string is more robust - we can deal with it if speed becomes an issue
            string str;
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
    m_captureMutex.Release();
}
#endif

