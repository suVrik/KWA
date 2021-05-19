#pragma once

#include "core/math/aabbox.h"
#include "core/math/float4x4.h"
#include "core/math/plane.h"

namespace kw {

class frustum {
public:
    frustum()
        : top()
        , bottom()
        , left()
        , right()
        , near()
        , far()
    {
    }

    frustum(const plane& top, const plane& bottom, const plane& left, const plane& right, const plane& near, const plane& far)
        : top(top)
        , bottom(bottom)
        , left(left)
        , right(right)
        , near(near)
        , far(far)
    {
    }

    frustum(const float4x4& matrix)
        : top(float3(matrix[0][3] - matrix[0][1], matrix[1][3] - matrix[1][1], matrix[2][3] - matrix[2][1]), matrix[3][3] - matrix[3][1])
        , bottom(float3(matrix[0][3] + matrix[0][1], matrix[1][3] + matrix[1][1], matrix[2][3] + matrix[2][1]), matrix[3][3] + matrix[3][1])
        , left(float3(matrix[0][3] + matrix[0][0], matrix[1][3] + matrix[1][0], matrix[2][3] + matrix[2][0]), matrix[3][3] + matrix[3][0])
        , right(float3(matrix[0][3] - matrix[0][0], matrix[1][3] - matrix[1][0], matrix[2][3] - matrix[2][0]), matrix[3][3] - matrix[3][0])
        , near(float3(matrix[0][2], matrix[1][2], matrix[2][2]), matrix[3][2])
        , far(float3(matrix[0][3] - matrix[0][2], matrix[1][3] - matrix[1][2], matrix[2][3] - matrix[2][2]), matrix[3][3] - matrix[3][2])
    {
    }

    union {
        struct {
            plane top;
            plane bottom;
            plane left;
            plane right;
            plane near;
            plane far;
        };

        plane data[6];
    };
};

constexpr bool intersect(const float3& point, const frustum& frustum) {
    for (size_t i = 0; i < 6; i++) {
        float distance = dot(point, frustum.data[i].normal) + frustum.data[i].distance;
        if (distance < 0.f) {
            return false;
        }
    }
    return true;
}

constexpr bool intersect(const aabbox& box, const frustum& frustum) {
    for (const plane& plane : frustum.data) {
        if (dot(box.center, plane.normal) + dot(box.extent, abs(plane.normal)) < -plane.distance) {
            return false;
        }
    }
    return true;
}

} // namespace kw
