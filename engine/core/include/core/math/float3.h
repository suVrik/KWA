#pragma once

#include "core/math/float2.h"

namespace kw {

class float3 {
public:
    constexpr float3()
        : x(0.f), y(0.f), z(0.f)
    {
    }

    explicit constexpr float3(float all)
        : x(all), y(all), z(all)
    {
    }


    explicit constexpr float3(float x, float y, float z)
        : x(x), y(y), z(z)
    {
    }

    explicit constexpr float3(const float2& value, float z)
        : x(value.x), y(value.y), z(z)
    {
    }

    constexpr float* begin() {
        return data;
    }

    constexpr const float* begin() const {
        return data;
    }

    constexpr float* end() {
        return data + 3;
    }

    constexpr const float* end() const {
        return data + 3;
    }

    constexpr float3 operator+(const float3& rhs) const {
        return float3(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    constexpr float3 operator-(const float3& rhs) const {
        return float3(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    constexpr float3 operator*(const float3& rhs) const {
        return float3(x * rhs.x, y * rhs.y, z * rhs.z);
    }

    constexpr float3 operator/(const float3& rhs) const {
        return float3(x / rhs.x, y / rhs.y, z / rhs.z);
    }

    constexpr float3& operator+=(const float3& value) {
        x += value.x;
        y += value.y;
        z += value.z;

        return *this;
    }

    constexpr float3& operator-=(const float3& value) {
        x -= value.x;
        y -= value.y;
        z -= value.z;

        return *this;
    }

    constexpr float3& operator*=(const float3& value) {
        x *= value.x;
        y *= value.y;
        z *= value.z;

        return *this;
    }

    constexpr float3& operator/=(const float3& value) {
        x /= value.x;
        y /= value.y;
        z /= value.z;

        return *this;
    }

    constexpr float3& operator*=(float value) {
        x *= value;
        y *= value;
        z *= value;

        return *this;
    }

    constexpr float3& operator/=(float value) {
        x /= value;
        y /= value;
        z /= value;

        return *this;
    }

    constexpr float& operator[](size_t index) {
        return data[index];
    }

    constexpr const float& operator[](size_t index) const {
        return data[index];
    }

    constexpr float3 operator-() const {
        return float3(-x, -y, -z);
    }

    constexpr float3 operator*(float rhs) const {
        return float3(x * rhs, y * rhs, z * rhs);
    }

    constexpr float3 operator/(float rhs) const {
        return float3(x / rhs, y / rhs, z / rhs);
    }

    constexpr bool operator==(const float3& value) const {
        return x == value.x && y == value.y && z == value.z;
    }

    constexpr bool operator!=(const float3& value) const {
        return x != value.x || y != value.y || z != value.z;
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

    friend constexpr float3 operator*(float lhs, const float3& rhs) {
        return rhs * lhs;
    }

    union {
        struct {
            float x, y, z;
        };

        struct {
            float r, g, b;
        };

        float data[3];
    };
};

constexpr float dot(const float3& lhs, const float3& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

constexpr float3 cross(const float3& lhs, const float3& rhs) {
    return float3(lhs.y * rhs.z - lhs.z * rhs.y,
                  lhs.z * rhs.x - lhs.x * rhs.z, 
                  lhs.x * rhs.y - lhs.y * rhs.x);
}

constexpr float square_length(const float3& value) {
    return sqr(value.x) + sqr(value.y) + sqr(value.z);
}

inline float length(const float3& value) {
    return std::sqrt(square_length(value));
}

constexpr float square_distance(const float3& lhs, const float3& rhs) {
    return sqr(lhs.x - rhs.x) + sqr(lhs.y - rhs.y) + sqr(lhs.z - rhs.z);
}

inline float distance(const float3& lhs, const float3& rhs) {
    return std::sqrt(square_distance(lhs, rhs));
}

inline float3 normalize(const float3& value) {
    float multiplier = 1.f / length(value);
    return float3(value.x * multiplier, value.y * multiplier, value.z * multiplier);
}

constexpr float3 lerp(const float3& from, const float3& to, float factor) {
    return from + (to - from) * factor;
}

constexpr float3 clamp(const float3& value, float min, float max) {
    return float3(clamp(value.x, min, max), clamp(value.y, min, max), clamp(value.z, min, max));
}

constexpr float3 clamp(const float3& value, const float3& min, const float3& max) {
    return float3(clamp(value.x, min.x, max.x), clamp(value.y, min.y, max.y), clamp(value.z, min.z, max.z));
}

constexpr float3 reflect(const float3& vector, const float3& normal) {
    return vector - 2.f * dot(vector, normal) * normal;
}

constexpr float3 abs(const float3& vector) {
    return float3(std::abs(vector.x), std::abs(vector.y), std::abs(vector.z));
}

constexpr float3 min(const float3& lhs, const float3& rhs) {
    return float3(std::fmin(lhs.x, rhs.x), std::fmin(lhs.y, rhs.y), std::fmin(lhs.z, rhs.z));
}

constexpr float3 max(const float3& lhs, const float3& rhs) {
    return float3(std::fmax(lhs.x, rhs.x), std::fmax(lhs.y, rhs.y), std::fmax(lhs.z, rhs.z));
}

constexpr bool equal(const float3& lhs, const float3& rhs, float epsilon = EPSILON) {
    return equal(lhs.x, rhs.x, epsilon) && equal(lhs.y, rhs.y, epsilon) && equal(lhs.z, rhs.z, epsilon);
}

constexpr bool equal(const float3& lhs, float rhs, float epsilon = EPSILON) {
    return equal(lhs.x, rhs, epsilon) && equal(lhs.y, rhs, epsilon) && equal(lhs.z, rhs, epsilon);
}

inline bool isfinite(const float3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

} // namespace kw
