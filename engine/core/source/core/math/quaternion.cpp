#include "core/math/quaternion.h"
#include "core/debug/assert.h"
#include "core/math/float4x4.h"

namespace kw {

quaternion quaternion::rotation(const float3& axis, float angle) {
    KW_ASSERT(isfinite(axis));
    KW_ASSERT(std::isfinite(angle));
    KW_ASSERT(equal(length(axis), 1.f));

    return quaternion(std::sin(0.5f * angle) * axis, std::cos(0.5f * angle));
}

quaternion quaternion::shortest_arc(const float3& from, const float3& to, const float3& spin_axis) {
    KW_ASSERT(isfinite(from));
    KW_ASSERT(isfinite(to));
    KW_ASSERT(isfinite(spin_axis));
    KW_ASSERT(equal(length(from), 1.f));
    KW_ASSERT(equal(length(to), 1.f));
    KW_ASSERT(equal(length(spin_axis), 1.f));

    float dot_product = dot(from, to);
    if (equal(dot_product, 1.f)) {
        return quaternion();
    } else if (equal(dot_product, -1.f)) {
        return quaternion(spin_axis, 0.f);
    } else {
        return normalize(quaternion(cross(from, to), dot_product + 1.f));
    }
}

quaternion::quaternion(const float3x3& matrix) {
    KW_ASSERT(isfinite(matrix));

    float a = matrix._11 - matrix._22 - matrix._33;
    float b = matrix._22 - matrix._11 - matrix._33;
    float c = matrix._33 - matrix._11 - matrix._22;
    float d = matrix._11 + matrix._22 + matrix._33;

    int greatest_index = 3;
    float greatest_index_value = d;

    if (a > greatest_index_value) {
        greatest_index_value = a;
        greatest_index = 0;
    }

    if (b > greatest_index_value) {
        greatest_index_value = b;
        greatest_index = 1;
    }

    if (c > greatest_index_value) {
        greatest_index_value = c;
        greatest_index = 2;
    }

    float greatest_value = std::sqrt(greatest_index_value + 1.f) * 0.5f;
    float multiplier = 0.25f / greatest_value;

    switch (greatest_index) {
        case 0:
            x = greatest_value;
            y = (matrix._12 + matrix._21) * multiplier;
            z = (matrix._31 + matrix._13) * multiplier;
            w = (matrix._23 - matrix._32) * multiplier;
            break;
        case 1:
            x = (matrix._12 + matrix._21) * multiplier;
            y = greatest_value;
            z = (matrix._23 + matrix._32) * multiplier;
            w = (matrix._31 - matrix._13) * multiplier;
            break;
        case 2:
            x = (matrix._31 + matrix._13) * multiplier;
            y = (matrix._23 + matrix._32) * multiplier;
            z = greatest_value;
            w = (matrix._12 - matrix._21) * multiplier;
            break;
        case 3:
            x = (matrix._23 - matrix._32) * multiplier;
            y = (matrix._31 - matrix._13) * multiplier;
            z = (matrix._12 - matrix._21) * multiplier;
            w = greatest_value;
            break;
        default:
            x = 0.f;
            y = 0.f;
            z = 0.f;
            w = 1.f;
    }
}

quaternion::quaternion(const float4x4& matrix)
    : quaternion(float3x3(matrix._11, matrix._12, matrix._13,
                          matrix._21, matrix._22, matrix._23,
                          matrix._31, matrix._32, matrix._33))
{
}

float4::float4(const quaternion& value)
    : x(value.x), y(value.y), z(value.z), w(value.w)
{
}

} // namespace kw
