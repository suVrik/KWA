#pragma once

namespace kw {

class MemoryResourceLinear;

enum class RenderApi {
    VULKAN,
    DIRECTX,
};

struct RenderDescriptor {
    RenderApi api;

    MemoryResourceLinear* memory_resource;

    bool is_validation_enabled;
    bool is_debug_names_enabled;
};

enum class TextureFormat {
    UNKNOWN,
    R8_SINT,
    R8_SNORM,
    R8_UINT,
    R8_UNORM,
    RG8_SINT,
    RG8_SNORM,
    RG8_UINT,
    RG8_UNORM,
    RGBA8_SINT,
    RGBA8_SNORM,
    RGBA8_UINT,
    RGBA8_UNORM,
    RGBA8_UNORM_SRGB,
    R16_FLOAT,
    R16_SINT,
    R16_SNORM,
    R16_UINT,
    R16_UNORM,
    RG16_FLOAT,
    RG16_SINT,
    RG16_SNORM,
    RG16_UINT,
    RG16_UNORM,
    RGBA16_FLOAT,
    RGBA16_SINT,
    RGBA16_SNORM,
    RGBA16_UINT,
    RGBA16_UNORM,
    R32_FLOAT,
    R32_SINT,
    R32_UINT,
    RG32_FLOAT,
    RG32_SINT,
    RG32_UINT,
    RGBA32_FLOAT,
    RGBA32_SINT,
    RGBA32_UINT,
    BGRA8_UNORM,
    BGRA8_UNORM_SRGB,
    D16_UNORM,
    D24_UNORM_S8_UINT,
    D32_FLOAT,
    D32_FLOAT_S8X24_UINT,
    BC1_UNORM,
    BC1_UNORM_SRGB,
    BC2_UNORM,
    BC2_UNORM_SRGB,
    BC3_UNORM,
    BC3_UNORM_SRGB,
    BC4_SNORM,
    BC4_UNORM,
    BC5_SNORM,
    BC5_UNORM,
    BC6H_SF16,
    BC6H_UF16,
    BC7_UNORM,
    BC7_UNORM_SRGB,
};

constexpr size_t TEXTURE_FORMAT_COUNT = 58;

namespace TextureFormatUtils {

bool is_depth_stencil(TextureFormat format);
bool is_compressed(TextureFormat format);
size_t get_pixel_size(TextureFormat format);

} // namespace TextureFormatUtils

class Render {
public:
    static Render* create_instance(const RenderDescriptor& descriptor);

    virtual ~Render() = default;

    virtual RenderApi get_api() const = 0;
};

} // namespace kw
