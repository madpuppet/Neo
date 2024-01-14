#pragma once

// Custom global operator new
void* operator new(std::size_t size);
void operator delete(void* ptr) noexcept;
void* operator new[](std::size_t size);
void operator delete[](void* ptr) noexcept;
void* operator new(std::size_t size, const std::nothrow_t&) noexcept;
void operator delete(void* ptr, const std::nothrow_t&) noexcept;

#define NEO_MEMORY_TRACKING 1

enum class MemoryGroup
{
    General,
    Texture,
    Models,
    Animation,
    Props,
    AI,
    MAX
};

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
extern StackTrace gStackTrace;

class MemoryTracker
{
    bool m_enabled = false;
    std::vector<MemoryGroup> m_activeGroup;
    u64 m_totalAllocated = 0;
    u64 m_memoryGroupAllocated[(int)MemoryGroup::MAX];
    u64 m_memoryGroupAllocCount[(int)MemoryGroup::MAX];
    u64 m_debugOverhead = 0;

    struct TrackedBlock
    {
        size_t size;
        MemoryGroup group;
        void* mem;
        char* stackTrace;
        int stackTraceSize;
    };
    std::vector<TrackedBlock*> m_blocks;

public:
    MemoryTracker() { memset(m_memoryGroupAllocated, 0, sizeof(m_memoryGroupAllocated)); memset(m_memoryGroupAllocCount, 0, sizeof(m_memoryGroupAllocCount)); }
    ~MemoryTracker() { m_enabled = false; }

    void* alloc(std::size_t size);
    void free(void* mem);

    void PushGroup(MemoryGroup group) { m_activeGroup.push_back(group); }
    void PopGroup() { m_activeGroup.pop_back(); }

    void EnableTracking(bool enable) { m_enabled = enable; }
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
#define MEMGROUP(x) MemoryGroupScope(x)
#else
#define MEMGROUP(x)
#endif
