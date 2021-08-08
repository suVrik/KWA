#pragma once

#include "core/math/float3x3.h"
#include "core/math/float4.h"

namespace kw {

class transform;

class float4x4 {
public:
    static float4x4 rotation(const float3& axis, float angle);
    static float4x4 scale(const float3& scale);
    static float4x4 translation(const float3& translation);

    static float4x4 perspective_lh(float fov_y, float aspect, float z_near, float z_far);
    static float4x4 perspective_rh(float fov_y, float aspect, float z_near, float z_far);

    static float4x4 orthographic_lh(float left, float right, float bottom, float top, float z_near, float z_far);
    static float4x4 orthographic_rh(float left, float right, float bottom, float top, float z_near, float z_far);

    static float4x4 look_at_lh(const float3& source, const float3& target, const float3& up);
    static float4x4 look_at_rh(const float3& source, const float3& target, const float3& up);

    constexpr float4x4()
        : _11(1.f), _12(0.f), _13(0.f), _14(0.f)
        , _21(0.f), _22(1.f), _23(0.f), _24(0.f)
        , _31(0.f), _32(0.f), _33(1.f), _34(0.f)
        , _41(0.f), _42(0.f), _43(0.f), _44(1.f)
    {
    }

    constexpr float4x4(float _11, float _12, float _13, float _14,
                       float _21, float _22, float _23, float _24,
                       float _31, float _32, float _33, float _34,
                       float _41, float _42, float _43, float _44)
        : _11(_11), _12(_12), _13(_13), _14(_14)
        , _21(_21), _22(_22), _23(_23), _24(_24)
        , _31(_31), _32(_32), _33(_33), _34(_34)
        , _41(_41), _42(_42), _43(_43), _44(_44)
    {
    }

    explicit constexpr float4x4(const float2x2& value)
        : _11(value._11), _12(value._12), _13(0.f), _14(0.f)
        , _21(value._21), _22(value._22), _23(0.f), _24(0.f)
        , _31(0.f),       _32(0.f),       _33(1.f), _34(0.f)
        , _41(0.f),       _42(0.f),       _43(0.f), _44(1.f)
    {
    }

    explicit constexpr float4x4(const float3x3& value)
        : _11(value._11), _12(value._12), _13(value._13), _14(0.f)
        , _21(value._21), _22(value._22), _23(value._23), _24(0.f)
        , _31(value._31), _32(value._32), _33(value._33), _34(0.f)
        , _41(0.f),       _42(0.f),       _43(0.f),       _44(1.f)
    {
    }

    constexpr float4x4(const float4& _r0, const float4& _r1, const float4& _r2, const float4& _r3)
        : _r0(_r0)
        , _r1(_r1)
        , _r2(_r2)
        , _r3(_r3)
    {
    }

    explicit float4x4(const quaternion& quaternion);
    explicit float4x4(const transform& value);

    constexpr float* begin() {
        return cells;
    }

    constexpr const float* begin() const {
        return cells;
    }

    constexpr float* end() {
        return cells + 16;
    }

    constexpr const float* end() const {
        return cells + 16;
    }

    constexpr float4x4 operator+(const float4x4& rhs) const {
        return float4x4(_11 + rhs._11, _12 + rhs._12, _13 + rhs._13, _14 + rhs._14,
                        _21 + rhs._21, _22 + rhs._22, _23 + rhs._23, _24 + rhs._24,
                        _31 + rhs._31, _32 + rhs._32, _33 + rhs._33, _34 + rhs._34,
                        _41 + rhs._41, _42 + rhs._42, _43 + rhs._43, _44 + rhs._44);
    }

    constexpr float4x4 operator-(const float4x4& rhs) const {
        return float4x4(_11 - rhs._11, _12 - rhs._12, _13 - rhs._13, _14 - rhs._14,
                        _21 - rhs._21, _22 - rhs._22, _23 - rhs._23, _24 - rhs._24,
                        _31 - rhs._31, _32 - rhs._32, _33 - rhs._33, _34 - rhs._34,
                        _41 - rhs._41, _42 - rhs._42, _43 - rhs._43, _44 - rhs._44);
    }

