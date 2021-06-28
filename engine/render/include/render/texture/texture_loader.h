#pragma once

#include "render/render.h"

#include <core/utils/parser_utils.h>

namespace kw {

class TextureLoader {
public:
    TextureLoader();
    explicit TextureLoader(const char* relative_path);

    // `name` field must be set outside.
    const CreateTextureDescriptor& get_create_texture_descriptor() const {
        return m_create_texture_descriptor;
    }

    // `texture` field must be set outside.
    UploadTextureDescriptor load(MemoryResource& transient_memory_resource, size_t size);

    bool is_loaded() const {
        return m_current_mip_level == UINT32_MAX;
    }

private:
    uint32_t read_next();

    Reader m_reader;
    CreateTextureDescriptor m_create_texture_descriptor;
    uint32_t m_current_mip_level;
    uint32_t m_current_array_layer;
    uint32_t m_current_z;
    uint32_t m_current_y;
    uint32_t m_current_x;
};

} // namespace kw
