#pragma once

#include <core/math/frustum.h>
#include <core/math/transform.h>

namespace kw {

class Camera {
public:
    Camera();

    const transform& get_transform() const;
    void set_transform(const transform& value);

    const float3& get_translation() const;
    void set_translation(const float3& value);

    const quaternion& get_rotation() const;
    void set_rotation(const quaternion& value);

    float get_fov() const;
    void set_fov(float value);

    float get_aspect_ratio() const;
    void set_aspect_ratio(float value);

    float get_z_near() const;
    void set_z_near(float value);

    float get_z_far() const;
    void set_z_far(float value);

    const float4x4& get_view_matrix() const;
    const float4x4& get_projection_matrix() const;
    const float4x4& get_view_projection_matrix() const;
    const float4x4& get_inverse_view_matrix() const;
    const float4x4& get_inverse_projection_matrix() const;
    const float4x4& get_inverse_view_projection_matrix() const;
    const frustum& get_frustum() const;

private:
    void update_view_matrix();
    void update_projection_matrix();
    void update_view_projection_matrix();
    void update_frustum();

    transform m_transform;
    float m_fov;
    float m_aspect_ratio;
    float m_z_near;
    float m_z_far;

    float4x4 m_view_matrix;
    float4x4 m_projection_matrix;
    float4x4 m_view_projection_matrix;
    float4x4 m_inverse_view_matrix;
    float4x4 m_inverse_projection_matrix;
    float4x4 m_inverse_view_projection_matrix;
    frustum m_frustum;
};

} // namespace kw
