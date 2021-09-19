#pragma once

#include "core/math/float4.h"

namespace kw {

class float3x3;
class float4x4;

class quaternion {
public:
    static quaternion rotation(const float3& axis, float angle);
    
    static quaternion shortest_arc(const float3& from, const float3& to, const float3& spin_axis);
    
    constexpr quaternion() 
        : x(0.f), y(0.f), z(0.f), w(1.f)
    {
    }

    constexpr quaternion(float x, float y, float z, float w)
        : x(x), y(y), z(z), w(w)
    {
    }

    constexpr quaternion(const float3& value, float w)
        : x(value.x), y(value.y), z(value.z), w(w)
    {
    }

    explicit constexpr quaternion(const float4& value)
        : x(value.x), y(value.y), z(value.z), w(value.w)
    {
    }

    explicit quaternion(const float3x3& matrix);
    explicit quaternion(const float4x4& matrix);

    constexpr float* begin() {
        return data;
    }

    constexpr const float* begin() const {
        return data;
    }

    constexpr float* end() {
        return data + 4;
    }

    constexpr const float* end() const {
        return data + 4;
    }

    constexpr quaternion operator*(const quaternion& rhs) const {
        float a = (w + x) * (rhs.w + rhs.x);
        float b = (z - y) * (rhs.y - rhs.z);
        float c = (x - w) * (rhs.y + rhs.z);
        float d = (y + z) * (rhs.x - rhs.w);
        float e = (x + z) * (rhs.x + rhs.y);
        float f = (x - z) * (rhs.x - rhs.y);
        float g = (w + y) * (rhs.w - rhs.z);
        float h = (w - y) * (rhs.w + rhs.z);

        return quaternion( a - ( e + f + g + h) * 0.5f,
                          -c + ( e - f + g - h) * 0.5f,
                          -d + ( e - f - g + h) * 0.5f,
                           b + (-e - f + g + h) * 0.5f);
    }

    constexpr quaternion& operator*=(const quaternion& value) {
        float a = (w + x) * (value.w + value.x);
        float b = (z - y) * (value.y - value.z);
        float c = (x - w) * (value.y + value.z);
        float d = (y + z) * (value.x - value.w);
        float e = (x + z) * (value.x + value.y);
        float f = (x - z) * (value.x - value.y);
        float g = (w + y) * (value.w - value.z);
        float h = (w - y) * (value.w + value.z);

        x =  a - ( e + f + g + h) * 0.5f;
        y = -c + ( e - f + g - h) * 0.5f;
        z = -d + ( e - f - g + h) * 0.5f;
        w =  b + (-e - f + g + h) * 0.5f;

        return *this;
    }

    constexpr quaternion operator-() const {
        return quaternion(-x, -y, -z, -w);
    }

    constexpr float& operator[](size_t index) {
        return data[index];
    }

    constexpr const float& operator[](size_t index) const {
        return data[index];
    }

    constexpr bool operator==(const quaternion& value) const {
        return x == value.x && y == value.y && z == value.z && w == value.w;
    }

    constexpr bool operator!=(const quaternion& value) const {
        return x != value.x || y != value.y || z != value.z || w != value.w;
    }

    constexpr float* operator&() {
        return data;
    }

    constexpr const float* operator&() const {
        return data;
    }

    friend constexpr float3 operator*(const float3& lhs, const quaternion& rhs) {
        float3 a(rhs.x, rhs.y, rhs.z);
        float3 b(cross(a, lhs));
        float3 c(cross(a, b));
        return lhs + ((b * rhs.w) + c) * 2.f;
    }

    friend constexpr float4 operator*(const float4& lhs, const quaternion& rhs) {
        return float4(float3(lhs.x, lhs.y, lhs.z) * rhs, 0.f);
    }

    union {
        struct {
            float x, y, z, w;
        };

        float data[4];
    };
};

constexpr float square_length(const quaternion& value) {
    return sqr(value.x) + sqr(value.y) + sqr(value.z) + sqr(value.w);
}

inline float length(const quaternion& value) {
    return std::sqrt(square_length(value));
}

inline quaternion normalize(const quaternion& value) {
    float multiplier = 1.f / length(value);
    return quaternion(value.x * multiplier, value.y * multiplier, value.z * multiplier, value.w * multiplier);
}

constexpr quaternion transpose(const quaternion& value) {
    return quaternion(-value.x, -value.y, -value.z, value.w);
}

constexpr quaternion inverse(const quaternion& value) {
    float divisor = square_length(value);
    float multiplier = 1.f / divisor;
    quaternion transposed = transpose(value);

    return quaternion(transposed.x * multiplier, 
                      transposed.y * multiplier, 
                      transposed.z * multiplier, 
                      transposed.w * multiplier);
}

inline quaternion slerp(const quaternion& from, quaternion to, float factor) {
    float cos_a = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;

    if (cos_a < 0.f) {
        cos_a = -cos_a;
        to = -to;
    }

    if (cos_a > 1.f - EPSILON) {
        return normalize(quaternion(lerp(float4(from), float4(to), factor)));
    }

    float a = std::acos(cos_a);
    float b = 1.f / std::sin(a);
    float c = std::sin((1.f - factor) * a) * b;
    float d = std::sin(factor * a) * b;

    return normalize(quaternion(
        c * from.x + d * to.x,
        c * from.y + d * to.y,
        c * from.z + d * to.z,
        c * from.w + d * to.w
    ));
}

constexpr bool equal(const quaternion& lhs, const quaternion& rhs, float epsilon = EPSILON) {
    return equal(lhs.x, rhs.x, epsilon) &&
           equal(lhs.y, rhs.y, epsilon) &&
           equal(lhs.z, rhs.z, epsilon) &&
           equal(lhs.w, rhs.w, epsilon);
}

inline bool isfinite(const quaternion& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) && std::isfinite(value.w);
}

} // namespace kw
