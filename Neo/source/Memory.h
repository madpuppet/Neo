#pragma once

#define NEO_MEMORY_TRACKING 1
#define NEO_STACK_TRACING 0

// Custom global operator new
//void* operator new(std::size_t size);
void operator delete(void* ptr) noexcept;
//void* operator new[](std::size_t size);
void operator delete[](void* ptr) noexcept;
//void* operator new(std::size_t size, const std::nothrow_t&) noexcept;
void operator delete(void* ptr, const std::nothrow_t&) noexcept;

//<REFLECT>
enum MemoryGroup
{
    MemoryGroup_General,
    MemoryGroup_System,
    MemoryGroup_Texture,
    MemoryGroup_Models,
    MemoryGroup_Animation,
    MemoryGroup_Props,
    MemoryGroup_AI,

    MemoryGroup_User1,
    MemoryGroup_User2,
    MemoryGroup_User3,
    MemoryGroup_User4,

    MemoryGroup_MAX
};

#if NEO_STACK_TRACING
class StackTrace
{
    static const int MaxSize = 65536;
    char m_buffer[MaxSize];
    int m_writePos = 0;
    bool m_initialized = false;

    void Reset() { m_writePos = 0; m_buffer[0] = 0; }
    void AddStr(const char* str)
    {
        while (m_writePos < MaxSize - 1 && *str)
        {
            m_buffer[m_writePos++] = *str++;
        }
        m_buffer[m_writePos] = 0;
    }

public:
    StackTrace();
    ~StackTrace();
    void Capture();
    const char* Data() { return m_buffer; }
    int DataSize() { return m_writePos+1; }
};
#else
class StackTrace
{
public:
    void Capture() {}
    const char* Data() { return nullptr; }
    int DataSize() { return 0; }
};
#endif
extern StackTrace gStackTrace;

// Memory Tracker records each allocation
// if NEO_MEMORY_TRACKING then tracking is stored
// if NEO_STACK_TRACING then a stack trace is stored with each memory block
class MemoryTracker
{
    vector<MemoryGroup> m_activeGroup;
    u64 m_totalAllocated = 0;
    u64 m_memoryGroupAllocated[(int)MemoryGroup_MAX];
    u64 m_memoryGroupAllocCount[(int)MemoryGroup_MAX];
    u64 m_debugOverhead = 0;

    struct TrackedBlock
    {
        size_t size;
        MemoryGroup group;
        void* mem;
#if NEO_STACK_TRACING
        char* stackTrace;
        int stackTraceSize;
#endif
    };
    hashtable<void *, TrackedBlock*> m_blocks;

public:
    MemoryTracker();
    ~MemoryTracker();
    void* alloc(std::size_t size);
    void free(void* mem);
    void PushGroup(MemoryGroup group) { m_activeGroup.push_back(group); }
    void PopGroup() { m_activeGroup.pop_back(); }
    bool EnableTracking(bool enable);
    void Dump();
};
extern MemoryTracker gMemoryTracker;

// use the MEMGROUP macro to mark the memory group of all allocations in the current c++ scope
#if NEO_MEMORY_TRACKING
class MemoryGroupScope
{
public:
    MemoryGroupScope(MemoryGroup group) { gMemoryTracker.PushGroup(group); }
    ~MemoryGroupScope() { gMemoryTracker.PopGroup(); }
};
class MemoryTrackDisableScope
{
public:
    bool oldState;
    MemoryTrackDisableScope() { oldState = gMemoryTracker.EnableTracking(false); }
    ~MemoryTrackDisableScope() { gMemoryTracker.EnableTracking(oldState); }
};

#define MEMGROUP(x) MemoryGroupScope __scopeMGS(MemoryGroup_##x)
#define NOMEMTRACK() MemoryTrackDisableScope __scopeMTDS
#else
#define MEMGROUP(x)
#define NOMEMTRACK()
#endif
