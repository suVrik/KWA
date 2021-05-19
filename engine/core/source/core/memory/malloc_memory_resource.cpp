#include "core/memory/malloc_memory_resource.h"

#include <debug/assert.h>

#include <malloc.h>

namespace kw {

MallocMemoryResource& MallocMemoryResource::instance() {
    static MallocMemoryResource instance;
    return instance;
}

void* MallocMemoryResource::allocate(size_t size, size_t alignment) {
    KW_ASSERT(alignment > 0 && (alignment & (alignment - 1)) == 0, "Alignment must be power of two.");
    KW_ASSERT(size > 0, "Size must be greater than zero.");

    return _aligned_malloc(size, alignment);
}

void* MallocMemoryResource::reallocate(void* memory, size_t size, size_t alignment) {
    KW_ASSERT(alignment > 0 && (alignment & (alignment - 1)) == 0, "Alignment must be power of two.");
    KW_ASSERT(size > 0, "Size must be greater than zero.");

    return _aligned_realloc(memory, size, alignment);
}

void MallocMemoryResource::deallocate(void* memory) {
    _aligned_free(memory);
}

} // namespace kw
