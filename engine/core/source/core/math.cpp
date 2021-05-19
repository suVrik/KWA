#include "core/math.h"

#include <algorithm>
#include <cassert>

namespace kw {

alignas(32) static const uint8_t LOG2_TABLE_32[32] = {
     0,  9,  1, 10, 13, 21,  2, 29,
    11, 14, 16, 18, 22, 25,  3, 30,
     8, 12, 20, 28, 15, 17, 24,  7,
    19, 27, 23,  6, 26,  5,  4, 31,
};

uint32_t log2(uint32_t value) {
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    return LOG2_TABLE_32[(value * 0x07C4ACDD) >> 27];
}

alignas(64) static const uint8_t LOG2_TABLE_64[64] = {
    63,  0, 58,  1, 59, 47, 53,  2,
    60, 39, 48, 27, 54, 33, 42,  3,
    61, 51, 37, 40, 49, 18, 28, 20,
    55, 30, 34, 11, 43, 14, 22,  4,
    62, 57, 46, 52, 38, 26, 32, 41,
    50, 36, 17, 19, 29, 10, 13, 21,
    56, 45, 25, 31, 35, 16,  9, 12,
    44, 24, 15,  8, 23,  7,  6,  5,
};

uint64_t log2(uint64_t value) {
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    return LOG2_TABLE_64[((value - (value >> 1)) * 0x07EDD5E59A4E28C2) >> 58];
}

float4x4 float4x4::rotation(const float3& axis, float angle) {
    assert(isfinite(axis));
    assert(std::isfinite(angle));
    assert(equal(length(axis), 1.f));

    float cos = std::cos(angle);
    float sin = std::sin(angle);
    float3 tmp = axis * (1.f - cos);

    return float4x4(cos + axis.x * tmp.x,          axis.y * tmp.x + axis.z * sin, axis.z * tmp.x - axis.y * sin, 0.f,
                    axis.x * tmp.y - axis.z * sin, cos + axis.y * tmp.y,          axis.z * tmp.y + axis.x * sin, 0.f,
                    axis.x * tmp.z + axis.y * sin, axis.y * tmp.z - axis.x * sin, cos + axis.z * tmp.z,          0.f,
                    0.f,                           0.f,                           0.f,                           1.f);
}

float4x4 float4x4::scale(const float3& scale) {
    assert(isfinite(scale));

    return float4x4(scale.x, 0.f,     0.f,     0.f,
                    0.f,     scale.y, 0.f,     0.f,
                    0.f,     0.f,     scale.z, 0.f,
                    0.f,     0.f,     0.f,     1.f);
}

float4x4 float4x4::translation(const float3& translation) {
    assert(isfinite(translation));

    return float4x4(1.f,           0.f,           0.f,           0.f,
                    0.f,           1.f,           0.f,           0.f,
                    0.f,           0.f,           1.f,           0.f,
                    translation.x, translation.y, translation.z, 1.f);
}

float4x4 float4x4::perspective_lh(float fov_y, float aspect, float z_near, float z_far) {
    assert(std::isfinite(fov_y));
    assert(std::isfinite(aspect));
    assert(std::isfinite(z_near));
    assert(std::isfinite(z_far));
    assert(!equal(aspect, 0.f));
    assert(!equal(fov_y, 0.f));
    assert(!equal(z_near, z_far));

    float multiplier = 1.f / std::tan(fov_y * 0.5f);
    return float4x4(multiplier / aspect, 0.f,        0.f,                               0.f,
                    0.f,                 multiplier, 0.f,                               0.f,
                    0.f,                 0.f,        z_far / (z_far - z_near),          1.f,
                    0.f,                 0.f,        z_far * z_near / (z_near - z_far), 0.f);
}

float4x4 float4x4::perspective_rh(float fov_y, float aspect, float z_near, float z_far) {
    assert(std::isfinite(fov_y));
    assert(std::isfinite(aspect));
    assert(std::isfinite(z_near));
    assert(std::isfinite(z_far));
    assert(!equal(aspect, 0.f));
    assert(!equal(fov_y, 0.f));
    assert(!equal(z_near, z_far));

    float tan_half_fov_y = std::tan(fov_y * 0.5f);
    return float4x4(1.f / (aspect * tan_half_fov_y), 0.f,                  0.f,                               0.f,
                    0.f,                             1.f / tan_half_fov_y, 0.f,                               0.f,
                    0.f,                             0.f,                  z_far / (z_near - z_far),          -1.f,
                    0.f,                             0.f,                  z_far * z_near / (z_near - z_far), 0.f);
}

float4x4 float4x4::orthographic_lh(float left, float right, float bottom, float top, float z_near, float z_far) {
    assert(std::isfinite(left));
    assert(std::isfinite(right));
    assert(std::isfinite(bottom));
    assert(std::isfinite(top));
    assert(std::isfinite(z_near));
    assert(std::isfinite(z_far));
    assert(!equal(left, right));
    assert(!equal(top, bottom));
    assert(!equal(z_near, z_far));

    return float4x4(2.f / (right - left),            0.f,                             0.f,                       0.f,
                    0.f,                             2.f / (top - bottom),            0.f,                       0.f,
                    0.f,                             0.f,                             1.f / (z_far - z_near),    0.f,
                    (left + right) / (left - right), (top + bottom) / (bottom - top), z_near / (z_near - z_far), 1.f);
}

float4x4 float4x4::orthographic_rh(float left, float right, float bottom, float top, float z_near, float z_far) {
    assert(!equal(left, right));
    assert(!equal(top, bottom));
    assert(!equal(z_near, z_far));

    return float4x4(2.f / (right - left),            0.f,                             0.f,                       0.f,
                    0.f,                             2.f / (top - bottom),            0.f,                       0.f,
                    0.f,                             0.f,                             1.f / (z_near - z_far),    0.f,
                    (left + right) / (left - right), (top + bottom) / (bottom - top), z_near / (z_near - z_far), 1.f);
}

float4x4 float4x4::look_at_lh(const float3& source, const float3& target, const float3& up) {
    assert(isfinite(source));
    assert(isfinite(target));
    assert(isfinite(up));
    assert(!equal(source, target));
    assert(!equal(up, float3()));
    assert(!equal(normalize(target - source), normalize(up)));

    float3 f(normalize(target - source));
    float3 s(normalize(cross(up, f)));
    float3 u(cross(f, s));

    return float4x4(s.x,             u.x,             f.x,             0.f,
                    s.y,             u.y,             f.y,             0.f,
                    s.z,             u.z,             f.z,             0.f,
                    -dot(source, s), -dot(source, u), -dot(source, f), 1.f);
}

float4x4 float4x4::look_at_rh(const float3& source, const float3& target, const float3& up) {
    assert(isfinite(source));
    assert(isfinite(target));
    assert(isfinite(up));
    assert(!equal(source, target));
    assert(!equal(up, float3()));
    assert(!equal(normalize(source - target), normalize(up)));

    float3 f(normalize(source - target));
    float3 s(normalize(cross(up, f)));
    float3 u(cross(f, s));

    return float4x4(s.x,             u.x,             f.x,             0.f,
                    s.y,             u.y,             f.y,             0.f,
                    s.z,             u.z,             f.z,             0.f,
                    -dot(source, s), -dot(source, u), -dot(source, f), 1.f);
}

quaternion quaternion::rotation(const float3& axis, float angle) {
    assert(isfinite(axis));
    assert(std::isfinite(angle));
    assert(equal(length(axis), 1.f));

    return quaternion(std::sin(0.5f * angle) * axis, std::cos(0.5f * angle));
}

quaternion quaternion::shortest_arc(const float3& from, const float3& to, const float3& spin_axis) {
    assert(isfinite(from));
    assert(isfinite(to));
    assert(isfinite(spin_axis));
    assert(equal(length(from), 1.f));
    assert(equal(length(to), 1.f));
    assert(equal(length(spin_axis), 1.f));

    float dot_product = dot(from, to);
    if (equal(dot_product, 1.f)) {
        return quaternion();
    } else if (equal(dot_product, -1.f)) {
        return quaternion(spin_axis, 0.f);
    } else {
        return normalize(quaternion(cross(from, to), dot_product + 1.f));
    }
}

quaternion quaternion::from_float3x3(const float3x3& matrix) {
    assert(isfinite(matrix));

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
            return quaternion(greatest_value,
                              (matrix._12 + matrix._21) * multiplier,
                              (matrix._31 + matrix._13) * multiplier,
                              (matrix._23 - matrix._32) * multiplier);
        case 1:
            return quaternion((matrix._12 + matrix._21) * multiplier,
                              greatest_value,
                              (matrix._23 + matrix._32) * multiplier,
                              (matrix._31 - matrix._13) * multiplier);
        case 2:
            return quaternion((matrix._31 + matrix._13) * multiplier,
                              (matrix._23 + matrix._32) * multiplier,
                              greatest_value,
                              (matrix._12 - matrix._21) * multiplier);
        case 3:
            return quaternion((matrix._23 - matrix._32) * multiplier,
                              (matrix._31 - matrix._13) * multiplier,
                              (matrix._12 - matrix._21) * multiplier,
                              greatest_value);
        default:
            return quaternion(0.f, 0.f, 0.f, 1.f);
    }
}