    constexpr float4x4 operator*(const float4x4& rhs) const {
        return float4x4(_11 * rhs._11 + _12 * rhs._21 + _13 * rhs._31 + _14 * rhs._41,
                        _11 * rhs._12 + _12 * rhs._22 + _13 * rhs._32 + _14 * rhs._42,
                        _11 * rhs._13 + _12 * rhs._23 + _13 * rhs._33 + _14 * rhs._43,
                        _11 * rhs._14 + _12 * rhs._24 + _13 * rhs._34 + _14 * rhs._44,
                        _21 * rhs._11 + _22 * rhs._21 + _23 * rhs._31 + _24 * rhs._41,
                        _21 * rhs._12 + _22 * rhs._22 + _23 * rhs._32 + _24 * rhs._42,
                        _21 * rhs._13 + _22 * rhs._23 + _23 * rhs._33 + _24 * rhs._43,
                        _21 * rhs._14 + _22 * rhs._24 + _23 * rhs._34 + _24 * rhs._44,
                        _31 * rhs._11 + _32 * rhs._21 + _33 * rhs._31 + _34 * rhs._41,
                        _31 * rhs._12 + _32 * rhs._22 + _33 * rhs._32 + _34 * rhs._42,
                        _31 * rhs._13 + _32 * rhs._23 + _33 * rhs._33 + _34 * rhs._43,
                        _31 * rhs._14 + _32 * rhs._24 + _33 * rhs._34 + _34 * rhs._44,
                        _41 * rhs._11 + _42 * rhs._21 + _43 * rhs._31 + _44 * rhs._41,
                        _41 * rhs._12 + _42 * rhs._22 + _43 * rhs._32 + _44 * rhs._42,
                        _41 * rhs._13 + _42 * rhs._23 + _43 * rhs._33 + _44 * rhs._43,
                        _41 * rhs._14 + _42 * rhs._24 + _43 * rhs._34 + _44 * rhs._44);
    }

    constexpr float4x4& operator+=(const float4x4& value) {
        _11 += value._11;
        _12 += value._12;
        _13 += value._13;
        _14 += value._14;
        _21 += value._21;
        _22 += value._22;
        _23 += value._23;
        _24 += value._24;
        _31 += value._31;
        _32 += value._32;
        _33 += value._33;
        _34 += value._34;
        _41 += value._41;
        _42 += value._42;
        _43 += value._43;
        _44 += value._44;

        return *this;
    }

    constexpr float4x4& operator-=(const float4x4& value) {
        _11 -= value._11;
        _12 -= value._12;
        _13 -= value._13;
        _14 -= value._14;
        _21 -= value._21;
        _22 -= value._22;
        _23 -= value._23;
        _24 -= value._24;
        _31 -= value._31;
        _32 -= value._32;
        _33 -= value._33;
        _34 -= value._34;
        _41 -= value._41;
        _42 -= value._42;
        _43 -= value._43;
        _44 -= value._44;

        return *this;
    }

    constexpr float4x4& operator*=(const float4x4& value) {
        float4x4 temp(
            _11 * value._11 + _12 * value._21 + _13 * value._31 + _14 * value._41,
            _11 * value._12 + _12 * value._22 + _13 * value._32 + _14 * value._42,
            _11 * value._13 + _12 * value._23 + _13 * value._33 + _14 * value._43,
            _11 * value._14 + _12 * value._24 + _13 * value._34 + _14 * value._44,
            _21 * value._11 + _22 * value._21 + _23 * value._31 + _24 * value._41,
            _21 * value._12 + _22 * value._22 + _23 * value._32 + _24 * value._42,
            _21 * value._13 + _22 * value._23 + _23 * value._33 + _24 * value._43,
            _21 * value._14 + _22 * value._24 + _23 * value._34 + _24 * value._44,
            _31 * value._11 + _32 * value._21 + _33 * value._31 + _34 * value._41,
            _31 * value._12 + _32 * value._22 + _33 * value._32 + _34 * value._42,
            _31 * value._13 + _32 * value._23 + _33 * value._33 + _34 * value._43,
            _31 * value._14 + _32 * value._24 + _33 * value._34 + _34 * value._44,
            _41 * value._11 + _42 * value._21 + _43 * value._31 + _44 * value._41,
            _41 * value._12 + _42 * value._22 + _43 * value._32 + _44 * value._42,
            _41 * value._13 + _42 * value._23 + _43 * value._33 + _44 * value._43,
            _41 * value._14 + _42 * value._24 + _43 * value._34 + _44 * value._44
        );

        return *this = temp;
    }

