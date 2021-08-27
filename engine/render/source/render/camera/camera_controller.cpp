#include "render/camera/camera_controller.h"
#include "render/camera/camera_manager.h"

#include <system/input.h>
#include <system/timer.h>
#include <system/window.h>

#include <core/debug/assert.h>

namespace kw {

CameraController::CameraController(const CameraControllerDescriptor& descriptor)
    : m_window(*descriptor.window)
    , m_input(*descriptor.input)
    , m_timer(*descriptor.timer)
    , m_camera_manager(*descriptor.camera_manager)
    , m_camera(m_camera_manager.get_camera())
    , m_speed(12.f)
    , m_acceleration(30.f)
    , m_deceleration(20.f)
    , m_mouse_sensitivity(0.002f)
    , m_mouse_deceleration(0.6f)
    , m_yaw(0.f)
    , m_pitch(0.f)
    , m_yaw_velocity(0.f)
    , m_pitch_velocity(0.f)
{
    KW_ASSERT(descriptor.window != nullptr);
    KW_ASSERT(descriptor.input != nullptr);
    KW_ASSERT(descriptor.timer != nullptr);
    KW_ASSERT(descriptor.camera_manager != nullptr);

    m_camera.set_fov(radians(60.f));
    m_camera.set_z_near(0.1f);
    m_camera.set_z_far(100.f);
}

float CameraController::get_speed() const {
    return m_speed;
}

void CameraController::set_speed(float value) {
    m_speed = value;
}

float CameraController::get_acceleration() const {
    return m_acceleration;
}

void CameraController::set_acceleration(float value) {
    m_acceleration = value;
}

float CameraController::get_deceleration() const {
    return m_deceleration;
}

void CameraController::set_deceleration(float value) {
    m_deceleration = value;
}

float CameraController::get_mouse_sensitivity() const {
    return m_mouse_sensitivity;
}

void CameraController::set_mouse_sensitivity(float value) {
    m_mouse_sensitivity = value;
}

float CameraController::get_mouse_deceleration() const {
    return m_mouse_deceleration;
}

void CameraController::set_mouse_deceleration(float value) {
    m_mouse_deceleration = value;
}

const float3& CameraController::get_linear_velocity() const {
    return m_linear_velocity;
}

void CameraController::set_linear_velocity(const float3& value) {
    m_linear_velocity = value;
}

void CameraController::update() {
    float3 camera_position = m_camera.get_translation();

    m_yaw_velocity *= m_mouse_deceleration;
    m_pitch_velocity *= m_mouse_deceleration;

    if (m_input.is_button_down(BUTTON_LEFT)) {
        m_yaw_velocity += m_input.get_mouse_dx() * m_mouse_sensitivity;
        m_pitch_velocity += m_input.get_mouse_dy() * m_mouse_sensitivity;
    }

    m_yaw += m_yaw_velocity;
    m_pitch += m_pitch_velocity;

    quaternion camera_rotation =
        quaternion::rotation(float3(0.f, 1.f, 0.f), m_yaw) *
        quaternion::rotation(float3(1.f, 0.f, 0.f), m_pitch);

    float3 forward = float3(0.f, 0.f, -1.f) * camera_rotation;
    float3 left = float3(-1.f, 0.f, 0.f) * camera_rotation;
    float3 up = float3(0.f, 1.f, 0.f);

    float3 direction;

    if (m_input.is_key_down(Scancode::W)) {
        direction -= forward;
    }
    if (m_input.is_key_down(Scancode::A)) {
        direction += left;
    }
    if (m_input.is_key_down(Scancode::S)) {
        direction += forward;
    }
    if (m_input.is_key_down(Scancode::D)) {
        direction -= left;
    }
    if (m_input.is_key_down(Scancode::Q)) {
        direction -= up;
    }
    if (m_input.is_key_down(Scancode::E)) {
        direction += up;
    }

    float velocity_length = length(m_velocity);
    if (!equal(velocity_length, 0.f)) {
        m_velocity -= m_velocity / velocity_length * std::min(velocity_length, m_deceleration * m_timer.get_elapsed_time());
    }

    float direction_length = length(direction);
    if (!equal(direction_length, 0.f)) {
        m_velocity += direction / direction_length * m_acceleration * m_timer.get_elapsed_time();
    }

    velocity_length = length(m_velocity);
    if (!equal(velocity_length, 0.f)) {
        m_velocity = m_velocity / velocity_length * std::min(velocity_length, m_speed);
    }

    camera_position += m_velocity * m_timer.get_elapsed_time();

    m_camera.set_aspect_ratio(static_cast<float>(m_window.get_width()) / m_window.get_height());
    m_camera.set_rotation(camera_rotation);
    m_camera.set_translation(camera_position);
}

} // namespace kw
