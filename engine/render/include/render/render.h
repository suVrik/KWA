#pragma once

#include <cstdint>

namespace kw {

class MemoryResource;
class Task;

enum class RenderApi : uint32_t {
    VULKAN,
    DIRECTX,
};

constexpr size_t RENDER_API_COUNT = 2;

struct RenderDescriptor {
    RenderApi api;

    // For memory allocated and deallocated at different times.
    MemoryResource* persistent_memory_resource;

    // For memory allocated and deallocated within a frame.
    MemoryResource* transient_memory_resource;

    bool is_validation_enabled;
    bool is_debug_names_enabled;

    // When overflows, automatically performs flush and waits for transfer to finish.
    uint64_t staging_buffer_size;

    // When overflows, new transient data overwrites old transient data, which may cause visual bugs.
    uint64_t transient_buffer_size;

    uint64_t buffer_allocation_size;
    uint64_t buffer_block_size;

    uint64_t texture_allocation_size;
    uint64_t texture_block_size;
};

enum class IndexSize : uint32_t {
    UINT16,
    UINT32,
};

constexpr size_t INDEX_SIZE_COUNT = 2;

enum class TextureType : uint32_t {
    TEXTURE_2D,
    TEXTURE_CUBE, // TextureDescriptor's `array_layer_count` must be 6.
    TEXTURE_3D,
    TEXTURE_2D_ARRAY,
    TEXTURE_CUBE_ARRAY, // TextureDescriptor's `array_layer_count` must be a multiple of 6.
};

constexpr size_t TEXTURE_TYPE_COUNT = 5;

enum class TextureFormat : uint32_t {
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
    RGB32_FLOAT,
    RGB32_SINT,
    RGB32_UINT,
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

constexpr size_t TEXTURE_FORMAT_COUNT = 61;

namespace TextureFormatUtils {

bool is_depth(TextureFormat format);
bool is_depth_stencil(TextureFormat format);
bool is_compressed(TextureFormat format);

bool is_allowed_texture(TextureFormat format);
bool is_allowed_attachment(TextureFormat format);
bool is_allowed_attribute(TextureFormat format);

// For compressed textures returns size of a 4x4 block.
uint64_t get_texel_size(TextureFormat format);

} // namespace TextureFormatUtils

class VertexBuffer {
public:
    size_t get_size() const {
        return static_cast<size_t>(m_size);
    }

    // Buffer range that contains some uploaded data and can be accessed in draw calls.
    size_t get_available_size() const {
        return static_cast<size_t>(m_available_size);
    }

    bool is_transient() const {
        return m_is_transient != 0;
    }

protected:
    VertexBuffer() = default;
    ~VertexBuffer() = default;

    uint64_t m_size;
    uint64_t m_available_size : 63;
    uint64_t m_is_transient : 1;
};

class IndexBuffer {
public:
    size_t get_size() const {
        return static_cast<size_t>(m_size);
    }
    
    // Buffer range that contains some uploaded data and can be accessed in draw calls.
    size_t get_available_size() const {
        return static_cast<size_t>(m_available_size);
    }

    IndexSize get_index_size() const {
        return static_cast<IndexSize>(m_index_size);
    }

    bool is_transient() const {
        return m_is_transient != 0;
    }

protected:
    IndexBuffer() = default;
    ~IndexBuffer() = default;

    uint64_t m_size : 63;
    uint64_t m_index_size : 1;
    uint64_t m_available_size : 63;
    uint64_t m_is_transient : 1;
};

class UniformBuffer {
public:
    size_t get_size() const {
        return static_cast<size_t>(m_size);
    }

protected:
    UniformBuffer() = default;
    ~UniformBuffer() = default;

    uint64_t m_size;
};

class Texture {
public:
    TextureType get_type() const {
        return m_type;
    }

    TextureFormat get_format() const {
        return m_format;
    }

    uint32_t get_mip_level_count() const {
        return m_mip_level_count;
    }

    // Mip levels that contain some uploaded data and can be used in draw calls. Goes from smallest to largest.
    // If mip level count is 10 and available mip level count is 2, it means mip levels 9 and 8 are available.
    uint32_t get_available_mip_level_count() const {
        return m_available_mip_level_count;
    }

    uint32_t get_array_layer_count() const {
        return m_array_layer_count;
    }

    uint32_t get_width() const {
        return m_width;
    }

    uint32_t get_height() const {
        return m_height;
    }

    uint32_t get_depth() const {
        return m_depth;
    }

protected:
    Texture() = default;
    ~Texture() = default;

    TextureType m_type : 5;
    TextureFormat m_format : 6;
    uint32_t m_mip_level_count : 5;
    uint32_t m_array_layer_count : 11;
    uint32_t m_available_mip_level_count : 5;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_depth;
};

class HostTexture {
public:
    TextureType get_type() const {
        return TextureType::TEXTURE_2D;
    }

    TextureFormat get_format() const {
        return m_format;
    }

    uint32_t get_mip_level_count() const {
        return 1;
    }

    uint32_t get_array_layer_count() const {
        return 1;
    }

