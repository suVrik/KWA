#include "core/math/transform.h"

namespace kw {

transform::transform(const float4x4& value) {
    translation = float3(value[3][0], value[3][1], value[3][2]);

    scale = float3(
        length(float3(value[0][0], value[0][1], value[0][2])),
        length(float3(value[1][0], value[1][1], value[1][2])),
        length(float3(value[2][0], value[2][1], value[2][2]))
    );

    float4x4 without_scale(value);
    for (size_t i = 0; i < 3; i++) {
        without_scale[0][i] /= scale.x;
        without_scale[1][i] /= scale.y;
        without_scale[2][i] /= scale.z;
    }

    rotation = quaternion(without_scale);
}

} // namespace kw
