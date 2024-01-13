#pragma once

// use our own STL allocator - needed for some platforms where we must use specific memory blocks
template <typename T>
class NeoAllocator {
public:
    using value_type = T;
    NeoAllocator() noexcept {}
    template <class U> NeoAllocator(const NeoAllocator<U>&) noexcept {}

    T* allocate(std::size_t n)
    {
        T* mem = (T*)std::malloc(sizeof(T) * n);
        printf("Alloc %d => %p\n", (int)(sizeof(T) * n), mem);
        return mem;
    }
    void deallocate(T* p, std::size_t n)
    {
        printf("Dealloc %p\n", p);
        std::free(p);
    }
    inline bool operator==(NeoAllocator const& a) { return this == &a; }
    inline bool operator!=(NeoAllocator const& a) { return !operator==(a); }
};

// Custom global operator new
void* operator new(std::size_t size);
void operator delete(void* ptr) noexcept;
void* operator new[](std::size_t size);
void operator delete[](void* ptr) noexcept;
void* operator new(std::size_t size, const std::nothrow_t&) noexcept;
void operator delete(void* ptr, const std::nothrow_t&) noexcept;

// Neo containers that we can control memory usage of - important for platforms like switch
template <typename Key, typename T>
using neomap = std::map<Key, T, std::less<Key>, NeoAllocator<std::pair<const Key, T>>>;

template <typename T>
using neovector = std::vector<T, NeoAllocator<T>>;

using neostring = std::basic_string<char, std::char_traits<char>, NeoAllocator<char>>;