    constexpr float4x4& operator*=(float value) {
        _11 *= value;
        _12 *= value;
        _13 *= value;
        _14 *= value;
        _21 *= value;
        _22 *= value;
        _23 *= value;
        _24 *= value;
        _31 *= value;
        _32 *= value;
        _33 *= value;
        _34 *= value;
        _41 *= value;
        _42 *= value;
        _43 *= value;
        _44 *= value;

        return *this;
    }

    constexpr float4x4& operator/=(float value) {
        _11 /= value;
        _12 /= value;
        _13 /= value;
        _14 /= value;
        _21 /= value;
        _22 /= value;
        _23 /= value;
        _24 /= value;
        _31 /= value;
        _32 /= value;
        _33 /= value;
        _34 /= value;
        _41 /= value;
        _42 /= value;
        _43 /= value;
        _44 /= value;

        return *this;
    }

    constexpr float4& operator[](size_t index) {
        return rows[index];
    }

    constexpr const float4& operator[](size_t index) const {
        return rows[index];
    }

    constexpr float4x4 operator-() const {
        return float4x4(-_11, -_12, -_13, -_14,
                        -_21, -_22, -_23, -_24,
                        -_31, -_32, -_33, -_34,
                        -_41, -_42, -_43, -_44);
    }

    constexpr float4x4 operator*(float rhs) const {
        return float4x4(_11 * rhs, _12 * rhs, _13 * rhs, _14 * rhs,
                        _21 * rhs, _22 * rhs, _23 * rhs, _24 * rhs,
                        _31 * rhs, _32 * rhs, _33 * rhs, _34 * rhs,
                        _41 * rhs, _42 * rhs, _43 * rhs, _44 * rhs);
    }

    constexpr float4 operator*(const float4& rhs) const {
        return float4(_11 * rhs.x + _12 * rhs.y + _13 * rhs.z + _14 * rhs.w,
                      _21 * rhs.x + _22 * rhs.y + _23 * rhs.z + _24 * rhs.w,
                      _31 * rhs.x + _32 * rhs.y + _33 * rhs.z + _34 * rhs.w,
                      _41 * rhs.x + _42 * rhs.y + _43 * rhs.z + _44 * rhs.w);
    }

    constexpr float3 operator*(const float3& rhs)  const{
        return float3(_11 * rhs.x + _12 * rhs.y + _13 * rhs.z,
                      _21 * rhs.x + _22 * rhs.y + _23 * rhs.z,
                      _31 * rhs.x + _32 * rhs.y + _33 * rhs.z);
    }

    constexpr bool operator==(const float4x4& value) const {
        return _11 == value._11 && _12 == value._12 && _13 == value._13 && _14 == value._14 &&
               _21 == value._21 && _22 == value._22 && _23 == value._23 && _24 == value._24 &&
               _31 == value._31 && _32 == value._32 && _33 == value._33 && _34 == value._34 &&
               _41 == value._41 && _42 == value._42 && _43 == value._43 && _44 == value._44;
    }

    constexpr bool operator!=(const float4x4& value) const {
        return _11 != value._11 || _12 != value._12 || _13 != value._13 || _14 != value._14 ||
               _21 != value._21 || _22 != value._22 || _23 != value._23 || _24 != value._24 ||
               _31 != value._31 || _32 != value._32 || _33 != value._33 || _34 != value._34 ||
               _41 != value._41 || _42 != value._42 || _43 != value._43 || _44 != value._44;
    }

    constexpr float* operator&() {
        return cells;
    }

    constexpr const float* operator&() const {
        return cells;
    }

    friend constexpr float4x4 operator*(float lhs, const float4x4& rhs) {
        return rhs * lhs;
    }

    friend constexpr float4 operator*(const float4& lhs, const float4x4& rhs) {
        return float4(lhs.x * rhs._11 + lhs.y * rhs._21 + lhs.z * rhs._31 + lhs.w * rhs._41,
                      lhs.x * rhs._12 + lhs.y * rhs._22 + lhs.z * rhs._32 + lhs.w * rhs._42,
                      lhs.x * rhs._13 + lhs.y * rhs._23 + lhs.z * rhs._33 + lhs.w * rhs._43,
                      lhs.x * rhs._14 + lhs.y * rhs._24 + lhs.z * rhs._34 + lhs.w * rhs._44);
    }

