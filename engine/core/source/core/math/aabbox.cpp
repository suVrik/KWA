#include "core/math/aabbox.h"
#include "core/math/transform.h"

namespace kw {

// Transforming Axis-Aligned Bounding Boxes by Jim Arvo from "Graphics Gems", Academic Press, 1990.
aabbox aabbox::operator*(const float4x4& rhs) const {
    float3 old_min = center - extent;
    float3 old_max = center + extent;

    float3 new_min(rhs[3][0], rhs[3][1], rhs[3][2]);
    float3 new_max(rhs[3][0], rhs[3][1], rhs[3][2]);
        
    for (size_t i = 0; i < 3; i++) {
        for (size_t j = 0; j < 3; j++) {
            float a = rhs[i][j] * old_min[i];
            float b = rhs[i][j] * old_max[i];
            if (a < b) {
                new_min[j] += a;
                new_max[j] += b;
            } else {
                new_min[j] += b;
                new_max[j] += a;
            }
        }
    }

    return from_min_max(new_min, new_max);
}

aabbox aabbox::operator*(const transform& rhs) const {
    float3 old_min = (center - extent) * rhs.scale;
    float3 old_max = (center + extent) * rhs.scale;

    float3 new_min = rhs.translation;
    float3 new_max = rhs.translation;

    float3x3 rotation(rhs.rotation);

    for (size_t i = 0; i < 3; i++) {
        for (size_t j = 0; j < 3; j++) {
            float a = rotation[i][j] * old_min[i];
            float b = rotation[i][j] * old_max[i];
            if (a < b) {
                new_min[j] += a;
                new_max[j] += b;
            } else {
                new_min[j] += b;
                new_max[j] += a;
            }
        }
    }

    return from_min_max(new_min, new_max);
}

} // namespace kw
