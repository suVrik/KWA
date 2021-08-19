#pragma once

#include <core/containers/shared_ptr.h>
#include <core/containers/vector.h>
#include <core/math/float4x4.h>

namespace kw {

class GraphicsPipeline;
class Texture;

class Material {
public:
    static constexpr size_t MAX_JOINT_COUNT = 32;

    //
    // Geometry render pass.
    //

    struct GeometryInstanceData {
        float4x4 model;
        float4x4 inverse_transpose_model;
    };

    struct UniformData {
        float4x4 model;
        float4x4 inverse_transpose_model;
        float4x4 joint_data[MAX_JOINT_COUNT];
    };

    struct GeometryPushConstants {
        float4x4 view_projection;
    };

    //
    // Opaque shadow render pass.
    //

    struct ShadowInstanceData {
        float4x4 model;
    };

    struct ShadowUniformData {
        float4x4 joint_data[MAX_JOINT_COUNT];
    };

    struct ShadowPushConstants {
        float4x4 view_projection;
    };

    //
    // Particle system render pass & translucent shadow render pass.
    //

    struct ParticleInstanceData {
        float4x4 model;
        float4 color;
        float2 uv_translation;
    };

    struct ParticlePushConstants {
        float4x4 view_projection;
        float4 uv_scale;
    };

    Material();
    Material(SharedPtr<GraphicsPipeline*> graphics_pipeline, Vector<SharedPtr<Texture*>>&& textures,
             bool is_shadow, bool is_skinned, bool is_particle);

    const SharedPtr<GraphicsPipeline*>& get_graphics_pipeline() const;
    const Vector<SharedPtr<Texture*>>& get_textures() const;
    bool is_shadow() const;
    bool is_geometry() const;
    bool is_skinned() const;
    bool is_particle() const;

    bool is_loaded() const;

private:
    SharedPtr<GraphicsPipeline*> m_graphics_pipeline;
    Vector<SharedPtr<Texture*>> m_textures;
    bool m_is_shadow;
    bool m_is_skinned;
    bool m_is_particle;
};

} // namespace kw
