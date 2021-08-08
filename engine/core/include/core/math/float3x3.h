#pragma once

#include "core/math/float2x2.h"
#include "core/math/float3.h"

namespace kw {

class quaternion;

class float3x3 {
public:
    constexpr float3x3()
        : _11(1.f), _12(0.f), _13(0.f)
        , _21(0.f), _22(1.f), _23(0.f)
        , _31(0.f), _32(0.f), _33(1.f)
    {
    }

    constexpr float3x3(float _11, float _12, float _13,
                       float _21, float _22, float _23,
                       float _31, float _32, float _33)
        : _11(_11), _12(_12), _13(_13)
        , _21(_21), _22(_22), _23(_23)
        , _31(_31), _32(_32), _33(_33)
    {
    }

    explicit constexpr float3x3(const float2x2& value)
        : _11(value._11), _12(value._12), _13(0.f)
        , _21(value._21), _22(value._22), _23(0.f)
        , _31(0.f),       _32(0.f),       _33(1.f)
    {
    }

    constexpr float3x3(const float3& _r0, const float3& _r1, const float3& _r2)
        : _r0(_r0)
        , _r1(_r1)
        , _r2(_r2)
    {
    }

    explicit float3x3(const quaternion& quaternion);

    constexpr float* begin() {
        return cells;
    }

    constexpr const float* begin() const {
        return cells;
    }

    constexpr float* end() {
        return cells + 9;
    }

    constexpr const float* end() const {
        return cells + 9;
    }

    constexpr float3x3 operator+(const float3x3& rhs) const {
        return float3x3(_11 + rhs._11, _12 + rhs._12, _13 + rhs._13,
                        _21 + rhs._21, _22 + rhs._22, _23 + rhs._23,
                        _31 + rhs._31, _32 + rhs._32, _33 + rhs._33);
    }

    constexpr float3x3 operator-(const float3x3& rhs) const {
        return float3x3(_11 - rhs._11, _12 - rhs._12, _13 - rhs._13,
                        _21 - rhs._21, _22 - rhs._22, _23 - rhs._23,
                        _31 - rhs._31, _32 - rhs._32, _33 - rhs._33);
    }

    constexpr float3x3 operator*(const float3x3& rhs) const {
        return float3x3(_11 * rhs._11 + _12 * rhs._21 + _13 * rhs._31,
                        _11 * rhs._12 + _12 * rhs._22 + _13 * rhs._32,
                        _11 * rhs._13 + _12 * rhs._23 + _13 * rhs._33,
                        _21 * rhs._11 + _22 * rhs._21 + _23 * rhs._31,
                        _21 * rhs._12 + _22 * rhs._22 + _23 * rhs._32,
                        _21 * rhs._13 + _22 * rhs._23 + _23 * rhs._33,
                        _31 * rhs._11 + _32 * rhs._21 + _33 * rhs._31,
                        _31 * rhs._12 + _32 * rhs._22 + _33 * rhs._32,
                        _31 * rhs._13 + _32 * rhs._23 + _33 * rhs._33);
    }

    constexpr float3x3& operator+=(const float3x3& value) {
        _11 += value._11;
        _12 += value._12;
        _13 += value._13;
        _21 += value._21;
        _22 += value._22;
        _23 += value._23;
        _31 += value._31;
        _32 += value._32;
        _33 += value._33;

        return *this;
    }

    constexpr float3x3& operator-=(const float3x3& value) {
        _11 -= value._11;
        _12 -= value._12;
        _13 -= value._13;
        _21 -= value._21;
        _22 -= value._22;
        _23 -= value._23;
        _31 -= value._31;
        _32 -= value._32;
        _33 -= value._33;

        return *this;
    }

    constexpr float3x3& operator*=(const float3x3& value) {
        float3x3 temp(
            _11 * value._11 + _12 * value._21 + _13 * value._31,
            _11 * value._12 + _12 * value._22 + _13 * value._32,
            _11 * value._13 + _12 * value._23 + _13 * value._33,
            _21 * value._11 + _22 * value._21 + _23 * value._31,
            _21 * value._12 + _22 * value._22 + _23 * value._32,
            _21 * value._13 + _22 * value._23 + _23 * value._33,
            _31 * value._11 + _32 * value._21 + _33 * value._31,
            _31 * value._12 + _32 * value._22 + _33 * value._32,
            _31 * value._13 + _32 * value._23 + _33 * value._33
        );

        return *this = temp;
    }

    constexpr float3x3& operator*=(float value) {
        _11 *= value;
        _12 *= value;
        _13 *= value;
        _21 *= value;
        _22 *= value;
        _23 *= value;
        _31 *= value;
        _32 *= value;
        _33 *= value;

        return *this;
    }

    constexpr float3x3& operator/=(float value) {
        _11 /= value;
        _12 /= value;
        _13 /= value;
        _21 /= value;
        _22 /= value;
        _23 /= value;
        _31 /= value;
        _32 /= value;
        _33 /= value;

        return *this;
    }

