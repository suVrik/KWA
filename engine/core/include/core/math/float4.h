#pragma once

#include "core/math/float3.h"

namespace kw {

class quaternion;

class float4 {
public:
    constexpr float4()
        : x(0.f), y(0.f), z(0.f), w(0.f)
    {
    }

    explicit constexpr float4(float all)
        : x(all), y(all), z(all), w(all)
    {
    }
    
    constexpr float4(float x, float y, float z, float w)
        : x(x), y(y), z(z), w(w)
    {
    }

    constexpr float4(const float2& value, float z, float w)
        : x(value.x), y(value.y), z(z), w(w)
    {
    }

    constexpr float4(const float3& value, float w)
        : x(value.x), y(value.y), z(value.z), w(w)
    {
    }

    explicit float4(const quaternion& value);

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

    constexpr float4 operator+(const float4& rhs) const {
        return float4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
    }

    constexpr float4 operator-(const float4& rhs) const {
        return float4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
    }

    constexpr float4 operator*(const float4& rhs) const {
        return float4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
    }

    constexpr float4 operator/(const float4& rhs) const {
        return float4(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w);
    }

    constexpr float4& operator+=(const float4& value) {
        x += value.x;
        y += value.y;
        z += value.z;
        w += value.w;

        return *this;
    }

    constexpr float4& operator-=(const float4& value) {
        x -= value.x;
        y -= value.y;
        z -= value.z;
        w -= value.w;

        return *this;
    }

    constexpr float4& operator*=(const float4& value) {
        x *= value.x;
        y *= value.y;
        z *= value.z;
        w *= value.w;

        return *this;
    }

    constexpr float4& operator/=(const float4& value) {
        x /= value.x;
        y /= value.y;
        z /= value.z;
        w /= value.w;

        return *this;
    }

    constexpr float4& operator*=(float value) {
        x *= value;
        y *= value;
        z *= value;
        w *= value;

        return *this;
    }

    constexpr float4& operator/=(float value) {
        x /= value;
        y /= value;
        z /= value;
        w /= value;

        return *this;
    }

    constexpr float& operator[](size_t index) {
        return data[index];
    }

    constexpr const float& operator[](size_t index) const {
        return data[index];
    }

    constexpr float4 operator-() const {
        return float4(-x, -y, -z, -w);
    }

    constexpr float4 operator*(float rhs) const {
        return float4(x * rhs, y * rhs, z * rhs, w * rhs);
    }

    constexpr float4 operator/(float rhs) const {
        return float4(x / rhs, y / rhs, z / rhs, w / rhs);
    }

    constexpr bool operator==(const float4& value) const {
        return x == value.x && y == value.y && z == value.z && w == value.w;
    }

    constexpr bool operator!=(const float4& value) const {
        return x != value.x || y != value.y || z != value.z || w != value.w;
    }

    constexpr float* operator&() {
        return data;
    }

    constexpr const float* operator&() const {
        return data;
    }

    explicit constexpr operator float2() const {
        return float2(x, y);
    }

    explicit constexpr operator float3() const {
        return float3(x, y, z);
    }

    friend constexpr float4 operator*(float lhs, const float4& rhs) {
        return rhs * lhs;
    }

    union {
        float3 xyz;

        struct {
            float x, y, z, w;
        };

        struct {
            float r, g, b, a;
        };

        float data[4];
    };
};

constexpr float dot(const float4& lhs, const float4& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
}

constexpr float4 cross(const float4& lhs, const float4& rhs) {
    return float4(lhs.y * rhs.z - rhs.y * lhs.z,
                  lhs.z * rhs.x - lhs.x * rhs.z, 
                  lhs.x * rhs.y - lhs.y * rhs.x, 
                  1.f);
}

constexpr float4 cross(const float4& lhs, const float3& rhs) {
    return float4(lhs.y * rhs.z - rhs.y * lhs.z,
                  lhs.z * rhs.x - lhs.x * rhs.z, 
                  lhs.x * rhs.y - lhs.y * rhs.x, 
                  1.f);
}

constexpr float4 cross(const float3& lhs, const float4& rhs) {
    return float4(lhs.y * rhs.z - rhs.y * lhs.z,
                  lhs.z * rhs.x - lhs.x * rhs.z, 
                  lhs.x * rhs.y - lhs.y * rhs.x, 
                  1.f);
}

constexpr float square_length(const float4& value) {
    return sqr(value.x) + sqr(value.y) + sqr(value.z) + sqr(value.w);
}

inline float length(const float4& value) {
    return std::sqrt(square_length(value));
}

constexpr float square_distance(const float4& lhs, const float4& rhs) {
    return sqr(lhs.x - rhs.x) + sqr(lhs.y - rhs.y) + sqr(lhs.z - rhs.z) + sqr(lhs.w - rhs.w);
}

inline float distance(const float4& lhs, const float4& rhs) {
    return std::sqrt(square_distance(lhs, rhs));
}

inline float4 normalize(const float4& value) {
    float multiplier = 1.f / length(value);
    return float4(value.x * multiplier, value.y * multiplier, value.z * multiplier, value.w * multiplier);
}

constexpr float4 lerp(const float4& from, const float4& to, float factor) {
    return from + (to - from) * factor;
}

constexpr float4 clamp(const float4& value, float min, float max) {
    return float4(clamp(value.x, min, max),
                  clamp(value.y, min, max),
                  clamp(value.z, min, max),
                  clamp(value.w, min, max));
}

constexpr float4 clamp(const float4& value, const float4& min, const float4& max) {
    return float4(clamp(value.x, min.x, max.x), 
                  clamp(value.y, min.y, max.y), 
                  clamp(value.z, min.z, max.z), 
                  clamp(value.w, min.w, max.w));
}

constexpr float4 reflect(const float4& vector, const float4& normal) {
    return vector - 2.f * dot(vector, normal) * normal;
}

constexpr float4 abs(const float4& vector) {
    return float4(std::abs(vector.x), std::abs(vector.y), std::abs(vector.z), std::abs(vector.w));
}

constexpr float4 min(const float4& lhs, const float4& rhs) {
    return float4(std::fmin(lhs.x, rhs.x), std::fmin(lhs.y, rhs.y), std::fmin(lhs.z, rhs.z), std::fmin(lhs.w, rhs.w));
}

constexpr float4 max(const float4& lhs, const float4& rhs) {
    return float4(std::fmax(lhs.x, rhs.x), std::fmax(lhs.y, rhs.y), std::fmax(lhs.z, rhs.z), std::fmax(lhs.w, rhs.w));
}

constexpr bool equal(const float4& lhs, const float4& rhs, float epsilon = EPSILON) {
    return equal(lhs.x, rhs.x, epsilon) &&
           equal(lhs.y, rhs.y, epsilon) &&
           equal(lhs.z, rhs.z, epsilon) &&
           equal(lhs.w, rhs.w, epsilon);
}

constexpr bool equal(const float4& lhs, float rhs, float epsilon = EPSILON) {
    return equal(lhs.x, rhs, epsilon) &&
           equal(lhs.y, rhs, epsilon) &&
           equal(lhs.z, rhs, epsilon) &&
           equal(lhs.w, rhs, epsilon);
}

inline bool isfinite(const float4& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) && std::isfinite(value.w);
}

} // namespace kw
