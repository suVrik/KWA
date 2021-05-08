#pragma once

#include <cmath>
#include <cstdint>

namespace kw {

constexpr float PI      = 3.14159265359f;
constexpr float EPSILON = 1e-6f;
constexpr float SQRT_2  = 1.41421356237f;

constexpr float sqr(float value) {
    return value * value;
}

constexpr float lerp(float from, float to, float factor) {
    return from + (to - from) * factor;
}

constexpr float clamp(float value, float min, float max) {
    return value < min ? min : (max < value ? max : value);
}

constexpr float equal(float a, float b, float epsilon = EPSILON) {
    return a - b > -epsilon && a - b < epsilon;
}

constexpr float degrees(float radians) {
    return radians / PI * 180.f;
}

constexpr float radians(float degrees) {
    return degrees / 180.f * PI;
}

template <typename T>
constexpr T align_up(T value, T alignment) {
    return (value + (alignment - 1)) & ~(alignment - 1);
}

template <typename T>
constexpr T align_down(T value, T alignment) {
    return value & ~(alignment - 1);
}

template <typename T>
constexpr bool is_pow2(T value) {
    return (value & (value - 1)) == 0;
}

constexpr uint32_t next_pow2(uint32_t value) {
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;
    return value;
}

constexpr uint32_t previous_pow2(uint32_t value) {
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value = value ^ (value >> 1);
    return value;
}

constexpr uint32_t count_bits_set(uint32_t value) {
    uint32_t result = value - ((value >> 1) & 0x55555555);
    result = ((result >> 2) & 0x33333333) + (result & 0x33333333);
    result = ((result >> 4) + result) & 0x0F0F0F0F;
    result = ((result >> 8) + result) & 0x00FF00FF;
    result = ((result >> 16) + result) & 0x0000FFFF;
    return result;
}

uint32_t log2(uint32_t value);
uint64_t log2(uint64_t value);

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
    
