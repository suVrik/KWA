#pragma once

#include <core/containers/vector.h>

namespace kw {

class CameraManager;
class LightPrimitive;
class MemoryResource;
class Render;
class Scene;
class Task;
class Texture;

struct ShadowMap {
    LightPrimitive* light_primitive;

    Texture* depth_texture;
    Texture* color_texture;

    uint64_t depth_max_counter[6];
    size_t depth_primitive_count[6];

    size_t color_primitive_count[6];
};

struct ShadowManagerDescriptor {
    Render* render;
    Scene* scene;
    CameraManager* camera_manager;

    uint32_t shadow_map_count;
    uint32_t shadow_map_dimension;

    // Don't allocate translucent shadow maps, useful for local environment map baking.
    bool disable_translucent_shadows;

    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

class ShadowManager {
public:
    explicit ShadowManager(const ShadowManagerDescriptor& descriptor);
    ~ShadowManager();

    // For use by opaque shadow render pass and translucent shadow render pass.
    Vector<ShadowMap>& get_shadow_maps();

    // These return dummy textures if this light doesn't cast any shadows.
    Texture* get_depth_texture(LightPrimitive* light_primitive) const;
    Texture* get_color_texture(LightPrimitive* light_primitive) const;

    uint32_t get_shadow_map_dimension() const;

    // Assignes shadow maps to light primitives.
    Task* create_task();

private:
    class Task;

    Render& m_render;
    Scene& m_scene;
    CameraManager& m_camera_manager;

    Vector<ShadowMap> m_shadow_maps;
    Texture* m_dummy_depth_texture;
    Texture* m_dummy_color_texture;
    uint32_t m_shadow_map_dimension;

    MemoryResource& m_persistent_memory_resource;
    MemoryResource& m_transient_memory_resource;
};

} // namespace kw
