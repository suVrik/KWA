#include "render/render.h"
#include "render/vulkan/render_vulkan.h"

#include <core/error.h>

#include <debug/assert.h>

namespace kw {

Render* Render::create_instance(const RenderDescriptor& descriptor) {
    KW_ASSERT(descriptor.memory_resource != nullptr);

    switch (descriptor.api) {
    case RenderApi::VULKAN:
        return new RenderVulkan(descriptor);
    default:
        KW_ERROR(false, "Chosen render API is not supported on your platform.");
    }
}

} // namespace kw
