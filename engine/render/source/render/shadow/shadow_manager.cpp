#include "render/shadow/shadow_manager.h"
#include "render/light/sphere_light_primitive.h"
#include "render/render.h"
#include "render/scene/scene.h"

#include <core/concurrency/task.h>
#include <core/containers/string.h>
#include <core/debug/assert.h>

#include <algorithm>

namespace kw {

class ShadowManager::Task : public kw::Task {
public:
    Task(ShadowManager& render_pass)
        : m_manager(render_pass)
    {
    }

    void run() override {
        //
        // Query all point lights in a frustum.
        //

        Vector<LightPrimitive*> primitives = m_manager.m_scene.query_lights(m_manager.m_scene.get_occlusion_camera().get_frustum());

        std::sort(primitives.begin(), primitives.end(), LightSort(m_manager.m_scene.get_camera()));
        
        Vector<SphereLightPrimitive*> shadow_lights(m_manager.m_transient_memory_resource);
        shadow_lights.reserve(m_manager.m_shadow_maps.size());

        for (uint32_t i = 0; i < primitives.size() && shadow_lights.size() < m_manager.m_shadow_maps.size(); i++) {
            if (SphereLightPrimitive* sphere_light_primitive = dynamic_cast<SphereLightPrimitive*>(primitives[i])) {
                if (sphere_light_primitive->is_shadow_enabled()) {
                    shadow_lights.push_back(sphere_light_primitive);
                }
            } else {
                KW_ASSERT(false, "Invalid light type.");
            }
        }

        //
        // Unlink shadow maps from light primitives that don't cast shadows this frame.
        //

        for (ShadowMap& shadow_map : m_manager.m_shadow_maps) {
            bool ok = false;
            for (SphereLightPrimitive*& sphere_light : shadow_lights) {
                if (shadow_map.light_primitive == sphere_light) {
                    std::swap(sphere_light, shadow_lights.back());
                    shadow_lights.pop_back();
                    ok = true;
                    break;
                }
            }
            if (!ok) {
                shadow_map.light_primitive = nullptr;
            }
        }

        //
        // Link new light primitives that cast shadows this frame.
        //

        for (SphereLightPrimitive* sphere_light : shadow_lights) {
            for (ShadowMap& shadow_map : m_manager.m_shadow_maps) {
                if (shadow_map.light_primitive == nullptr) {
                    shadow_map.light_primitive = sphere_light;
                    
                    // Light primitive has been reassigned. All cubemap sides must be re-rendered.
                    std::fill(std::begin(shadow_map.depth_max_counter), std::end(shadow_map.depth_max_counter), 0);
                    std::fill(std::begin(shadow_map.depth_primitive_count), std::end(shadow_map.depth_primitive_count), SIZE_MAX);
                    std::fill(std::begin(shadow_map.color_primitive_count), std::end(shadow_map.color_primitive_count), SIZE_MAX);

                    break;
                }
            }
        }
    }

    const char* get_name() const override {
        return "Shadow Manager";
    }

private:
    struct LightSort {
    public:
        explicit LightSort(Camera& camera)
            : m_camera(camera)
        {
        }

        bool operator()(LightPrimitive* a, LightPrimitive* b) const {
            float distance_a = square_distance(a->get_global_translation(), m_camera.get_translation());
            float distance_b = square_distance(b->get_global_translation(), m_camera.get_translation());
            if (distance_a == distance_b) {
                return a < b;
            }
            return distance_a < distance_b;
        }

    private:
        Camera& m_camera;
    };

