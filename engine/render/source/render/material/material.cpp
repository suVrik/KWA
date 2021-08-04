#include "render/material/material.h"

#include <core/memory/malloc_memory_resource.h>

namespace kw {

// `MallocMemoryResource` is required here because of MSVC's STL debug iterators.
// It allocates some stuff in constructor (only in debug though).
Material::Material()
    : m_graphics_pipeline(nullptr)
    , m_textures(MallocMemoryResource::instance())
    , m_is_skinned(false)
{
}
    
Material::Material(SharedPtr<GraphicsPipeline*> graphics_pipeline, Vector<SharedPtr<Texture*>>&& textures, bool is_skinned)
    : m_graphics_pipeline(std::move(graphics_pipeline))
    , m_textures(std::move(textures))
    , m_is_skinned(is_skinned)
{
}

const SharedPtr<GraphicsPipeline*>& Material::get_graphics_pipeline() const {
    return m_graphics_pipeline;
}

const Vector<SharedPtr<Texture*>>& Material::get_textures() const {
    return m_textures;
}

bool Material::is_skinned() const {
    return m_is_skinned;
}

} // namespace kw
