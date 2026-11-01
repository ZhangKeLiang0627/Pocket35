#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#define MALLOC_CAP_SPIRAM (1u << 10)
#define MALLOC_CAP_INTERNAL (1u << 11)
#define MALLOC_CAP_8BIT (1u << 2)

inline void* heap_caps_malloc(size_t size, uint32_t) { return malloc(size); }

inline void* heap_caps_calloc(size_t count, size_t size, uint32_t) { return calloc(count, size); }

inline void* heap_caps_aligned_alloc(size_t alignment, size_t size, uint32_t)
{
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment < sizeof(void*) ? sizeof(void*) : alignment, size) != 0)
        return nullptr;
    return ptr;
}

inline void heap_caps_free(void* ptr) { free(ptr); }