    ShadowManager& m_manager;
};

ShadowManager::ShadowManager(const ShadowManagerDescriptor& descriptor)
    : m_render(*descriptor.render)
    , m_scene(*descriptor.scene)
    , m_shadow_maps(descriptor.shadow_map_count, ShadowMap{}, *descriptor.persistent_memory_resource)
    , m_shadow_map_dimension(descriptor.shadow_map_dimension)
    , m_persistent_memory_resource(*descriptor.persistent_memory_resource)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
{
    KW_ASSERT(descriptor.scene != nullptr);
    KW_ASSERT(descriptor.render != nullptr);
    KW_ASSERT(descriptor.shadow_map_dimension > 0 && is_pow2(descriptor.shadow_map_dimension));
    KW_ASSERT(descriptor.persistent_memory_resource != nullptr);
    KW_ASSERT(descriptor.transient_memory_resource != nullptr);

    int i = 0;

    for (ShadowMap& shadow_map : m_shadow_maps) {
        char depth_texture_name[32];
        std::snprintf(depth_texture_name, sizeof(depth_texture_name), "shadow_depth_texture_%d", i);

        CreateTextureDescriptor create_depth_texture_descriptor{};
        create_depth_texture_descriptor.name = depth_texture_name;
        create_depth_texture_descriptor.type = TextureType::TEXTURE_CUBE;
        create_depth_texture_descriptor.format = TextureFormat::D16_UNORM;
        create_depth_texture_descriptor.array_layer_count = 6;
        create_depth_texture_descriptor.width = descriptor.shadow_map_dimension;
        create_depth_texture_descriptor.height = descriptor.shadow_map_dimension;

        shadow_map.depth_texture = m_render.create_texture(create_depth_texture_descriptor);

        ClearTextureDescriptor clear_depth_texture_descriptor{};
        clear_depth_texture_descriptor.texture = shadow_map.depth_texture;
        clear_depth_texture_descriptor.clear_depth = 1.f;

        m_render.clear_texture(clear_depth_texture_descriptor);

        char color_texture_name[32];
        std::snprintf(color_texture_name, sizeof(color_texture_name), "shadow_color_texture_%d", i);

        CreateTextureDescriptor create_color_texture_descriptor{};
        create_color_texture_descriptor.name = color_texture_name;
        create_color_texture_descriptor.type = TextureType::TEXTURE_CUBE;
        create_color_texture_descriptor.format = TextureFormat::RGBA8_UNORM;
        create_color_texture_descriptor.array_layer_count = 6;
        create_color_texture_descriptor.width = descriptor.shadow_map_dimension;
        create_color_texture_descriptor.height = descriptor.shadow_map_dimension;

        shadow_map.color_texture = m_render.create_texture(create_color_texture_descriptor);

        ClearTextureDescriptor clear_color_texture_descriptor{};
        clear_color_texture_descriptor.texture = shadow_map.color_texture;
        std::fill(std::begin(clear_color_texture_descriptor.clear_color), std::end(clear_color_texture_descriptor.clear_color), 1.f);

        m_render.clear_texture(clear_color_texture_descriptor);

        i++;
    }

    CreateTextureDescriptor create_depth_texture_descriptor{};
    create_depth_texture_descriptor.name = "shadow_depth_texture_dummy";
    create_depth_texture_descriptor.type = TextureType::TEXTURE_CUBE;
    create_depth_texture_descriptor.format = TextureFormat::D16_UNORM;
    create_depth_texture_descriptor.array_layer_count = 6;
    create_depth_texture_descriptor.width = 1;
    create_depth_texture_descriptor.height = 1;

    m_dummy_depth_texture = m_render.create_texture(create_depth_texture_descriptor);

    ClearTextureDescriptor clear_depth_texture_descriptor{};
    clear_depth_texture_descriptor.texture = m_dummy_depth_texture;
    clear_depth_texture_descriptor.clear_depth = 1.f;

    m_render.clear_texture(clear_depth_texture_descriptor);

    CreateTextureDescriptor create_color_texture_descriptor{};
    create_color_texture_descriptor.name = "shadow_color_texture_dummy";
    create_color_texture_descriptor.type = TextureType::TEXTURE_CUBE;
    create_color_texture_descriptor.format = TextureFormat::RGBA8_UNORM;
    create_color_texture_descriptor.array_layer_count = 6;
    create_color_texture_descriptor.width = 1;
    create_color_texture_descriptor.height = 1;

    m_dummy_color_texture = m_render.create_texture(create_color_texture_descriptor);

    ClearTextureDescriptor clear_color_texture_descriptor{};
    clear_color_texture_descriptor.texture = m_dummy_color_texture;
    std::fill(std::begin(clear_color_texture_descriptor.clear_color), std::end(clear_color_texture_descriptor.clear_color), 1.f);

    m_render.clear_texture(clear_color_texture_descriptor);
}

ShadowManager::~ShadowManager() {
    m_render.destroy_texture(m_dummy_color_texture);
    m_render.destroy_texture(m_dummy_depth_texture);

    for (ShadowMap& shadow_map : m_shadow_maps) {
        m_render.destroy_texture(shadow_map.color_texture);
        m_render.destroy_texture(shadow_map.depth_texture);
    }
}

Vector<ShadowMap>& ShadowManager::get_shadow_maps() {
    return m_shadow_maps;
}

Texture* ShadowManager::get_depth_texture(LightPrimitive* light_primitive) const {
    for (const ShadowMap& shadow_map : m_shadow_maps) {
        if (shadow_map.light_primitive == light_primitive) {
            return shadow_map.depth_texture;
        }
    }
    return m_dummy_depth_texture;
}

Texture* ShadowManager::get_color_texture(LightPrimitive* light_primitive) const {
    for (const ShadowMap& shadow_map : m_shadow_maps) {
        if (shadow_map.light_primitive == light_primitive) {
            return shadow_map.color_texture;
        }
    }
    return m_dummy_color_texture;
}

uint32_t ShadowManager::get_shadow_map_dimension() const {
    return m_shadow_map_dimension;
}

Task* ShadowManager::create_task() {
    return m_transient_memory_resource.construct<Task>(*this);
}

} // namespace kw