    constexpr float3& operator[](size_t index) {
        return rows[index];
    }

    constexpr const float3& operator[](size_t index) const {
        return rows[index];
    }

    constexpr float3x3 operator-() const {
        return float3x3(-_11, -_12, -_13,
                        -_21, -_22, -_23,
                        -_31, -_32, -_33);
    }

    constexpr float3x3 operator*(float rhs) const {
        return float3x3(_11 * rhs, _12 * rhs, _13 * rhs,
                        _21 * rhs, _22 * rhs, _23 * rhs,
                        _31 * rhs, _32 * rhs, _33 * rhs);
    }

    constexpr float3 operator*(const float3& rhs) const {
        return float3(_11 * rhs.x + _12 * rhs.y + _13 * rhs.z,
                      _21 * rhs.x + _22 * rhs.y + _23 * rhs.z,
                      _31 * rhs.x + _32 * rhs.y + _33 * rhs.z);
    }

    constexpr float3x3 operator/(float rhs) const {
        return float3x3(_11 / rhs, _12 / rhs, _13 / rhs,
                        _21 / rhs, _22 / rhs, _23 / rhs,
                        _31 / rhs, _32 / rhs, _33 / rhs);
    }

    constexpr bool operator==(const float3x3& value) const {
        return _11 == value._11 && _12 == value._12 && _13 == value._13 &&
               _21 == value._21 && _22 == value._22 && _23 == value._23 &&
               _31 == value._31 && _32 == value._32 && _33 == value._33;
    }

    constexpr bool operator!=(const float3x3& value) const {
        return _11 != value._11 || _12 != value._12 || _13 != value._13 ||
               _21 != value._21 || _22 != value._22 || _23 != value._23 ||
               _31 != value._31 || _32 != value._32 || _33 != value._33;
    }

    constexpr float* operator&() {
        return cells;
    }

    constexpr const float* operator&() const {
        return cells;
    }

    friend constexpr float3x3 operator*(float lhs, const float3x3& rhs) {
        return rhs * lhs;
    }

    friend constexpr float3 operator*(const float3& lhs, const float3x3& rhs) {
        return float3(lhs.x * rhs._11 + lhs.y * rhs._21 + lhs.z * rhs._31,
                      lhs.x * rhs._12 + lhs.y * rhs._22 + lhs.z * rhs._32,
                      lhs.x * rhs._13 + lhs.y * rhs._23 + lhs.z * rhs._33);
    }

    union {
        struct {
            float _m00, _m01, _m02;
            float _m10, _m11, _m12;
            float _m20, _m21, _m22;
        };

        struct {
            float _11, _12, _13;
            float _21, _22, _23;
            float _31, _32, _33;
        };

        struct {
            float3 _r0;
            float3 _r1;
            float3 _r2;
        };

        float3 rows[3];
        float cells[9];
    };
};

constexpr float3x3 transpose(const float3x3& value) {
    return float3x3(value._11, value._21, value._31,
                    value._12, value._22, value._32,
                    value._13, value._23, value._33);
}

constexpr float3x3 inverse(const float3x3& value) {
    float3x3 result(value._33 * value._22 - value._23 * value._32,
                    value._13 * value._32 - value._33 * value._12,
                    value._23 * value._12 - value._13 * value._22,
                    value._23 * value._31 - value._33 * value._21,
                    value._33 * value._11 - value._13 * value._31,
                    value._13 * value._21 - value._23 * value._11,
                    value._21 * value._32 - value._31 * value._22,
                    value._31 * value._12 - value._11 * value._32,
                    value._11 * value._22 - value._21 * value._12);

    float det = value._11 * result._11 + value._21 * result._12 + value._31 * result._13;
    if (equal(det, 0.f)) {
        return float3x3();
    }

    float factor = 1.f / det;
    result._11 *= factor;
    result._12 *= factor;
    result._13 *= factor;
    result._21 *= factor;
    result._22 *= factor;
    result._23 *= factor;
    result._31 *= factor;
    result._32 *= factor;
    result._33 *= factor;

    return result;
}

constexpr bool equal(const float3x3& lhs, const float3x3& rhs, float epsilon = EPSILON) {
    return equal(lhs._11, rhs._11, epsilon) &&
           equal(lhs._12, rhs._12, epsilon) &&
           equal(lhs._13, rhs._13, epsilon) &&
           equal(lhs._21, rhs._21, epsilon) &&
           equal(lhs._22, rhs._22, epsilon) &&
           equal(lhs._23, rhs._23, epsilon) &&
           equal(lhs._31, rhs._31, epsilon) &&
           equal(lhs._32, rhs._32, epsilon) &&
           equal(lhs._33, rhs._33, epsilon);
}

inline bool isfinite(const float3x3& value) {
    return std::isfinite(value._11) && std::isfinite(value._12) && std::isfinite(value._13) &&
           std::isfinite(value._21) && std::isfinite(value._22) && std::isfinite(value._23) &&
           std::isfinite(value._31) && std::isfinite(value._32) && std::isfinite(value._33);
}

} // namespace kw
