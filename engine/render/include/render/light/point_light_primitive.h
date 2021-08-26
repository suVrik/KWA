#pragma once

#include "render/light/light_primitive.h"

namespace kw {

class PointLightPrimitive : public LightPrimitive {
public:
    struct ShadowParams {
        float normal_bias;
        float perspective_bias;
        float pcss_radius;
        float pcss_filter_factor;
    };

    explicit PointLightPrimitive(bool is_shadow_enabled = false,
                                 float3 color = float3(1.f, 1.f, 1.f),
                                 float power = 1.f,
                                 const transform& local_transform = transform());

    bool is_shadow_enabled() const;
    void toggle_shadow_enabled(bool value);

    const ShadowParams& get_shadow_params() const;
    void set_shadow_params(const ShadowParams& value);

    UniquePtr<Primitive> clone(MemoryResource& memory_resource) const override;

private:
    bool m_is_shadow_enabled;
    ShadowParams m_shadow_params;
};

} // namespace kw
