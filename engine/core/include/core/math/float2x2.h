#pragma once

#include "core/math/float2.h"

namespace kw {

class float2x2 {
public:
    constexpr float2x2()
        : _11(1.f), _12(0.f)
        , _21(0.f), _22(1.f)
    {
    }

    constexpr float2x2(float _11, float _12,
                       float _21, float _22)
        : _11(_11), _12(_12)
        , _21(_21), _22(_22)
    {
    }

    constexpr float2x2(const float2& _r0, const float2& _r1) 
        : _r0(_r0)
        , _r1(_r1)
    {
    }

    constexpr float* begin() {
        return cells;
    }

    constexpr const float* begin() const {
        return cells;
    }

    constexpr float* end() {
        return cells + 4;
    }

    constexpr const float* end() const {
        return cells + 4;
    }

    constexpr float2x2 operator+(const float2x2& rhs) const {
        return float2x2(_11 + rhs._11, _12 + rhs._12, 
                        _21 + rhs._21, _22 + rhs._22);
    }

    constexpr float2x2 operator-(const float2x2& rhs) const {
        return float2x2(_11 - rhs._11, _12 - rhs._12, 
                        _21 - rhs._21, _22 - rhs._22);
    }

    constexpr float2x2 operator*(const float2x2& rhs) const {
        return float2x2(_11 * rhs._11 + _12 * rhs._21,
                        _11 * rhs._12 + _12 * rhs._22,
                        _21 * rhs._11 + _22 * rhs._21,
                        _21 * rhs._12 + _22 * rhs._22);
    }

    constexpr float2x2& operator+=(const float2x2& value) {
        _11 += value._11;
        _12 += value._12;
        _21 += value._21;
        _22 += value._22;

        return *this;
    }

    constexpr float2x2& operator-=(const float2x2& value) {
        _11 -= value._11;
        _12 -= value._12;
        _21 -= value._21;
        _22 -= value._22;

        return *this;
    }

    constexpr float2x2& operator*=(const float2x2& value) {
        float2x2 temp(
            _11 * value._11 + _12 * value._21,
            _11 * value._12 + _12 * value._22,
            _21 * value._11 + _22 * value._21,
            _21 * value._12 + _22 * value._22
        );

        return *this = temp;
    }

    constexpr float2x2& operator*=(float value) {
        _11 *= value;
        _12 *= value;
        _21 *= value;
        _22 *= value;

        return *this;
    }

    constexpr float2x2& operator/=(float value) {
        _11 /= value;
        _12 /= value;
        _21 /= value;
        _22 /= value;

        return *this;
    }

    constexpr float2& operator[](size_t index) {
        return rows[index];
    }

    constexpr const float2& operator[](size_t index) const {
        return rows[index];
    }

    constexpr float2x2 operator-() const {
        return float2x2(-_11, -_12,
                        -_21, -_22);
    }

    constexpr float2 operator*(const float2& rhs) const {
        return float2(_11 * rhs.x + _12 * rhs.y,
                      _21 * rhs.x + _22 * rhs.y);
    }

    constexpr float2x2 operator*(float rhs) const {
        return float2x2(_11 * rhs, _12 * rhs, _21 * rhs, _22 * rhs);
    }

    constexpr float2x2 operator/(float rhs) const {
        return float2x2(_11 / rhs, _12 / rhs, _21 / rhs, _22 / rhs);
    }

    constexpr bool operator==(const float2x2& value) const {
        return _11 == value._11 && _12 == value._12 &&
               _21 == value._21 && _22 == value._22;
    }

    constexpr bool operator!=(const float2x2& value) const {
        return _11 != value._11 || _12 != value._12 ||
               _21 != value._21 || _22 != value._22;
    }

    constexpr float* operator&() {
        return cells;
    }

    constexpr const float* operator&() const {
        return cells;
    }

    friend constexpr float2 operator*(const float2& lhs, const float2x2& rhs) {
        return float2(lhs.x * rhs._11 + lhs.y * rhs._21,
                      lhs.x * rhs._12 + lhs.y * rhs._22);
    }

    friend constexpr float2x2 operator*(float lhs, const float2x2& rhs) {
        return rhs * lhs;
    }

    union {
        struct {
            float _m00, _m01;
            float _m10, _m11;
        };

        struct {
            float _11, _12;
            float _21, _22;
        };

        struct {
            float2 _r0;
            float2 _r1;
        };

        float2 rows[2];
        float cells[4];
    };
};

constexpr float2x2 transpose(const float2x2& value) {
    return float2x2(value._11, value._21,
                    value._12, value._22);
}

constexpr float2x2 inverse(const float2x2& value) {
    float det = value._11 * value._22 - value._12 * value._21;

    if (equal(det, 0.f)) {
        return float2x2();
    }

    float multiplier = 1.f / det;
    return float2x2( value._22 * multiplier, -value._12 * multiplier,
                    -value._21 * multiplier,  value._11 * multiplier);
}

constexpr bool equal(const float2x2& lhs, const float2x2& rhs, float epsilon = EPSILON) {
    return equal(lhs._11, rhs._11, epsilon) && equal(lhs._12, rhs._12, epsilon) &&
           equal(lhs._21, rhs._21, epsilon) && equal(lhs._22, rhs._22, epsilon);
}

inline bool isfinite(const float2x2& value) {
    return std::isfinite(value._11) && std::isfinite(value._12) &&
           std::isfinite(value._21) && std::isfinite(value._22);
}

} // namespace kw