    friend constexpr float3 operator*(const float3& lhs, const float4x4& rhs) {
        return float3(lhs.x * rhs._11 + lhs.y * rhs._21 + lhs.z * rhs._31,
                      lhs.x * rhs._12 + lhs.y * rhs._22 + lhs.z * rhs._32,
                      lhs.x * rhs._13 + lhs.y * rhs._23 + lhs.z * rhs._33);
    }

    union {
        struct {
            float _m00, _m01, _m02, _m03;
            float _m10, _m11, _m12, _m13;
            float _m20, _m21, _m22, _m23;
            float _m30, _m31, _m32, _m33;
        };

        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };

        struct {
            float4 _r0;
            float4 _r1;
            float4 _r2;
            float4 _r3;
        };

        float4 rows[4];
        float cells[16];
    };
};

constexpr float4x4 transpose(const float4x4& value) {
    return float4x4(value._11, value._21, value._31, value._41,
                    value._12, value._22, value._32, value._42,
                    value._13, value._23, value._33, value._43,
                    value._14, value._24, value._34, value._44);
}

constexpr float4x4 operator/(const float4x4& lhs, float rhs) {
    return float4x4(lhs._11 / rhs, lhs._12 / rhs, lhs._13 / rhs, lhs._14 / rhs,
                    lhs._21 / rhs, lhs._22 / rhs, lhs._23 / rhs, lhs._24 / rhs,
                    lhs._31 / rhs, lhs._32 / rhs, lhs._33 / rhs, lhs._34 / rhs,
                    lhs._41 / rhs, lhs._42 / rhs, lhs._43 / rhs, lhs._44 / rhs);
}

constexpr float3 point_transform(const float3& point, const float4x4& transform) {
    float3 result(point.x * transform._11 + point.y * transform._21 + point.z * transform._31 + transform._41,
                  point.x * transform._12 + point.y * transform._22 + point.z * transform._32 + transform._42,
                  point.x * transform._13 + point.y * transform._23 + point.z * transform._33 + transform._43);
    return result / (point.x * transform._14 + point.y * transform._24 + point.z * transform._34 + transform._44);
}

constexpr float3 normal_transform(const float3& normal, const float4x4& inverse_transform) {
    return float3(
        normal.x * inverse_transform._11 + normal.y * inverse_transform._12 + normal.z * inverse_transform._13,
        normal.x * inverse_transform._21 + normal.y * inverse_transform._22 + normal.z * inverse_transform._23,
        normal.x * inverse_transform._31 + normal.y * inverse_transform._32 + normal.z * inverse_transform._33
    );
}

