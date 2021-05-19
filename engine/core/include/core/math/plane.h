#pragma once

#include "core/math/float3.h"

namespace kw {

class plane {
public:
    plane()
        : normal(1.f, 0.f, 0.f)
        , distance(0.f)
    {
    }

    plane(const float3& normal, float distance)
        : normal(normal)
        , distance(distance)
    {
    }

    union {
        struct {
            float3 normal;
            float distance;
        };

        float data[4];
    };
};

} // namespace kw
