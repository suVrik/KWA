#include "render/render.h"
#include "render/vulkan/render_vulkan.h"

#include <core/error.h>
#include <core/math/scalar.h>

#include <array>

namespace kw {

namespace TextureFormatUtils {

struct TextureFormatProperties {
    uint8_t is_depth              : 1;
    uint8_t is_stencil            : 1;
    uint8_t is_compressed         : 1;
    uint8_t is_allowed_texture    : 1;
    uint8_t is_allowed_attachment : 1;
    uint8_t is_allowed_attribute  : 1;
    uint8_t texel_size            : 6;
    uint8_t reserved              : 4;
};

static const TextureFormatProperties TEXTURE_FORMAT_PROPERTIES[] = {
    { 0, 0, 0, 0, 0, 0, 0,  0 }, // UNKNOWN
    { 0, 0, 0, 1, 1, 1, 1,  0 }, // R8_SINT
    { 0, 0, 0, 1, 1, 1, 1,  0 }, // R8_SNORM
    { 0, 0, 0, 1, 1, 1, 1,  0 }, // R8_UINT
    { 0, 0, 0, 1, 1, 1, 1,  0 }, // R8_UNORM
    { 0, 0, 0, 1, 1, 1, 2,  0 }, // RG8_SINT
    { 0, 0, 0, 1, 1, 1, 2,  0 }, // RG8_SNORM
    { 0, 0, 0, 1, 1, 1, 2,  0 }, // RG8_UINT
    { 0, 0, 0, 1, 1, 1, 2,  0 }, // RG8_UNORM
    { 0, 0, 0, 1, 1, 1, 4,  0 }, // RGBA8_SINT
    { 0, 0, 0, 1, 1, 1, 4,  0 }, // RGBA8_SNORM
    { 0, 0, 0, 1, 1, 1, 4,  0 }, // RGBA8_UINT
    { 0, 0, 0, 1, 1, 1, 4,  0 }, // RGBA8_UNORM
    { 0, 0, 0, 1, 1, 0, 4,  0 }, // RGBA8_UNORM_SRGB
    { 0, 0, 0, 1, 1, 1, 2,  0 }, // R16_FLOAT
    { 0, 0, 0, 1, 1, 1, 2,  0 }, // R16_SINT
    { 0, 0, 0, 1, 1, 1, 2,  0 }, // R16_SNORM
    { 0, 0, 0, 1, 1, 1, 2,  0 }, // R16_UINT
    { 0, 0, 0, 1, 1, 1, 2,  0 }, // R16_UNORM
    { 0, 0, 0, 1, 1, 1, 4,  0 }, // RG16_FLOAT
    { 0, 0, 0, 1, 1, 1, 4,  0 }, // RG16_SINT
    { 0, 0, 0, 1, 1, 1, 4,  0 }, // RG16_SNORM
    { 0, 0, 0, 1, 1, 1, 4,  0 }, // RG16_UINT
    { 0, 0, 0, 1, 1, 1, 4,  0 }, // RG16_UNORM
    { 0, 0, 0, 1, 1, 1, 8,  0 }, // RGBA16_FLOAT
    { 0, 0, 0, 1, 1, 1, 8,  0 }, // RGBA16_SINT
    { 0, 0, 0, 1, 1, 1, 8,  0 }, // RGBA16_SNORM
    { 0, 0, 0, 1, 1, 1, 8,  0 }, // RGBA16_UINT
    { 0, 0, 0, 1, 1, 1, 8,  0 }, // RGBA16_UNORM
    { 0, 0, 0, 1, 1, 1, 4,  0 }, // R32_FLOAT
    { 0, 0, 0, 1, 1, 1, 4,  0 }, // R32_SINT
    { 0, 0, 0, 1, 1, 1, 4,  0 }, // R32_UINT
    { 0, 0, 0, 1, 1, 1, 8,  0 }, // RG32_FLOAT
    { 0, 0, 0, 1, 1, 1, 8,  0 }, // RG32_SINT
    { 0, 0, 0, 1, 1, 1, 8,  0 }, // RG32_UINT
    { 0, 0, 0, 0, 0, 1, 12, 0 }, // RGB32_FLOAT
    { 0, 0, 0, 0, 0, 1, 12, 0 }, // RGB32_SINT
    { 0, 0, 0, 0, 0, 1, 12, 0 }, // RGB32_UINT
    { 0, 0, 0, 1, 1, 1, 16, 0 }, // RGBA32_FLOAT
    { 0, 0, 0, 1, 1, 1, 16, 0 }, // RGBA32_SINT
    { 0, 0, 0, 1, 1, 1, 16, 0 }, // RGBA32_UINT
    { 0, 0, 0, 1, 1, 1, 4,  0 }, // BGRA8_UNORM
    { 0, 0, 0, 1, 1, 0, 4,  0 }, // BGRA8_UNORM_SRGB
    { 1, 0, 0, 1, 0, 0, 2,  0 }, // D16_UNORM
    { 1, 1, 0, 1, 0, 0, 4,  0 }, // D24_UNORM_S8_UINT
    { 1, 0, 0, 1, 0, 0, 4,  0 }, // D32_FLOAT
    { 1, 1, 0, 1, 0, 0, 8,  0 }, // D32_FLOAT_S8X24_UINT
    { 0, 0, 1, 1, 0, 0, 8,  0 }, // BC1_UNORM
    { 0, 0, 1, 1, 0, 0, 8,  0 }, // BC1_UNORM_SRGB
    { 0, 0, 1, 1, 0, 0, 16, 0 }, // BC2_UNORM
    { 0, 0, 1, 1, 0, 0, 16, 0 }, // BC2_UNORM_SRGB
    { 0, 0, 1, 1, 0, 0, 16, 0 }, // BC3_UNORM
    { 0, 0, 1, 1, 0, 0, 16, 0 }, // BC3_UNORM_SRGB
    { 0, 0, 1, 1, 0, 0, 8,  0 }, // BC4_SNORM
    { 0, 0, 1, 1, 0, 0, 8,  0 }, // BC4_UNORM
    { 0, 0, 1, 1, 0, 0, 16, 0 }, // BC5_SNORM
    { 0, 0, 1, 1, 0, 0, 16, 0 }, // BC5_UNORM
    { 0, 0, 1, 1, 0, 0, 16, 0 }, // BC6H_SF16
    { 0, 0, 1, 1, 0, 0, 16, 0 }, // BC6H_UF16
    { 0, 0, 1, 1, 0, 0, 16, 0 }, // BC7_UNORM
    { 0, 0, 1, 1, 0, 0, 16, 0 }, // BC7_UNORM_SRGB
};

static_assert(std::size(TEXTURE_FORMAT_PROPERTIES) == TEXTURE_FORMAT_COUNT);

bool is_depth(TextureFormat format) {
    return TEXTURE_FORMAT_PROPERTIES[static_cast<size_t>(format)].is_depth != 0;
}

bool is_depth_stencil(TextureFormat format) {
    return TEXTURE_FORMAT_PROPERTIES[static_cast<size_t>(format)].is_depth != 0 &&
           TEXTURE_FORMAT_PROPERTIES[static_cast<size_t>(format)].is_stencil != 0;
}

bool is_compressed(TextureFormat format) {
    return TEXTURE_FORMAT_PROPERTIES[static_cast<size_t>(format)].is_compressed != 0;
}

bool is_allowed_texture(TextureFormat format) {
    return TEXTURE_FORMAT_PROPERTIES[static_cast<size_t>(format)].is_allowed_texture != 0;
}

bool is_allowed_attachment(TextureFormat format) {
    return TEXTURE_FORMAT_PROPERTIES[static_cast<size_t>(format)].is_allowed_attachment != 0;
}

bool is_allowed_attribute(TextureFormat format) {
    return TEXTURE_FORMAT_PROPERTIES[static_cast<size_t>(format)].is_allowed_attribute != 0;
}

uint64_t get_texel_size(TextureFormat format) {
    return TEXTURE_FORMAT_PROPERTIES[static_cast<size_t>(format)].texel_size;
}

} // namespace TextureFormatUtils

Render* Render::create_instance(const RenderDescriptor& descriptor) {
    KW_ERROR(descriptor.persistent_memory_resource != nullptr, "Invalid persistent memory resource.");
    KW_ERROR(descriptor.transient_memory_resource != nullptr, "Invalid transient memory resource.");
    KW_ERROR(descriptor.staging_buffer_size >= 1024, "Staging buffer must be at least 1KB.");
    KW_ERROR(descriptor.transient_buffer_size >= 1024, "Transient buffer must be at least 1KB.");
    KW_ERROR(descriptor.buffer_allocation_size >= descriptor.buffer_block_size, "Vertex/index allocation must be larger than block.");
    KW_ERROR(descriptor.buffer_block_size > 0, "Vertex/index block must not be empty.");
    KW_ERROR(is_pow2(descriptor.buffer_allocation_size), "Vertex/index allocation must be power of 2.");
    KW_ERROR(is_pow2(descriptor.buffer_block_size), "Vertex/index block must be power of 2.");
    KW_ERROR(descriptor.texture_allocation_size >= descriptor.texture_block_size, "Texture allocation must be larger than block.");
    KW_ERROR(descriptor.texture_block_size > 0, "Texture block must not be empty.");
    KW_ERROR(is_pow2(descriptor.texture_allocation_size), "Texture allocation must be power of 2.");
    KW_ERROR(is_pow2(descriptor.texture_block_size), "Texture block must be power of 2.");

    switch (descriptor.api) {
    case RenderApi::VULKAN:
        return new (descriptor.persistent_memory_resource->allocate<RenderVulkan>()) RenderVulkan(descriptor);
    default:
        KW_ERROR(false, "Chosen render API is not supported on your platform.");
    }
}

} // namespace kw