quaternion quaternion::from_float4x4(const float4x4& matrix) {
    assert(isfinite(matrix));

    return from_float3x3(float3x3(matrix._11, matrix._12, matrix._13,
                                  matrix._21, matrix._22, matrix._23,
                                  matrix._31, matrix._32, matrix._33));
}

float3x3 quaternion::to_float3x3(const quaternion& quaternion) {
    assert(isfinite(quaternion));

    float xx = quaternion.x * quaternion.x;
    float xy = quaternion.x * quaternion.y;
    float xz = quaternion.x * quaternion.z;
    float xw = quaternion.x * quaternion.w;
    float yy = quaternion.y * quaternion.y;
    float yz = quaternion.y * quaternion.z;
    float yw = quaternion.y * quaternion.w;
    float zz = quaternion.z * quaternion.z;
    float zw = quaternion.z * quaternion.w;

    return float3x3(1.f - 2.f * (yy + zz), 2.f * (xy + zw),       2.f * (xz - yw),
                    2.f * (xy - zw),       1.f - 2.f * (xx + zz), 2.f * (yz + xw),
                    2.f * (xz + yw),       2.f * (yz - xw),       1.f - 2.f * (xx + yy));
}

float4x4 quaternion::to_float4x4(const quaternion& quaternion) {
    return float4x4(to_float3x3(quaternion));
}

