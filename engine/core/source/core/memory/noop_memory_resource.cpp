#include "core/memory/noop_memory_resource.h"
#include "core/debug/assert.h"

namespace kw {

NoopMemoryResource& NoopMemoryResource::instance() {
    static NoopMemoryResource instance;
    return instance;
}

void* NoopMemoryResource::allocate(size_t size, size_t alignment) {
    KW_ASSERT(false, "NoopMemoryResource must not be used.");
    return nullptr;
}

void* NoopMemoryResource::reallocate(void* memory, size_t size, size_t alignment) {
    KW_ASSERT(false, "NoopMemoryResource must not be used.");
    return nullptr;
}

void NoopMemoryResource::deallocate(void* memory) {
    KW_ASSERT(false, "NoopMemoryResource must not be used.");
}

} // namespace kw
