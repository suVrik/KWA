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

    struct InstanceData {
        float4x4 model;
        float4x4 inverse_transpose_model;
    };

    struct UniformData {
        float4x4 model;
        float4x4 inverse_transpose_model;
        float4x4 joint_data[MAX_JOINT_COUNT];
    };

    struct PushConstants {
        float4x4 view_projection;
    };

    Material();
    Material(SharedPtr<GraphicsPipeline*> graphics_pipeline, Vector<SharedPtr<Texture*>>&& textures, bool is_skinned);

    const SharedPtr<GraphicsPipeline*>& get_graphics_pipeline() const;
    const Vector<SharedPtr<Texture*>>& get_textures() const;
    bool is_skinned() const;

private:
    SharedPtr<GraphicsPipeline*> m_graphics_pipeline;
    Vector<SharedPtr<Texture*>> m_textures;
    bool m_is_skinned;
};

} // namespace kw
