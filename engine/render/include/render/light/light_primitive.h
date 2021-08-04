#pragma once

#include "render/acceleration_structure/acceleration_structure_primitive.h"

#include <core/math/aabbox.h>

namespace kw {

class LightPrimitive : public AccelerationStructurePrimitive {
public:
    explicit LightPrimitive(float3 color = float3(1.f, 1.f, 1.f), float power = 1.f,
                            const transform& local_transform = transform());
    ~LightPrimitive();

    const aabbox& get_bounds() const override;

    const float3& get_color() const;
    void set_color(const float3& value);

    float get_power() const;
    void set_power(float value);

protected:
    void global_transform_updated() override;

private:
    aabbox m_bounds;
    float3 m_color;
    float m_power;
};

} // namespace kw
