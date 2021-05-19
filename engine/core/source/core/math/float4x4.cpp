#include "core/math/float4x4.h"
#include "core/debug/assert.h"
#include "core/math/transform.h"

namespace kw {

float4x4 float4x4::rotation(const float3& axis, float angle) {
    KW_ASSERT(isfinite(axis));
    KW_ASSERT(std::isfinite(angle));
    KW_ASSERT(equal(length(axis), 1.f));

    float cos = std::cos(angle);
    float sin = std::sin(angle);
    float3 tmp = axis * (1.f - cos);

    return float4x4(cos + axis.x * tmp.x,          axis.y * tmp.x + axis.z * sin, axis.z * tmp.x - axis.y * sin, 0.f,
                    axis.x * tmp.y - axis.z * sin, cos + axis.y * tmp.y,          axis.z * tmp.y + axis.x * sin, 0.f,
                    axis.x * tmp.z + axis.y * sin, axis.y * tmp.z - axis.x * sin, cos + axis.z * tmp.z,          0.f,
                    0.f,                           0.f,                           0.f,                           1.f);
}

float4x4 float4x4::scale(const float3& scale) {
    KW_ASSERT(isfinite(scale));

    return float4x4(scale.x, 0.f,     0.f,     0.f,
                    0.f,     scale.y, 0.f,     0.f,
                    0.f,     0.f,     scale.z, 0.f,
                    0.f,     0.f,     0.f,     1.f);
}

float4x4 float4x4::translation(const float3& translation) {
    KW_ASSERT(isfinite(translation));

    return float4x4(1.f,           0.f,           0.f,           0.f,
                    0.f,           1.f,           0.f,           0.f,
                    0.f,           0.f,           1.f,           0.f,
                    translation.x, translation.y, translation.z, 1.f);
}

float4x4 float4x4::perspective_lh(float fov_y, float aspect, float z_near, float z_far) {
    KW_ASSERT(std::isfinite(fov_y));
    KW_ASSERT(std::isfinite(aspect));
    KW_ASSERT(std::isfinite(z_near));
    KW_ASSERT(std::isfinite(z_far));
    KW_ASSERT(!equal(aspect, 0.f));
    KW_ASSERT(!equal(fov_y, 0.f));
    KW_ASSERT(!equal(z_near, z_far));

    float multiplier = 1.f / std::tan(fov_y * 0.5f);
    return float4x4(multiplier / aspect, 0.f,        0.f,                               0.f,
                    0.f,                 multiplier, 0.f,                               0.f,
                    0.f,                 0.f,        z_far / (z_far - z_near),          1.f,
                    0.f,                 0.f,        z_far * z_near / (z_near - z_far), 0.f);
}

float4x4 float4x4::perspective_rh(float fov_y, float aspect, float z_near, float z_far) {
    KW_ASSERT(std::isfinite(fov_y));
    KW_ASSERT(std::isfinite(aspect));
    KW_ASSERT(std::isfinite(z_near));
    KW_ASSERT(std::isfinite(z_far));
    KW_ASSERT(!equal(aspect, 0.f));
    KW_ASSERT(!equal(fov_y, 0.f));
    KW_ASSERT(!equal(z_near, z_far));

    float tan_half_fov_y = std::tan(fov_y * 0.5f);
    return float4x4(1.f / (aspect * tan_half_fov_y), 0.f,                  0.f,                               0.f,
                    0.f,                             1.f / tan_half_fov_y, 0.f,                               0.f,
                    0.f,                             0.f,                  z_far / (z_near - z_far),          -1.f,
                    0.f,                             0.f,                  z_far * z_near / (z_near - z_far), 0.f);
}

float4x4 float4x4::orthographic_lh(float left, float right, float bottom, float top, float z_near, float z_far) {
    KW_ASSERT(std::isfinite(left));
    KW_ASSERT(std::isfinite(right));
    KW_ASSERT(std::isfinite(bottom));
    KW_ASSERT(std::isfinite(top));
    KW_ASSERT(std::isfinite(z_near));
    KW_ASSERT(std::isfinite(z_far));
    KW_ASSERT(!equal(left, right));
    KW_ASSERT(!equal(top, bottom));
    KW_ASSERT(!equal(z_near, z_far));

    return float4x4(2.f / (right - left),            0.f,                             0.f,                       0.f,
                    0.f,                             2.f / (top - bottom),            0.f,                       0.f,
                    0.f,                             0.f,                             1.f / (z_far - z_near),    0.f,
                    (left + right) / (left - right), (top + bottom) / (bottom - top), z_near / (z_near - z_far), 1.f);
}

float4x4 float4x4::orthographic_rh(float left, float right, float bottom, float top, float z_near, float z_far) {
    KW_ASSERT(!equal(left, right));
    KW_ASSERT(!equal(top, bottom));
    KW_ASSERT(!equal(z_near, z_far));

    return float4x4(2.f / (right - left),            0.f,                             0.f,                       0.f,
                    0.f,                             2.f / (top - bottom),            0.f,                       0.f,
                    0.f,                             0.f,                             1.f / (z_near - z_far),    0.f,
                    (left + right) / (left - right), (top + bottom) / (bottom - top), z_near / (z_near - z_far), 1.f);
}

float4x4 float4x4::look_at_lh(const float3& source, const float3& target, const float3& up) {
    KW_ASSERT(isfinite(source));
    KW_ASSERT(isfinite(target));
    KW_ASSERT(isfinite(up));
    KW_ASSERT(!equal(source, target));
    KW_ASSERT(!equal(up, float3()));
    KW_ASSERT(!equal(normalize(target - source), normalize(up)));

    float3 f(normalize(target - source));
    float3 s(normalize(cross(up, f)));
    float3 u(cross(f, s));

    return float4x4(s.x,             u.x,             f.x,             0.f,
                    s.y,             u.y,             f.y,             0.f,
                    s.z,             u.z,             f.z,             0.f,
                    -dot(source, s), -dot(source, u), -dot(source, f), 1.f);
}

float4x4 float4x4::look_at_rh(const float3& source, const float3& target, const float3& up) {
    KW_ASSERT(isfinite(source));
    KW_ASSERT(isfinite(target));
    KW_ASSERT(isfinite(up));
    KW_ASSERT(!equal(source, target));
    KW_ASSERT(!equal(up, float3()));
    KW_ASSERT(!equal(normalize(source - target), normalize(up)));

    float3 f(normalize(source - target));
    float3 s(normalize(cross(up, f)));
    float3 u(cross(f, s));

    return float4x4(s.x,             u.x,             f.x,             0.f,
                    s.y,             u.y,             f.y,             0.f,
                    s.z,             u.z,             f.z,             0.f,
                    -dot(source, s), -dot(source, u), -dot(source, f), 1.f);
}

float4x4::float4x4(const quaternion& quaternion) {
    KW_ASSERT(isfinite(quaternion));

    float xx = quaternion.x * quaternion.x;
    float xy = quaternion.x * quaternion.y;
    float xz = quaternion.x * quaternion.z;
    float xw = quaternion.x * quaternion.w;
    float yy = quaternion.y * quaternion.y;
    float yz = quaternion.y * quaternion.z;
    float yw = quaternion.y * quaternion.w;
    float zz = quaternion.z * quaternion.z;
    float zw = quaternion.z * quaternion.w;

    _m00 = 1.f - 2.f * (yy + zz);
    _m01 = 2.f * (xy + zw);
    _m02 = 2.f * (xz - yw);
    _m03 = 0.f;
    _m10 = 2.f * (xy - zw);
    _m11 = 1.f - 2.f * (xx + zz);
    _m12 = 2.f * (yz + xw);
    _m13 = 0.f;
    _m20 = 2.f * (xz + yw);
    _m21 = 2.f * (yz - xw);
    _m22 = 1.f - 2.f * (xx + yy);
    _m23 = 0.f;
    _m30 = 0.f;
    _m31 = 0.f;
    _m32 = 0.f;
    _m33 = 1.f;
}

float4x4::float4x4(const transform& transform) {
    KW_ASSERT(isfinite(transform));

    float xx = transform.rotation.x * transform.rotation.x;
    float xy = transform.rotation.x * transform.rotation.y;
    float xz = transform.rotation.x * transform.rotation.z;
    float xw = transform.rotation.x * transform.rotation.w;
    float yy = transform.rotation.y * transform.rotation.y;
    float yz = transform.rotation.y * transform.rotation.z;
    float yw = transform.rotation.y * transform.rotation.w;
    float zz = transform.rotation.z * transform.rotation.z;
    float zw = transform.rotation.z * transform.rotation.w;

    _m00 = transform.scale.x * (1.f - 2.f * (yy + zz));
    _m01 = 2.f * transform.scale.x * (xy + zw);
    _m02 = 2.f * transform.scale.x * (xz - yw);
    _m03 = 0.f;
    _m10 = 2.f * transform.scale.y * (xy - zw);
    _m11 = transform.scale.y * (1.f - 2.f * (xx + zz));
    _m12 = 2.f * transform.scale.y * (yz + xw);
    _m13 = 0.f;
    _m20 = 2.f * transform.scale.z * (xz + yw);
    _m21 = 2.f * transform.scale.z * (yz - xw);
    _m22 = transform.scale.z * (1.f - 2.f * (xx + yy));
    _m23 = 0.f;
    _m30 = transform.translation.x;
    _m31 = transform.translation.y;
    _m32 = transform.translation.z;
    _m33 = 1.f;
}

} // namespace kw
