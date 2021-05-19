#pragma once

#include "core/math/float4x4.h"
#include "core/math/quaternion.h"

namespace kw {

class transform {
public:
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

    explicit transform(const float4x4& value);

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
            rhs.rotation * rotation,
            rhs.scale * scale
        );
    }

    constexpr transform& operator*=(const transform& value) {
        translation = translation * value.rotation * value.scale + value.translation;
        rotation = value.rotation * rotation;
        scale = value.scale * scale;

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

} // namespace kw
