#pragma once

#include "core/math/float3.h"

namespace kw {

class float4x4;
class transform;

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
