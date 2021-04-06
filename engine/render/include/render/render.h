#pragma once

#include <cstdint>

namespace kw {

class MemoryResource;

enum class RenderApi {
    VULKAN,
    DIRECTX,
};

struct RenderDescriptor {
    RenderApi api;

    // For memory allocated and deallocated at different times.
    MemoryResource* persistent_memory_resource;

    // For memory allocated and deallocated in the same scope.
    MemoryResource* transient_memory_resource;

    bool is_validation_enabled;
    bool is_debug_names_enabled;

    // When overflows, automatically performs flush and waits for transfer to finish.
    size_t staging_buffer_size;

    // When overflows, new transient data overwrites old transient data, which may cause visual bugs.
    size_t transient_buffer_size;

    size_t buffer_allocation_size;
    size_t buffer_block_size;

    size_t texture_allocation_size;
    size_t texture_block_size;
};

struct BufferDescriptor {
    const char* name;

    const void* data;
    size_t size;
};

enum class TextureType {
    TEXTURE_2D,
    TEXTURE_CUBE, // TextureDescriptor's `array_size` must be 6.
    TEXTURE_3D,
    TEXTURE_2D_ARRAY,
    TEXTURE_CUBE_ARRAY, // TextureDescriptor's `array_size` must be a multiple of 6.
};

constexpr size_t TEXTURE_TYPE_COUNT = 5;

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

struct TextureDescriptor {
    const char* name;

    const void* data;
    size_t size;

    TextureType type;
    TextureFormat format;
    uint32_t array_size; // 0 is interpreted as 1.
    uint32_t mip_levels; // 0 is interpreted as 1.
    uint32_t width;
    uint32_t height;
    uint32_t depth; // 0 is interpreted as 1.

    // size_t offsets[array_size][mip_levels];
    const size_t* offsets;
};

// Rendering backends cast these to their own types, this is for type safety in user code.
typedef struct VertexBuffer_T {} *VertexBuffer;
typedef struct IndexBuffer_T {} *IndexBuffer;
typedef struct UniformBuffer_T {} *UniformBuffer;
typedef struct Texture_T {} *Texture;

class Render {
public:
    static Render* create_instance(const RenderDescriptor& descriptor);

    virtual ~Render() = default;

    virtual VertexBuffer create_vertex_buffer(const BufferDescriptor& buffer_descriptor) = 0;
    virtual void destroy_vertex_buffer(VertexBuffer vertex_buffer) = 0;

    virtual IndexBuffer create_index_buffer(const BufferDescriptor& buffer_descriptor) = 0;
    virtual void destroy_index_buffer(IndexBuffer index_buffer) = 0;

    virtual UniformBuffer acquire_transient_buffer(const void* data, size_t size) = 0;

    virtual Texture create_texture(const TextureDescriptor& texture_descriptor) = 0;
    virtual void destroy_texture(Texture texture) = 0;

    virtual RenderApi get_api() const = 0;
};

} // namespace kw
