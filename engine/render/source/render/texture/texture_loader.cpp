#include "render/texture/texture_loader.h"

#include <core/debug/assert.h>
#include <core/error.h>
#include <core/memory/memory_resource.h>

namespace kw {

constexpr uint32_t KWT_SIGNATURE = ' TWK';

TextureLoader::TextureLoader(const char* path)
    : m_reader(path)
{
    KW_ERROR(m_reader, "Failed to open texture \"%s\".", path);
    KW_ERROR(read_next() == KWT_SIGNATURE, "Invalid texture \"%s\" signature.", path);

    m_create_texture_descriptor.name = path;
    m_create_texture_descriptor.type = static_cast<TextureType>(read_next());
    m_create_texture_descriptor.format = static_cast<TextureFormat>(read_next());
    m_create_texture_descriptor.mip_level_count = read_next();
    m_create_texture_descriptor.array_layer_count = read_next();
    m_create_texture_descriptor.width = read_next();
    m_create_texture_descriptor.height = read_next();
    m_create_texture_descriptor.depth = read_next();

    m_current_mip_level = m_create_texture_descriptor.mip_level_count - 1;
    m_current_array_layer = 0;
    m_current_z = 0;
    m_current_y = 0;
    m_current_x = 0;
}

UploadTextureDescriptor TextureLoader::load(MemoryResource& transient_memory_resource, size_t size) {
    KW_ASSERT(!is_loaded(), "Texture must be not loaded.");
    KW_ASSERT(size > 16, "At least 16 bytes is needed for texture loading.");

    bool is_compressed = TextureFormatUtils::is_compressed(m_create_texture_descriptor.format);
    uint64_t texel_size = TextureFormatUtils::get_texel_size(m_create_texture_descriptor.format);

    uint32_t texel_width = std::max(m_create_texture_descriptor.width >> m_current_mip_level, 1U);
    uint32_t texel_height = std::max(m_create_texture_descriptor.height >> m_current_mip_level, 1U);
    uint32_t texel_depth = std::max(m_create_texture_descriptor.depth >> m_current_mip_level, 1U);

    uint32_t block_width = is_compressed ? (texel_width + 3) / 4 : texel_width;
    uint32_t block_height = is_compressed ? (texel_height + 3) / 4 : texel_height;

    uint64_t mip_size = texel_size * block_width * block_height * texel_depth * m_create_texture_descriptor.array_layer_count;
    uint64_t total_size = 0;

    UploadTextureDescriptor result;
    result.base_mip_level = m_current_mip_level;
    result.mip_level_count = 1;
    result.base_array_layer = m_current_array_layer;
    result.array_layer_count = 1;
    result.x = m_current_x;
    result.y = m_current_y;
    result.z = m_current_z;
    result.width = 1;
    result.height = 1;
    result.depth = 1;

    if (m_current_array_layer > 0 || m_current_z > 0 || m_current_y > 0 || m_current_x > 0 || size < mip_size) {
        uint32_t array_layers_max = size / (texel_size * block_width * block_height * texel_depth);

        if (m_current_z > 0 || m_current_y > 0 || m_current_x > 0 || array_layers_max == 0) {
            uint32_t depth_slices_max = size / (texel_size * block_width * block_height);

            if (m_current_y > 0 || m_current_x > 0 || depth_slices_max == 0) {
                uint32_t rows_max = size / (texel_size * block_width);

                if (m_current_x > 0 || rows_max == 0) {
                    uint32_t columns_max = size / texel_size;
                    uint32_t columns_left = texel_width - m_current_x;
                    uint32_t column_blocks_left = is_compressed ? (columns_left + 3) / 4 : columns_left;
                    uint32_t column_blocks_to_load = std::min(column_blocks_left, columns_max);
                    uint32_t columns_to_load = is_compressed ? std::min(column_blocks_to_load * 4, columns_left) : column_blocks_to_load;
                    KW_ASSERT(columns_to_load > 0);

                    result.width = columns_to_load;
                    result.height = is_compressed ? 4 : 1;

                    total_size = texel_size * column_blocks_to_load;
                    m_current_x += columns_to_load;
                } else {
                    uint32_t rows_left = texel_height - m_current_y;
                    uint32_t row_blocks_left = is_compressed ? (rows_left + 3) / 4 : rows_left;
                    uint32_t row_blocks_to_load = std::min(row_blocks_left, rows_max);
                    uint32_t rows_to_load = is_compressed ? std::min(row_blocks_to_load * 4, rows_left) : row_blocks_to_load;
                    KW_ASSERT(rows_to_load > 0);

                    result.width = texel_width;
                    result.height = rows_to_load;

                    total_size = texel_size * row_blocks_to_load * block_width;
                    m_current_y += rows_to_load;
                }
            } else {
                uint32_t depth_slices_left = texel_depth - m_current_z;
                uint32_t depth_slices_to_load = std::min(depth_slices_left, depth_slices_max);
                KW_ASSERT(depth_slices_to_load > 0);

                result.width = texel_width;
                result.height = texel_height;
                result.depth = depth_slices_to_load;

                total_size = texel_size * depth_slices_to_load * block_width * block_height;
                m_current_z += depth_slices_to_load;
            }
        } else {
            uint32_t array_layers_left = m_create_texture_descriptor.array_layer_count - m_current_array_layer;
            uint32_t array_layers_to_load = std::min(array_layers_left, array_layers_max);
            KW_ASSERT(array_layers_to_load > 0);

            result.array_layer_count = array_layers_to_load;
            result.width = texel_width;
            result.height = texel_height;
            result.depth = texel_depth;

            total_size = texel_size * array_layers_to_load * block_width * block_height * texel_depth;
            m_current_array_layer += array_layers_to_load;
        }

        m_current_y += (m_current_x / texel_width) * (is_compressed ? 4 : 1);
        m_current_z += m_current_y / texel_height;
        m_current_array_layer += m_current_z / texel_depth;

        KW_ASSERT(m_current_array_layer / m_create_texture_descriptor.array_layer_count <= 1U);
        m_current_mip_level -= m_current_array_layer / m_create_texture_descriptor.array_layer_count;

        m_current_array_layer %= m_create_texture_descriptor.array_layer_count;
        m_current_z %= texel_depth;
        m_current_y %= texel_height;
        m_current_x %= texel_width;
    } else {
        uint32_t base_mip_level = m_current_mip_level;

        while (m_current_mip_level != UINT32_MAX && total_size + mip_size <= size) {
            total_size += mip_size;

            // This may overflow and it's ok. All the following variables will contain undefined values, but we won't use them.
            m_current_mip_level--;

            texel_width = std::max(m_create_texture_descriptor.width >> m_current_mip_level, 1U);
            texel_height = std::max(m_create_texture_descriptor.height >> m_current_mip_level, 1U);
            texel_depth = std::max(m_create_texture_descriptor.depth >> m_current_mip_level, 1U);

            block_width = is_compressed ? (texel_width + 3) / 4 : texel_width;
            block_height = is_compressed ? (texel_height + 3) / 4 : texel_height;

            mip_size = texel_size * block_width * block_height * texel_depth * m_create_texture_descriptor.array_layer_count;
        }

        KW_ASSERT(base_mip_level > m_current_mip_level || m_current_mip_level == UINT32_MAX);

        result.base_mip_level = base_mip_level;
        result.mip_level_count = base_mip_level - m_current_mip_level;
        result.array_layer_count = m_create_texture_descriptor.array_layer_count;
        result.width = std::max(m_create_texture_descriptor.width >> base_mip_level, 1U);
        result.height = std::max(m_create_texture_descriptor.height >> base_mip_level, 1U);
        result.depth = std::max(m_create_texture_descriptor.depth >> base_mip_level, 1U);
    }

    void* texture_data = transient_memory_resource.allocate(total_size, 1);

    KW_ERROR(m_reader.read(texture_data, total_size), "Failed to read texture data.");

    result.data = texture_data;
    result.size = total_size;

    return result;
}

uint32_t TextureLoader::read_next() {
    std::optional<uint32_t> value = m_reader.read_le<uint32_t>();
    KW_ERROR(value, "Failed to read texture header.");
    return *value;
}

} // namespace kw