    explicit constexpr float2(float x, float y) 
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

constexpr bool equal(const float2& lhs, const float2& rhs, float epsilon = EPSILON) {
    return equal(lhs.x, rhs.x, epsilon) && equal(lhs.y, rhs.y, epsilon);
}

constexpr bool equal(const float2& lhs, float rhs, float epsilon = EPSILON) {
    return equal(lhs.x, rhs, epsilon) && equal(lhs.y, rhs, epsilon);
}

inline bool isfinite(const float2& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

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

constexpr bool equal(const float3& lhs, const float3& rhs, float epsilon = EPSILON) {
    return equal(lhs.x, rhs.x, epsilon) && equal(lhs.y, rhs.y, epsilon) && equal(lhs.z, rhs.z, epsilon);
}

constexpr bool equal(const float3& lhs, float rhs, float epsilon = EPSILON) {
    return equal(lhs.x, rhs, epsilon) && equal(lhs.y, rhs, epsilon) && equal(lhs.z, rhs, epsilon);
}

inline bool isfinite(const float3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

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
    
    explicit constexpr float4(float x, float y, float z, float w)
        : x(x), y(y), z(z), w(w)
    {
    }

    explicit constexpr float4(const float2& value, float z, float w)
        : x(value.x), y(value.y), z(z), w(w)
    {
    }

    explicit constexpr float4(const float3& value, float w)
        : x(value.x), y(value.y), z(value.z), w(w)
    {
    }

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

class float2x2 {
public:
    constexpr float2x2()
        : _11(1.f), _12(0.f)
        , _21(0.f), _22(1.f)
    {
    }

    explicit constexpr float2x2(float _11, float _12,
                                float _21, float _22)
        : _11(_11), _12(_12)
        , _21(_21), _22(_22)
    {
    }

    constexpr float2x2(const float2& _r0, const float2& _r1) 
        : _r0(_r0), _r1(_r1)
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

class float3x3 {
public:
    constexpr float3x3()
        : _11(1.f), _12(0.f), _13(0.f)
        , _21(0.f), _22(1.f), _23(0.f)
        , _31(0.f), _32(0.f), _33(1.f)
    {
    }

    explicit constexpr float3x3(float _11, float _12, float _13,
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
        : _r0(_r0), _r1(_r1), _r2(_r2)
    {
    }

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

    explicit constexpr float4x4(float _11, float _12, float _13, float _14,
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
        : _r0(_r0), _r1(_r1), _r2(_r2), _r3(_r3) {
    }

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

class quaternion {
public:
    static quaternion rotation(const float3& axis, float angle);
    static quaternion shortest_arc(const float3& from, const float3& to, const float3& spin_axis);
    static quaternion from_matrix(const float3x3& matrix);
    static quaternion from_matrix(const float4x4& matrix);
    static float4x4 to_matrix(const quaternion& quaternion);

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

        return quaternion( b + (-e - f + g + h) * 0.5f,
                           a - ( e + f + g + h) * 0.5f,
                          -c + ( e - f + g - h) * 0.5f,
                          -d + ( e - f - g + h) * 0.5f);
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

        x =  b + (-e - f + g + h) * 0.5f;
        y =  a - ( e + f + g + h) * 0.5f;
        z = -c + ( e - f + g - h) * 0.5f;
        w = -d + ( e - f - g + h) * 0.5f;

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

inline quaternion slerp(const quaternion& from, const quaternion& to, float factor) {
    factor = factor * 0.5f;

    float a = std::acos(from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w);
    if (a < 0.f) {
        a = -a;
    }

    float b = 1.f / std::sin(a);
    float c = std::sin((1 - factor) * a) * b;
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

class transform {
public:
    static float4x4 to_float4x4(const transform& value);

    constexpr transform()
        : translation()
        , rotation()
        , scale(1.f, 1.f, 1.f)
    {
    }

    constexpr transform(const float3& translation)
        : translation(translation)
        , rotation()
        , scale(1.f, 1.f, 1.f)
    {
    }

    constexpr transform(const float3& translation, const quaternion& rotation, const float3& scale)
        : translation(translation)
        , rotation(rotation)
        , scale(scale)
    {
    }

    constexpr float* begin() {
        return data;
    }

    constexpr const float* begin() const {
        return data;
    }

    constexpr float* end() {
        return data + 10;
    }

    constexpr const float* end() const {
        return data + 10;
    }

    constexpr transform operator*(const transform& rhs) const {
        return transform(
            translation * rhs.rotation * rhs.scale + rhs.translation,
            rotation * rhs.rotation,
            scale * rhs.scale
        );
    }

    constexpr transform& operator*=(const transform& value) {
        translation = translation * value.rotation * value.scale + value.translation;
        rotation = rotation * value.rotation,
        scale = scale * value.scale;

        return *this;
    }

    constexpr bool operator==(const transform& value) const {
        return translation == value.translation && rotation == value.rotation && scale == value.scale;
    }

    constexpr bool operator!=(const transform& value) const {
        return translation != value.translation || rotation != value.rotation || scale != value.scale;
    }

    constexpr float* operator&() {
        return data;
    }

    constexpr const float* operator&() const {
        return data;
    }

    friend constexpr float4 operator*(const float4& lhs, const transform& rhs) {
        return float4(float3(lhs.x, lhs.y, lhs.z) * rhs.scale * rhs.rotation + rhs.translation * lhs.w, 1.f);
    }

    friend constexpr float3 operator*(const float3& lhs, const transform& rhs) {
        return lhs * rhs.scale * rhs.rotation + rhs.translation;
    }

    union {
        struct {
            float3 translation;
            quaternion rotation;
            float3 scale;
        };

        float data[10];
    };
};

constexpr transform inverse(const transform& value) {
    quaternion inverse_rotation = inverse(value.rotation);
    float3 inverse_scale = float3(1.f / value.scale.x, 1.f / value.scale.y, 1.f / value.scale.z);

    return transform(
        -value.translation * inverse_scale * inverse_rotation,
        inverse_rotation,
        inverse_scale
    );
}

constexpr bool equal(const transform& lhs, const transform& rhs, float epsilon = EPSILON) {
    return equal(lhs.translation, rhs.translation, epsilon) &&
           equal(lhs.rotation, rhs.rotation, epsilon) &&
           equal(lhs.scale, rhs.scale, epsilon);
}

inline bool isfinite(const transform& value) {
    return isfinite(value.translation) && isfinite(value.rotation) && isfinite(value.scale);
}

// This representation works very well for aabbox VS frustum intersection test, but worse for every other operation.
class aabbox {
public:
    static constexpr aabbox from_min_max(const float3& min, const float3& max) {
        return aabbox((min + max) / 2.f, (max - min) / 2.f);
    }

    constexpr aabbox()
        : center()
        , extent()
    {
    }

    constexpr aabbox(const float3& center, const float3& extent)
        : center(center)
        , extent(extent)
    {
    }

    aabbox operator*(const float4x4& rhs) const;
    aabbox operator*(const transform& rhs) const;

    constexpr aabbox operator+(const float3& rhs) const {
        float3 min_ = min(center - extent, rhs);
        float3 max_ = max(center + extent, rhs);

        return from_min_max(min_, max_);
    }

    constexpr aabbox operator+(const aabbox& rhs) const {
        float3 min_ = min(center - extent, rhs.center - rhs.extent);
        float3 max_ = max(center + extent, rhs.center + rhs.extent);

        return from_min_max(min_, max_);
    }

    inline aabbox& operator*=(const float4x4& rhs) {
        return *this = *this * rhs;
    }

    inline aabbox& operator*=(const transform& rhs) {
        return *this = *this * rhs;
    }

    constexpr aabbox& operator+=(const float3& value) {
        return *this = *this + value;
    }

    constexpr aabbox& operator+=(const aabbox& value) {
        return *this = *this + value;
    }

    constexpr bool operator==(const aabbox& value) const {
        return center == value.center && extent == value.extent;
    }

    constexpr bool operator!=(const aabbox& value) const {
        return center != value.center || extent != value.extent;
    }

    union {
        struct {
            float3 center;
            float3 extent;
        };

        float data[6];
    };
};

inline bool intersect(const float3& lhs, const aabbox& rhs) {
    return std::abs(rhs.center.x - lhs.x) <= rhs.extent.x &&
           std::abs(rhs.center.y - lhs.y) <= rhs.extent.y &&
           std::abs(rhs.center.z - lhs.z) <= rhs.extent.z;
}

inline bool intersect(const aabbox& lhs, const aabbox& rhs) {
    return std::abs(lhs.center.x - rhs.center.x) <= lhs.extent.x + rhs.extent.x &&
           std::abs(lhs.center.y - rhs.center.y) <= lhs.extent.y + rhs.extent.y &&
           std::abs(lhs.center.z - rhs.center.z) <= lhs.extent.z + rhs.extent.z;
}

constexpr bool equal(const aabbox& lhs, const aabbox& rhs, float epsilon = EPSILON) {
    return equal(lhs.center, rhs.center, epsilon) && equal(lhs.extent, rhs.extent, epsilon);
}

inline bool isfinite(const aabbox& value) {
    return isfinite(value.center) && isfinite(value.extent);
}

} // namespace kw