    uint32_t get_width() const {
        return m_width;
    }

    uint32_t get_height() const {
        return m_height;
    }

    uint32_t get_depth() const {
        return 1;
    }

protected:
    HostTexture() = default;
    ~HostTexture() = default;

    TextureFormat m_format;
    uint32_t m_width;
    uint32_t m_height;
};

struct CreateTextureDescriptor {
    const char* name;

    TextureType type;
    TextureFormat format;
    uint32_t mip_level_count; // 0 is interpreted as 1.
    uint32_t array_layer_count; // 0 is interpreted as 1.
    uint32_t width; // Width of largest mip level.
    uint32_t height; // Height of largest mip level.
    uint32_t depth; // Depth of largest mip level. 0 is interpreted as 1.
};

struct UploadTextureDescriptor {
    Texture* texture;
    
    // The smallest mip is the first, the largest mip is the last. Inside each mip level, array layers are ordered in
    // natural order. Inside each array layer, depth slices are ordered in natural order. Inside each depth slice,
    // rows are ordered in natural order. Inside each row, columns are ordered in natural order.
    const void* data;
    size_t size;

    // Because the mip layers are reversed, `base_mip_level` equal to 10 and `mip_level_count` equal to 4
    // mean that mip levels 10, 9, 8, 7 are stored in `data`.
    uint32_t base_mip_level;
    uint32_t mip_level_count; // 0 is interpreted as 1.
    uint32_t base_array_layer;
    uint32_t array_layer_count; // 0 is interpreted as 1.
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t width; // Width of base mip level or sub-range within it.
    uint32_t height; // Height of base mip level or sub-range within it.
    uint32_t depth; // Depth of base mip level or sub-range within it. 0 is interpreted as 1.
};

class Render {
public:
    static Render* create_instance(const RenderDescriptor& descriptor);

    virtual ~Render() = default;

    // Created vertex buffer's subranges can't be used in draw calls until uploaded.
    virtual VertexBuffer* create_vertex_buffer(const char* name, size_t size) = 0;

    // Upload given data right after available vertex buffer data. The total uploaded data size must not exceed the
    // vertex buffer size. Uploaded data can be used in draw calls right away. May block if staging buffer is full
    // and needs to be flushed. If data is larger than staging buffer, multiple flushes will be performed.
    virtual void upload_vertex_buffer(VertexBuffer* vertex_buffer, const void* data, size_t size) = 0;

    // The actual resource is destroyed when all frames that were using it have completed on device.
    virtual void destroy_vertex_buffer(VertexBuffer* vertex_buffer) = 0;

    // Created index buffer's subranges can't be used in draw calls until uploaded.
    virtual IndexBuffer* create_index_buffer(const char* name, size_t size, IndexSize index_size) = 0;

    // Upload given data right after available index buffer data. The total uploaded data size must not exceed the
    // index buffer size. Uploaded data can be used in draw calls right away. May block if staging buffer is full
    // and needs to be flushed. If data is larger than staging buffer, multiple flushes will be performed.
    virtual void upload_index_buffer(IndexBuffer* index_buffer, const void* data, size_t size) = 0;

    // The actual resource is destroyed when all frames that were using it have completed on device.
    virtual void destroy_index_buffer(IndexBuffer* index_buffer) = 0;

    // Created texture can't be used in draw calls until at least the smallest mip level is available.
    virtual Texture* create_texture(const CreateTextureDescriptor& create_texture_descriptor) = 0;

    // Texture data must be uploaded sequentially: larger mip level after smaller mip level, higher index array layer
    // after lower index array layer..., right-hand column after left-hand column. When some mip level is uploaded,
    // it automatically becomes available and can be sampled in shaders. May block if staging buffer is full
    // and needs to be flushed. If data is larger than staging buffer, multiple flushes will be performed.
    virtual void upload_texture(const UploadTextureDescriptor& upload_texture_descriptor) = 0;

    // The actual resource is destroyed when all frames that were using it have completed on device.
    virtual void destroy_texture(Texture* texture) = 0;

    // You can blit to host textures and read them on host.
    virtual HostTexture* create_host_texture(const char* name, TextureFormat format, uint32_t width, uint32_t height) = 0;

    // Read the given host texture to host memory.
    virtual void read_host_texture(HostTexture* host_texture, void* buffer, size_t size) = 0;

    // The actual resource is destroyed when all frames that were using it have completed on device.
    virtual void destroy_host_texture(HostTexture* host_texture) = 0;

    // Buffer and handle lifetime is defined by transient memory resource. Must NOT be destroyed manually.
    virtual VertexBuffer* acquire_transient_vertex_buffer(const void* data, size_t size) = 0;
    virtual IndexBuffer* acquire_transient_index_buffer(const void* data, size_t size, IndexSize index_size) = 0;
    virtual UniformBuffer* acquire_transient_uniform_buffer(const void* data, size_t size) = 0;

    // Create task that flushes all uploads to device. Tasks that want their uploads to be transferred to device on
    // current frame must run before this task.
    virtual Task* create_task() = 0;

    virtual RenderApi get_api() const = 0;
};

} // namespace kw
