#include "Neo.h"

void* operator new(std::size_t size) {
    void* mem = std::malloc(size);
    printf("Custom operator new called. Size: %d => %p\n", (int)size, mem);
    return mem;
}

// Custom global operator delete
void operator delete(void* ptr) noexcept {
    printf("Custom operator delete called: %p\n", ptr);
    std::free(ptr);
}

// Custom global operator new[] (array version)
void* operator new[](std::size_t size)
{
    void* mem = std::malloc(size);
    printf("Custom operator new[] called. Size: %d => %p\n", (int)size, mem);
    return mem;
}

// Custom global operator delete[] (array version)
void operator delete[](void* ptr) noexcept {
    printf("Custom operator delete[] called: %p\n", ptr);
    std::free(ptr);
}

// Custom global nothrow operator new
void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    void* mem = std::malloc(size);
    printf("Custom operator new (no throw) called. Size: %d => %p\n", (int)size, mem);
    return std::malloc(size);
}

// Custom global nothrow operator delete
void operator delete(void* ptr, const std::nothrow_t&) noexcept {
    printf("Custom operator delete (nothrow) called: %p\n", ptr);
    std::free(ptr);
}