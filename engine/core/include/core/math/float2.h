#pragma once

#include "core/math/scalar.h"

namespace kw {

class float2 {
public:
    constexpr float2()
        : x(0.f), y(0.f)
    {
    }

    explicit constexpr float2(float all)
        : x(all), y(all)
    {
    }
    
    constexpr float2(float x, float y) 
        : x(x), y(y)
    {
    }

    constexpr float* begin() {
        return data;
    }

    constexpr const float* begin() const {
        return data;
    }

    constexpr float* end() {
        return data + 2;
    }

    constexpr const float* end() const {
        return data + 2;
    }

    constexpr float2 operator+(const float2& rhs) const {
        return float2(x + rhs.x, y + rhs.y);
    }

    constexpr float2 operator-(const float2& rhs) const {
        return float2(x - rhs.x, y - rhs.y);
    }

    constexpr float2 operator*(const float2& rhs) const {
        return float2(x * rhs.x, y * rhs.y);
    }

    constexpr float2 operator/(const float2& rhs) const {
        return float2(x / rhs.x, y / rhs.y);
    }

    constexpr float2& operator+=(const float2& value) {
        x += value.x;
        y += value.y;

        return *this;
    }

    constexpr float2& operator-=(const float2& value) {
        x -= value.x;
        y -= value.y;

        return *this;
    }

    constexpr float2& operator*=(const float2& value) {
        x *= value.x;
        y *= value.y;

        return *this;
    }

    constexpr float2& operator/=(const float2& value) {
        x /= value.x;
        y /= value.y;

        return *this;
    }

    constexpr float2& operator*=(float value) {
        x *= value;
        y *= value;

        return *this;
    }

    constexpr float2& operator/=(float value) {
        x /= value;
        y /= value;

        return *this;
    }

    constexpr float& operator[](size_t index) {
        return data[index];
    }

    constexpr const float& operator[](size_t index) const {
        return data[index];
    }

    constexpr float2 operator-() const {
        return float2(-x, -y);
    }

    constexpr float2 operator*(float rhs) const {
        return float2(x * rhs, y * rhs);
    }

    constexpr float2 operator/(float rhs) const {
        return float2(x / rhs, y / rhs);
    }

    constexpr bool operator==(const float2& value) const {
        return x == value.x && y == value.y;
    }

    constexpr bool operator!=(const float2& value) const {
        return x != value.x || y != value.y;
    }

    constexpr float* operator&() {
        return data;
    }

    constexpr const float* operator&() const {
        return data;
    }

    friend constexpr float2 operator*(float lhs, const float2& rhs) {
        return rhs * lhs;
    }

    union {
        struct {
            float x, y;
        };

        struct {
            float r, g;
        };

        float data[2];
    };
};

constexpr float dot(const float2& lhs, const float2& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

constexpr float square_length(const float2& value) {
    return sqr(value.x) + sqr(value.y);
}

inline float length(const float2& value) {
    return std::sqrt(square_length(value));
}

constexpr float square_distance(const float2& lhs, const float2& rhs) {
    return sqr(lhs.x - rhs.x) + sqr(lhs.y - rhs.y);
}

inline float distance(const float2& lhs, const float2& rhs) {
    return std::sqrt(square_distance(lhs, rhs));
}

inline float2 normalize(const float2& value) {
    float multiplier = 1.f / length(value);
    return float2(value.x * multiplier, value.y * multiplier);
}

constexpr float2 lerp(const float2& from, const float2& to, float factor) {
    return from + (to - from) * factor;
}

constexpr float2 clamp(const float2& value, float min, float max) {
    return float2(clamp(value.x, min, max), clamp(value.y, min, max));
}

constexpr float2 clamp(const float2& value, const float2& min, const float2& max) {
    return float2(clamp(value.x, min.x, max.x), clamp(value.y, min.y, max.y));
}

constexpr float2 reflect(const float2& vector, const float2& normal) {
    return vector - 2.f * dot(vector, normal) * normal;
}

constexpr float2 abs(const float2& vector) {
    return float2(std::abs(vector.x), std::abs(vector.y));
}

constexpr float2 min(const float2& lhs, const float2& rhs) {
    return float2(std::fmin(lhs.x, rhs.x), std::fmin(lhs.y, rhs.y));
}

constexpr float2 max(const float2& lhs, const float2& rhs) {
    return float2(std::fmax(lhs.x, rhs.x), std::fmax(lhs.y, rhs.y));
}

constexpr bool equal(const float2& lhs, const float2& rhs, float epsilon = EPSILON) {
    return equal(lhs.x, rhs.x, epsilon) && equal(lhs.y, rhs.y, epsilon);
}

constexpr bool equal(const float2& lhs, float rhs, float epsilon = EPSILON) {
    return equal(lhs.x, rhs, epsilon) && equal(lhs.y, rhs, epsilon);
}

inline bool isfinite(const float2& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

} // namespace kw