constexpr float4x4 inverse(const float4x4& value) {
    float _1 = value._33  * value._44;
    float _2 = value._43  * value._34;
    float _3 = value._23  * value._44;
    float _4 = value._43  * value._24;
    float _5 = value._23  * value._34;
    float _6 = value._33  * value._24;
    float _7 = value._13  * value._44;
    float _8 = value._43  * value._14;
    float _9 = value._13  * value._34;
    float _10 = value._33 * value._14;
    float _11 = value._13 * value._24;
    float _12 = value._23 * value._14;
    float _13 = value._31 * value._42;
    float _14 = value._41 * value._32;
    float _15 = value._21 * value._42;
    float _16 = value._41 * value._22;
    float _17 = value._21 * value._32;
    float _18 = value._31 * value._22;
    float _19 = value._11 * value._42;
    float _20 = value._41 * value._12;
    float _21 = value._11 * value._32;
    float _22 = value._31 * value._12;
    float _23 = value._11 * value._22;
    float _24 = value._21 * value._12;

    float4x4 result(
        (_1  * value._22 + _4  * value._32 + _5  * value._42) - (_2  * value._22 + _3  * value._32 + _6  * value._42),
        (_2  * value._12 + _7  * value._32 + _10 * value._42) - (_1  * value._12 + _8  * value._32 + _9  * value._42),
        (_3  * value._12 + _8  * value._22 + _11 * value._42) - (_4  * value._12 + _7  * value._22 + _12 * value._42),
        (_6  * value._12 + _9  * value._22 + _12 * value._32) - (_5  * value._12 + _10 * value._22 + _11 * value._32),
        (_2  * value._21 + _3  * value._31 + _6  * value._41) - (_1  * value._21 + _4  * value._31 + _5  * value._41),
        (_1  * value._11 + _8  * value._31 + _9  * value._41) - (_2  * value._11 + _7  * value._31 + _10 * value._41),
        (_4  * value._11 + _7  * value._21 + _12 * value._41) - (_3  * value._11 + _8  * value._21 + _11 * value._41),
        (_5  * value._11 + _10 * value._21 + _11 * value._31) - (_6  * value._11 + _9  * value._21 + _12 * value._31),
        (_13 * value._24 + _16 * value._34 + _17 * value._44) - (_14 * value._24 + _15 * value._34 + _18 * value._44),
        (_14 * value._14 + _19 * value._34 + _22 * value._44) - (_13 * value._14 + _20 * value._34 + _21 * value._44),
        (_15 * value._14 + _20 * value._24 + _23 * value._44) - (_16 * value._14 + _19 * value._24 + _24 * value._44),
        (_18 * value._14 + _21 * value._24 + _24 * value._34) - (_17 * value._14 + _22 * value._24 + _23 * value._34),
        (_15 * value._33 + _18 * value._43 + _14 * value._23) - (_17 * value._43 + _13 * value._23 + _16 * value._33),
        (_21 * value._43 + _13 * value._13 + _20 * value._33) - (_19 * value._33 + _22 * value._43 + _14 * value._13),
        (_19 * value._23 + _24 * value._43 + _16 * value._13) - (_23 * value._43 + _15 * value._13 + _20 * value._23),
        (_23 * value._33 + _17 * value._13 + _22 * value._23) - (_21 * value._23 + _24 * value._33 + _18 * value._13)
    );

    float det = value._11 * result._11 + value._21 * result._12 + value._31 * result._13 + value._41 * result._14;
    if (equal(det, 0.f)) {
        return float4x4();
    }

    float multiplier = 1.f / det;
    result._11 *= multiplier;
    result._12 *= multiplier;
    result._13 *= multiplier;
    result._14 *= multiplier;
    result._21 *= multiplier;
    result._22 *= multiplier;
    result._23 *= multiplier;
    result._24 *= multiplier;
    result._31 *= multiplier;
    result._32 *= multiplier;
    result._33 *= multiplier;
    result._34 *= multiplier;
    result._41 *= multiplier;
    result._42 *= multiplier;
    result._43 *= multiplier;
    result._44 *= multiplier;

    return result;
}

constexpr bool equal(const float4x4& lhs, const float4x4& rhs, float epsilon = EPSILON) {
    return equal(lhs._11, rhs._11, epsilon) &&
           equal(lhs._12, rhs._12, epsilon) &&
           equal(lhs._13, rhs._13, epsilon) &&
           equal(lhs._14, rhs._14, epsilon) &&
           equal(lhs._21, rhs._21, epsilon) &&
           equal(lhs._22, rhs._22, epsilon) &&
           equal(lhs._23, rhs._23, epsilon) &&
           equal(lhs._24, rhs._24, epsilon) &&
           equal(lhs._31, rhs._31, epsilon) &&
           equal(lhs._32, rhs._32, epsilon) &&
           equal(lhs._33, rhs._33, epsilon) &&
           equal(lhs._34, rhs._34, epsilon) &&
           equal(lhs._41, rhs._41, epsilon) &&
           equal(lhs._42, rhs._42, epsilon) &&
           equal(lhs._43, rhs._43, epsilon) &&
           equal(lhs._44, rhs._44, epsilon);
}

inline bool isfinite(const float4x4& value) {
    return std::isfinite(value._11) && std::isfinite(value._12) && std::isfinite(value._13) && std::isfinite(value._14) &&
           std::isfinite(value._21) && std::isfinite(value._22) && std::isfinite(value._23) && std::isfinite(value._24) && 
           std::isfinite(value._31) && std::isfinite(value._32) && std::isfinite(value._33) && std::isfinite(value._34) && 
           std::isfinite(value._41) && std::isfinite(value._42) && std::isfinite(value._43) && std::isfinite(value._44);
}

} // namespace kw
