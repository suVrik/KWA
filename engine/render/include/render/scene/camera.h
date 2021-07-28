#pragma once

#include <core/math/transform.h>

// TODO:
//#include <core/math/frustum.h>

namespace kw {

class Camera {
public:
    Camera();

    const transform& get_transform() const {
        return m_transform;
    }

    void set_transform(const transform& value);
    
    const float3& get_translation() const {
        return m_transform.translation;
    }

    void set_translation(const float3& value);

    const quaternion& get_rotation() const {
        return m_transform.rotation;
    }

    void set_rotation(const quaternion& value);

    float get_fov() const {
        return m_fov;
    }

    void set_fov(float value);

    float get_aspect_ratio() const {
        return m_aspect_ratio;
    }

    void set_aspect_ratio(float value);

    float get_z_near() const {
        return m_z_near;
    }

    void set_z_near(float value);

    float get_z_far() const {
        return m_z_far;
    }

    void set_z_far(float value);

    const float4x4& get_view_matrix() const {
        return m_view_matrix;
    }

    const float4x4& get_projection_matrix() const {
        return m_projection_matrix;
    }

    const float4x4& get_view_projection_matrix() const {
        return m_view_projection_matrix;
    }

    const float4x4& get_inverse_view_matrix() const {
        return m_inverse_view_matrix;
    }

    const float4x4& get_inverse_projection_matrix() const {
        return m_inverse_projection_matrix;
    }

    const float4x4& get_inverse_view_projection_matrix() const {
        return m_inverse_view_projection_matrix;
    }

    // TODO:
    //const frustum& get_frustum() const {
    //    return m_frustum;
    //}

private:
    void update_view_matrix();
    void update_projection_matrix();
    void update_view_projection_matrix();

    // TODO:
    // void update_frustum();

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

    // TODO:
    // frustum m_frustum;
};

} // namespace kw
