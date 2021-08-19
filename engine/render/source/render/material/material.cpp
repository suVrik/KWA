#include "render/material/material.h"

#include <core/debug/assert.h>
#include <core/memory/malloc_memory_resource.h>

namespace kw {

// `MallocMemoryResource` is required here because of MSVC's STL debug iterators.
// It allocates some iterator stuff in the constructor (only in debug though).
Material::Material()
    : m_graphics_pipeline(nullptr)
    , m_textures(MallocMemoryResource::instance())
    , m_is_shadow(false)
    , m_is_skinned(false)
    , m_is_particle(false)
{
}

Material::Material(SharedPtr<GraphicsPipeline*> graphics_pipeline, Vector<SharedPtr<Texture*>>&& textures,
                   bool is_shadow, bool is_skinned, bool is_particle)
    : m_graphics_pipeline(std::move(graphics_pipeline))
    , m_textures(std::move(textures))
    , m_is_shadow(is_shadow)
    , m_is_skinned(is_skinned)
    , m_is_particle(is_particle)
{
    KW_ASSERT(m_graphics_pipeline, "Invalid graphics pipeline.");
    KW_ASSERT(!m_is_skinned || !is_particle, "Skinned particle is not allowed.");
}

const SharedPtr<GraphicsPipeline*>& Material::get_graphics_pipeline() const {
    return m_graphics_pipeline;
}

const Vector<SharedPtr<Texture*>>& Material::get_textures() const {
    return m_textures;
}

bool Material::is_shadow() const {
    return m_is_shadow;
}

bool Material::is_geometry() const {
    return !m_is_particle;
}

bool Material::is_skinned() const {
    return m_is_skinned;
}

bool Material::is_particle() const {
    return m_is_particle;
}

bool Material::is_loaded() const {
    return m_graphics_pipeline && *m_graphics_pipeline != nullptr;
}

} // namespace kw
