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
        : left(left)
        , right(right)
        , bottom(bottom)
        , top(top)
        , near(near)
        , far(far)
    {
    }

    frustum(const float4x4& view_projection_matrix)
        : left(normalize(plane(
            float3(
                view_projection_matrix[0][3] + view_projection_matrix[0][0],
                view_projection_matrix[1][3] + view_projection_matrix[1][0],
                view_projection_matrix[2][3] + view_projection_matrix[2][0]
            ),
            view_projection_matrix[3][3] + view_projection_matrix[3][0]
        )))
        , right(normalize(plane(
            float3(
                view_projection_matrix[0][3] - view_projection_matrix[0][0],
                view_projection_matrix[1][3] - view_projection_matrix[1][0],
                view_projection_matrix[2][3] - view_projection_matrix[2][0]
            ),
            view_projection_matrix[3][3] - view_projection_matrix[3][0]
        )))
        , bottom(normalize(plane(
            float3(
                view_projection_matrix[0][3] + view_projection_matrix[0][1],
                view_projection_matrix[1][3] + view_projection_matrix[1][1],
                view_projection_matrix[2][3] + view_projection_matrix[2][1]
            ),
            view_projection_matrix[3][3] + view_projection_matrix[3][1]
        )))
        , top(normalize(plane(
            float3(
                view_projection_matrix[0][3] - view_projection_matrix[0][1],
                view_projection_matrix[1][3] - view_projection_matrix[1][1],
                view_projection_matrix[2][3] - view_projection_matrix[2][1]
            ),
            view_projection_matrix[3][3] - view_projection_matrix[3][1]
        )))
        , near(normalize(plane(
            float3(
                view_projection_matrix[0][2],
                view_projection_matrix[1][2],
                view_projection_matrix[2][2]
            ),
            view_projection_matrix[3][2]
        )))
        , far(normalize(plane(
            float3(
                view_projection_matrix[0][3] - view_projection_matrix[0][2],
                view_projection_matrix[1][3] - view_projection_matrix[1][2],
                view_projection_matrix[2][3] - view_projection_matrix[2][2]
            ),
            view_projection_matrix[3][3] - view_projection_matrix[3][2]
        )))
    {
    }

    union {
        struct {
            plane left;
            plane right;
            plane bottom;
            plane top;
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
