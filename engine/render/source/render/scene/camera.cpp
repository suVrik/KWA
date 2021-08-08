#include "render/scene/camera.h"

namespace kw {

Camera::Camera()
    : m_fov(radians(70.f))
    , m_aspect_ratio(1.f)
    , m_z_near(0.05f)
    , m_z_far(50.f)
{
    update_projection_matrix();
}

const transform& Camera::get_transform() const {
    return m_transform;
}

void Camera::set_transform(const transform& value) {
    m_transform = value;
    update_view_matrix();
}

const float3& Camera::get_translation() const {
    return m_transform.translation;
}

void Camera::set_translation(const float3& value) {
    m_transform.translation = value;
    update_view_matrix();
}

const quaternion& Camera::get_rotation() const {
    return m_transform.rotation;
}

void Camera::set_rotation(const quaternion& value) {
    m_transform.rotation = value;
    update_view_matrix();
}

float Camera::get_fov() const {
    return m_fov;
}

void Camera::set_fov(float value) {
    m_fov = value;
    update_projection_matrix();
}

float Camera::get_aspect_ratio() const {
    return m_aspect_ratio;
}

void Camera::set_aspect_ratio(float value) {
    m_aspect_ratio = value;
    update_projection_matrix();
}

float Camera::get_z_near() const {
    return m_z_near;
}

void Camera::set_z_near(float value) {
    m_z_near = value;
    update_projection_matrix();
}

float Camera::get_z_far() const {
    return m_z_far;
}

void Camera::set_z_far(float value) {
    m_z_far = value;
    update_projection_matrix();
}

const float4x4& Camera::get_view_matrix() const {
    return m_view_matrix;
}

const float4x4& Camera::get_projection_matrix() const {
    return m_projection_matrix;
}

const float4x4& Camera::get_view_projection_matrix() const {
    return m_view_projection_matrix;
}

const float4x4& Camera::get_inverse_view_matrix() const {
    return m_inverse_view_matrix;
}

const float4x4& Camera::get_inverse_projection_matrix() const {
    return m_inverse_projection_matrix;
}

const float4x4& Camera::get_inverse_view_projection_matrix() const {
    return m_inverse_view_projection_matrix;
}

const frustum& Camera::get_frustum() const {
    return m_frustum;
}

void Camera::update_view_matrix() {
    m_view_matrix = float4x4::look_at_lh(m_transform.translation, m_transform.translation + float3(0.f, 0.f, 1.f) * m_transform.rotation, float3(0.f, 1.f, 0.f) * m_transform.rotation);

    m_inverse_view_matrix = inverse(m_view_matrix);

    update_view_projection_matrix();
}

void Camera::update_projection_matrix() {
    m_projection_matrix = float4x4::perspective_rh(m_fov, m_aspect_ratio, m_z_near, m_z_far);

    m_inverse_projection_matrix = inverse(m_projection_matrix);

    update_view_projection_matrix();
}

void Camera::update_view_projection_matrix() {
    m_view_projection_matrix = m_view_matrix * m_projection_matrix;

    m_inverse_view_projection_matrix = inverse(m_view_projection_matrix);

    update_frustum();
}

void Camera::update_frustum() {
    m_frustum = frustum(m_view_projection_matrix);
}

} // namespace kw