float4x4 transform::to_float4x4(const transform& transform) {
    float xx = transform.rotation.x * transform.rotation.x;
    float xy = transform.rotation.x * transform.rotation.y;
    float xz = transform.rotation.x * transform.rotation.z;
    float xw = transform.rotation.x * transform.rotation.w;
    float yy = transform.rotation.y * transform.rotation.y;
    float yz = transform.rotation.y * transform.rotation.z;
    float yw = transform.rotation.y * transform.rotation.w;
    float zz = transform.rotation.z * transform.rotation.z;
    float zw = transform.rotation.z * transform.rotation.w;

    return float4x4(transform.scale.x * (1.f - 2.f * (yy + zz)), 2.f * transform.scale.x * (xy + zw),         2.f * transform.scale.x * (xz - yw),         0.f,
                    2.f * transform.scale.y * (xy - zw),         transform.scale.y * (1.f - 2.f * (xx + zz)), 2.f * transform.scale.y * (yz + xw),         0.f,
                    2.f * transform.scale.z * (xz + yw),         2.f * transform.scale.z * (yz - xw),         transform.scale.z * (1.f - 2.f * (xx + yy)), 0.f,
                    transform.translation.x,                     transform.translation.y,                     transform.translation.z,                     1.f);
}

transform transform::from_float4x4(const float4x4& value) {
    transform result;
    
    result.translation = float3(value[3][0], value[3][1], value[3][2]);

    result.scale = float3(
        length(float3(value[0][0], value[0][1], value[0][2])),
        length(float3(value[1][0], value[1][1], value[1][2])),
        length(float3(value[2][0], value[2][1], value[2][2]))
    );

    float4x4 without_scale(value);
    for (size_t i = 0; i < 3; i++) {
        without_scale[0][i] /= result.scale.x;
        without_scale[1][i] /= result.scale.y;
        without_scale[2][i] /= result.scale.z;
    }
    result.rotation = quaternion::from_float4x4(without_scale);

    return result;
}

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

    float3x3 rotation = quaternion::to_float3x3(rhs.rotation);

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
