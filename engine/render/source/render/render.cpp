#include "render/render.h"
#include "render/vulkan/render_vulkan.h"

#include <core/error.h>

#include <array>

namespace kw {

namespace TextureFormatUtils {

static const bool IS_DEPTH_STENCIL[] = {
    false, // UNKNOWN
    false, // R8_SINT
    false, // R8_SNORM
    false, // R8_UINT
    false, // R8_UNORM
    false, // RG8_SINT
    false, // RG8_SNORM
    false, // RG8_UINT
    false, // RG8_UNORM
    false, // RGBA8_SINT
    false, // RGBA8_SNORM
    false, // RGBA8_UINT
    false, // RGBA8_UNORM
    false, // RGBA8_UNORM_SRGB
    false, // R16_FLOAT
    false, // R16_SINT
    false, // R16_SNORM
    false, // R16_UINT
    false, // R16_UNORM
    false, // RG16_FLOAT
    false, // RG16_SINT
    false, // RG16_SNORM
    false, // RG16_UINT
    false, // RG16_UNORM
    false, // RGBA16_FLOAT
    false, // RGBA16_SINT
    false, // RGBA16_SNORM
    false, // RGBA16_UINT
    false, // RGBA16_UNORM
    false, // R32_FLOAT
    false, // R32_SINT
    false, // R32_UINT
    false, // RG32_FLOAT
    false, // RG32_SINT
    false, // RG32_UINT
    false, // RGBA32_FLOAT
    false, // RGBA32_SINT
    false, // RGBA32_UINT
    false, // BGRA8_UNORM
    false, // BGRA8_UNORM_SRGB
    true,  // D16_UNORM
    true,  // D24_UNORM_S8_UINT
    true,  // D32_FLOAT
    true,  // D32_FLOAT_S8X24_UINT
    false, // BC1_UNORM
    false, // BC1_UNORM_SRGB
    false, // BC2_UNORM
    false, // BC2_UNORM_SRGB
    false, // BC3_UNORM
    false, // BC3_UNORM_SRGB
    false, // BC4_SNORM
    false, // BC4_UNORM
    false, // BC5_SNORM
    false, // BC5_UNORM
    false, // BC6H_SF16
    false, // BC6H_UF16
    false, // BC7_UNORM
    false, // BC7_UNORM_SRGB
};

static_assert(std::size(IS_DEPTH_STENCIL) == TEXTURE_FORMAT_COUNT);

bool is_depth_stencil(TextureFormat format) {
    return IS_DEPTH_STENCIL[static_cast<size_t>(format)];
}

static const bool IS_COMPRESSED[] = {
    false, // UNKNOWN
    false, // R8_SINT
    false, // R8_SNORM
    false, // R8_UINT
    false, // R8_UNORM
    false, // RG8_SINT
    false, // RG8_SNORM
    false, // RG8_UINT
    false, // RG8_UNORM
    false, // RGBA8_SINT
    false, // RGBA8_SNORM
    false, // RGBA8_UINT
    false, // RGBA8_UNORM
    false, // RGBA8_UNORM_SRGB
    false, // R16_FLOAT
    false, // R16_SINT
    false, // R16_SNORM
    false, // R16_UINT
    false, // R16_UNORM
    false, // RG16_FLOAT
    false, // RG16_SINT
    false, // RG16_SNORM
    false, // RG16_UINT
    false, // RG16_UNORM
    false, // RGBA16_FLOAT
    false, // RGBA16_SINT
    false, // RGBA16_SNORM
    false, // RGBA16_UINT
    false, // RGBA16_UNORM
    false, // R32_FLOAT
    false, // R32_SINT
    false, // R32_UINT
    false, // RG32_FLOAT
    false, // RG32_SINT
    false, // RG32_UINT
    false, // RGBA32_FLOAT
    false, // RGBA32_SINT
    false, // RGBA32_UINT
    false, // BGRA8_UNORM
    false, // BGRA8_UNORM_SRGB
    false, // D16_UNORM
    false, // D24_UNORM_S8_UINT
    false, // D32_FLOAT
    false, // D32_FLOAT_S8X24_UINT
    true,  // BC1_UNORM
    true,  // BC1_UNORM_SRGB
    true,  // BC2_UNORM
    true,  // BC2_UNORM_SRGB
    true,  // BC3_UNORM
    true,  // BC3_UNORM_SRGB
    true,  // BC4_SNORM
    true,  // BC4_UNORM
    true,  // BC5_SNORM
    true,  // BC5_UNORM
    true,  // BC6H_SF16
    true,  // BC6H_UF16
    true,  // BC7_UNORM
    true,  // BC7_UNORM_SRGB
};

static_assert(std::size(IS_COMPRESSED) == TEXTURE_FORMAT_COUNT);

bool is_compressed(TextureFormat format) {
    return IS_COMPRESSED[static_cast<size_t>(format)];
}

static const size_t PIXEL_SIZE[] = {
    0,  // UNKNOWN
    1,  // R8_SINT
    1,  // R8_SNORM
    1,  // R8_UINT
    1,  // R8_UNORM
    2,  // RG8_SINT
    2,  // RG8_SNORM
    2,  // RG8_UINT
    2,  // RG8_UNORM
    4,  // RGBA8_SINT
    4,  // RGBA8_SNORM
    4,  // RGBA8_UINT
    4,  // RGBA8_UNORM
    4,  // RGBA8_UNORM_SRGB
    2,  // R16_FLOAT
    2,  // R16_SINT
    2,  // R16_SNORM
    2,  // R16_UINT
    2,  // R16_UNORM
    4,  // RG16_FLOAT
    4,  // RG16_SINT
    4,  // RG16_SNORM
    4,  // RG16_UINT
    4,  // RG16_UNORM
    8,  // RGBA16_FLOAT
    8,  // RGBA16_SINT
    8,  // RGBA16_SNORM
    8,  // RGBA16_UINT
    8,  // RGBA16_UNORM
    4,  // R32_FLOAT
    4,  // R32_SINT
    4,  // R32_UINT
    8,  // RG32_FLOAT
    8,  // RG32_SINT
    8,  // RG32_UINT
    16, // RGBA32_FLOAT
    16, // RGBA32_SINT
    16, // RGBA32_UINT
    4,  // BGRA8_UNORM
    4,  // BGRA8_UNORM_SRGB
    2,  // D16_UNORM
    4,  // D24_UNORM_S8_UINT
    4,  // D32_FLOAT
    8,  // D32_FLOAT_S8X24_UINT
    0,  // BC1_UNORM
    0,  // BC1_UNORM_SRGB
    0,  // BC2_UNORM
    0,  // BC2_UNORM_SRGB
    0,  // BC3_UNORM
    0,  // BC3_UNORM_SRGB
    0,  // BC4_SNORM
    0,  // BC4_UNORM
    0,  // BC5_SNORM
    0,  // BC5_UNORM
    0,  // BC6H_SF16
    0,  // BC6H_UF16
    0,  // BC7_UNORM
    0,  // BC7_UNORM_SRGB
};

static_assert(std::size(PIXEL_SIZE) == TEXTURE_FORMAT_COUNT);

size_t get_pixel_size(TextureFormat format) {
    return PIXEL_SIZE[static_cast<size_t>(format)];
}

} // namespace TextureFormatUtils

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
