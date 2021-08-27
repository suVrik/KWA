#pragma once

#include "render/camera/camera.h"

namespace kw {

class CameraManager;
class Input;
class Timer;
class Window;

struct CameraControllerDescriptor {
    Window* window;
    Input* input;
    Timer* timer;
    CameraManager* camera_manager;
};

class CameraController {
public:
    explicit CameraController(const CameraControllerDescriptor& descriptor);

    float get_speed() const;
    void set_speed(float value);
    
    float get_acceleration() const;
    void set_acceleration(float value);
    
    float get_deceleration() const;
    void set_deceleration(float value);
    
    float get_mouse_sensitivity() const;
    void set_mouse_sensitivity(float value);
    
    float get_mouse_deceleration() const;
    void set_mouse_deceleration(float value);
    
    const float3& get_linear_velocity() const;
    void set_linear_velocity(const float3& value);

    void update();

private:
    Window& m_window;
    Input& m_input;
    Timer& m_timer;
    CameraManager& m_camera_manager;

    Camera& m_camera;

    float m_speed;
    float m_acceleration;
    float m_deceleration;
    float m_mouse_sensitivity;
    float m_mouse_deceleration;
    float3 m_linear_velocity;

    float m_yaw;
    float m_pitch;
    float m_yaw_velocity;
    float m_pitch_velocity;
    float3 m_velocity;
};

} // namespace kw
