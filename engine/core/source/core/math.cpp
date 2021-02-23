#include "core/math.h"

#include <algorithm>
#include <cassert>

namespace kw {

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

quaternion quaternion::from_matrix(const float3x3& matrix) {
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

quaternion quaternion::from_matrix(const float4x4& matrix) {
    assert(isfinite(matrix));

    return from_matrix(float3x3(matrix._11, matrix._12, matrix._13,
                                matrix._21, matrix._22, matrix._23,
                                matrix._31, matrix._32, matrix._33));
}

float4x4 quaternion::to_matrix(const quaternion& quaternion) {
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

    return float4x4(1.f - 2.f * (yy + zz), 2.f * (xy + zw),       2.f * (xz - yw),       0.f,
                    2.f * (xy - zw),       1.f - 2.f * (xx + zz), 2.f * (yz + xw),       0.f,
                    2.f * (xz + yw),       2.f * (yz - xw),       1.f - 2.f * (xx + yy), 0.f,
                    0.f,                   0.f,                   0.f,                   1.f);
}

} // namespace kw
