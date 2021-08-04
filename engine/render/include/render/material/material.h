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
        float4x4 inverse_model;
    };

    struct UniformData {
        float4x4 model;
        float4x4 inverse_model;
        float4x4 joint_data[MAX_JOINT_COUNT];
    };

    struct PushConstants {
        float4x4 view_projection;
    };

    Material();
    Material(SharedPtr<GraphicsPipeline*> graphics_pipeline, Vector<SharedPtr<Texture*>>&& textures, bool is_skinned);

    const SharedPtr<GraphicsPipeline*>& get_graphics_pipeline() const {
        return m_graphics_pipeline;
    }

    const Vector<SharedPtr<Texture*>>& get_textures() const {
        return m_textures;
    }
    
    bool is_skinned() const {
        return m_is_skinned;
    }

private:
    SharedPtr<GraphicsPipeline*> m_graphics_pipeline;
    Vector<SharedPtr<Texture*>> m_textures;
    bool m_is_skinned;
};

} // namespace kw
