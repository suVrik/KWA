#include "player.h"
#include "game_scene.h"

#include <physics/scene/capsule_controller_primitive.h>

#include <render/camera/camera_manager.h>
#include <render/geometry/geometry.h>
#include <render/geometry/geometry_manager.h>
#include <render/geometry/skeleton.h>
#include <render/material/material_manager.h>
#include <render/motion/motion_geometry_primitive.h>
#include <render/motion/motion_graph.h>
#include <render/motion/motion_graph_manager.h>
#include <render/debug/debug_draw_manager.h>

#include <system/input.h>
#include <system/timer.h>
#include <system/window.h>

namespace kw {

constexpr float CAMERA_FOV = radians(60.f);
constexpr float CAMERA_Z_NEAR = 0.1f;
constexpr float CAMERA_Z_FAR = 100.f;
constexpr float MOUSE_SENSITIVITY = 0.002f;
constexpr float SPEED_WALK = 4.f;
constexpr float SPEED_CROUCH = 2.f;
constexpr float GROUNDED_VERTICAL_VELOCITY = -5.f;
constexpr float GRAVITATIONAL_ACCELERATION = -9.8f;
constexpr float MOVEMENT_YAW_CHANGE_SPEED = 1.5f * PI;
constexpr float IS_IDLE_CHANGE_SPEED = 4.f;
constexpr float ANGLE_RESET_IS_IDLE_THRESHOLD = 0.75f;
constexpr float ANGLE_RESET_DELTA_THRESHOLD = 0.75f * PI;
constexpr float ANGLE_RESET_DURATION = 0.35f;
constexpr float ANGLE_CHANGE_SPEED = PI;

Player::Player(const PlayerDescriptor& descriptor)
    : PrefabPrimitive(*descriptor.memory_resource)
    , m_debug_draw_manager(*descriptor.debug_draw_manager)
    , m_scene(*descriptor.scene)
    , m_window(*descriptor.window)
    , m_input(*descriptor.input)
    , m_timer(*descriptor.timer)
    , m_camera_manager(*descriptor.camera_manager)
    , m_is_idle_attribute("is_idle", *descriptor.memory_resource)
    , m_angle_attribute("angle", *descriptor.memory_resource)
    , m_walk_event("walk", *descriptor.memory_resource)
    , m_crouch_event("crouch", *descriptor.memory_resource)
    , m_left_thigh_joint("Robot 4 L Thigh", *descriptor.memory_resource)
    , m_left_calf_joint("Robot 4 L Calf", *descriptor.memory_resource)
    , m_left_foot_joint("Robot 4 L Foot", *descriptor.memory_resource)
    , m_right_thigh_joint("Robot 4 R Thigh", *descriptor.memory_resource)
    , m_right_calf_joint("Robot 4 R Calf", *descriptor.memory_resource)
    , m_right_foot_joint("Robot 4 R Foot", *descriptor.memory_resource)
    , m_geometry_primitive_offset(0.f)
    , m_left_foot_offset(0.f)
    , m_right_foot_offset(0.f)
{
    UniquePtr<MotionGeometryPrimitive> geometry_primitive = allocate_unique<MotionGeometryPrimitive>(
        *descriptor.memory_resource,
        *descriptor.memory_resource,
        descriptor.motion_graph_manager->load("resource/motion_graphs/test_subject.kwm"),
        descriptor.geometry_manager->load("resource/geometry/ik/test_subject.kwg"),
        descriptor.material_manager->load("resource/materials/ik/test_subject.kwm"),
        descriptor.material_manager->load("resource/materials/skinned_shadow.kwm")
    );

    m_geometry_primitive = geometry_primitive.get();
    m_geometry_primitive->set_attribute(m_is_idle_attribute, 1.f);

    add_child(std::move(geometry_primitive));

    UniquePtr<CapsuleControllerPrimitive> controller_primitive = allocate_unique<CapsuleControllerPrimitive>(
        *descriptor.memory_resource, 0.3f, 0.5f, 0.5f
    );

    m_controller_primitive = controller_primitive.get();

    add_child(std::move(controller_primitive));

    m_input.toggle_mouse_relative(true);
}

void Player::update() {
    //
    // Update camera orientation.
    //

    yaw += m_input.get_mouse_dx() * MOUSE_SENSITIVITY;
    pitch += m_input.get_mouse_dy() * MOUSE_SENSITIVITY;

    //
    // Update blend tree's `is_idle` attribute. 
    //

    float is_idle_attribute = m_geometry_primitive->get_attribute(m_is_idle_attribute);

    bool is_idle = m_input.is_key_down(Scancode::W) || m_input.is_key_down(Scancode::A) || m_input.is_key_down(Scancode::S) || m_input.is_key_down(Scancode::D);
    if (is_idle) {
        m_geometry_primitive->set_attribute(m_is_idle_attribute, std::max(is_idle_attribute - IS_IDLE_CHANGE_SPEED * m_timer.get_elapsed_time(), 0.f));
    } else {
        m_geometry_primitive->set_attribute(m_is_idle_attribute, std::min(is_idle_attribute + IS_IDLE_CHANGE_SPEED * m_timer.get_elapsed_time(), 1.f));
    }

    //
    // Update movement angle.
    //

    if (is_idle) {
        float delta = shortest_angle(normalize_angle(movement_yaw), normalize_angle(yaw));
        if (delta > 0.f) {
            movement_yaw += std::min(delta, MOVEMENT_YAW_CHANGE_SPEED * m_timer.get_elapsed_time());
        } else {
            movement_yaw += std::max(delta, -MOVEMENT_YAW_CHANGE_SPEED * m_timer.get_elapsed_time());
        }
    }

    //
    // Compute movement speed.
    //

    float speed;

    bool is_crouching = m_input.is_key_down(Scancode::CTRL);
    if (is_crouching) {
        m_geometry_primitive->emit_event(m_crouch_event);
        speed = SPEED_CROUCH;
    } else {
        m_geometry_primitive->emit_event(m_walk_event);
        speed = SPEED_WALK;
    }

    //
    // Compute movement direction (relative to movement angle).
    //

    float3 direction;

    if (m_input.is_key_down(Scancode::W) && !m_input.is_key_down(Scancode::S)) {
        direction.z = 1.f;
    } else if (m_input.is_key_down(Scancode::S) && !m_input.is_key_down(Scancode::W)) {
        direction.z = -1.f;
    }

    if (m_input.is_key_down(Scancode::A) && !m_input.is_key_down(Scancode::D)) {
        direction.x = -1.f;
    } else if (m_input.is_key_down(Scancode::D) && !m_input.is_key_down(Scancode::A)) {
        direction.x = 1.f;
    }

    if (!equal(length(direction), 0.f)) {
        direction = normalize(direction);
    }

    //
    // Apply gravity.
    //

    bool is_grounded = (m_controller_primitive->move(float3(0.f, m_velocity.y, 0.f) * m_timer.get_elapsed_time()) & ControllerCollision::COLLISION_DOWN) == ControllerCollision::COLLISION_DOWN;
    if (is_grounded) {
        m_velocity.y = GROUNDED_VERTICAL_VELOCITY;
    } else {
        m_velocity.y += GRAVITATIONAL_ACCELERATION * m_timer.get_elapsed_time();
    }

    //
    // Move and transfer transform from controller primitive to player primitive.
    //

    m_controller_primitive->move(speed * direction * quaternion::rotation(float3(0.f, 1.f, 0.f), movement_yaw) * m_timer.get_elapsed_time());

    set_global_translation(m_controller_primitive->get_global_translation());

    m_geometry_primitive->set_local_translation(float3());
    m_controller_primitive->set_local_translation(float3());

    m_geometry_primitive->set_local_rotation(quaternion::rotation(float3(0.f, 1.f, 0.f), movement_yaw + PI));

    //
    // Update blend tree's angle attribute.
    //

    if (!equal(length(direction), 0.f)) {
        float source_angle = radians(m_geometry_primitive->get_attribute(m_angle_attribute));
        float target_angle = normalize_angle(std::atan2f(direction.z, direction.x));
        float delta = shortest_angle(source_angle, target_angle);

        if (m_geometry_primitive->get_attribute(m_is_idle_attribute) > ANGLE_RESET_IS_IDLE_THRESHOLD) {
            m_geometry_primitive->frozen_fade(ANGLE_RESET_DURATION);
            m_geometry_primitive->set_attribute(m_angle_attribute, degrees(normalize_angle(target_angle)));
        } else {
            if (std::abs(delta) >= ANGLE_RESET_DELTA_THRESHOLD) {
                m_geometry_primitive->frozen_fade(ANGLE_RESET_DURATION);
                m_geometry_primitive->set_attribute(m_angle_attribute, degrees(normalize_angle(target_angle)));
            } else {
                if (delta > 0.f) {
                    m_geometry_primitive->set_attribute(m_angle_attribute, degrees(normalize_angle(source_angle + std::min(delta, ANGLE_CHANGE_SPEED * m_timer.get_elapsed_time()))));
                } else {
                    m_geometry_primitive->set_attribute(m_angle_attribute, degrees(normalize_angle(source_angle + std::max(delta, -ANGLE_CHANGE_SPEED * m_timer.get_elapsed_time()))));
                }
            }
        }
    }

    //
    // Apply IK.
    //

    SharedPtr<Geometry> geometry = m_geometry_primitive->get_geometry();
    if (geometry && geometry->is_loaded() && geometry->get_skeleton()) {
        const Skeleton* skeleton = geometry->get_skeleton();

        uint32_t left_thigh_index = skeleton->get_joint_index(m_left_thigh_joint);
        uint32_t left_calf_index = skeleton->get_joint_index(m_left_calf_joint);
        uint32_t left_foot_index = skeleton->get_joint_index(m_left_foot_joint);

        uint32_t right_thigh_index = skeleton->get_joint_index(m_right_thigh_joint);
        uint32_t right_calf_index = skeleton->get_joint_index(m_right_calf_joint);
        uint32_t right_foot_index = skeleton->get_joint_index(m_right_foot_joint);

        if (left_thigh_index != UINT32_MAX && left_calf_index != UINT32_MAX && left_foot_index != UINT32_MAX &&
            right_thigh_index != UINT32_MAX && right_calf_index != UINT32_MAX && right_foot_index != UINT32_MAX)
        {
            if (is_grounded) {
                const Vector<float4x4>& model_space_matrices = m_geometry_primitive->get_model_space_joint_pre_ik_matrices();

                const float4x4& left_foot_model = model_space_matrices[left_foot_index];
                const float4x4& right_foot_model = model_space_matrices[right_foot_index];

                float4x4 global_transform_matrix(m_geometry_primitive->get_global_transform());

                float4x4 left_foot_global = left_foot_model * global_transform_matrix;
                float4x4 right_foot_global = right_foot_model * global_transform_matrix;

                float3 left_foot = point_transform(float3(), left_foot_global);
                float3 right_foot = point_transform(float3(), right_foot_global);

                const float3& global_translation = get_global_translation();

                std::optional<QueryResult> left_result = m_scene.raycast(float3(left_foot.x, global_translation.y + 1.f, left_foot.z), float3(0.f, -1.f, 0.f), 2.f, 1);
                std::optional<QueryResult> right_result = m_scene.raycast(float3(right_foot.x, global_translation.y + 1.f, right_foot.z), float3(0.f, -1.f, 0.f), 2.f, 1);

                float geometry_primitive_offset = 0.f;
                
                if (left_result) {
                    geometry_primitive_offset = std::min(geometry_primitive_offset, left_result->position.y - global_translation.y);
                }

                if (right_result) {
                    geometry_primitive_offset = std::min(geometry_primitive_offset, right_result->position.y - global_translation.y);
                }

                m_geometry_primitive_offset = lerp(m_geometry_primitive_offset, geometry_primitive_offset, 0.1f);

                // TODO: Use linear rather than exponetial to work better on different framerate.
                m_geometry_primitive->set_local_translation(float3(0.f, m_geometry_primitive_offset, 0.f));

                if (left_result) {
                    // TODO: Use linear rather than exponetial to work better on different framerate.
                    m_left_foot_offset = lerp(m_left_foot_offset, left_result->position.y - m_geometry_primitive->get_global_translation().y + m_geometry_primitive_offset, 0.5f);

                    left_foot.y += m_left_foot_offset;

                    m_geometry_primitive->set_ik_target(left_thigh_index, left_calf_index, left_foot_index, float4(left_foot, 1.f));
                } else {
                    m_geometry_primitive->set_ik_target(left_thigh_index, left_calf_index, left_foot_index, float4(0.f));
                }

                if (right_result) {
                    // TODO: Use linear rather than exponetial to work better on different framerate.
                    m_right_foot_offset = lerp(m_right_foot_offset, right_result->position.y - m_geometry_primitive->get_global_translation().y + m_geometry_primitive_offset, 0.5f);

                    right_foot.y += m_right_foot_offset;

                    m_geometry_primitive->set_ik_target(right_thigh_index, right_calf_index, right_foot_index, float4(right_foot, 1.f));
                } else {
                    m_geometry_primitive->set_ik_target(right_thigh_index, right_calf_index, right_foot_index, float4(0.f));
                }
            } else {
                m_geometry_primitive->set_ik_target(left_thigh_index, left_calf_index, left_foot_index, float4(0.f));
                m_geometry_primitive->set_ik_target(right_thigh_index, right_calf_index, right_foot_index, float4(0.f));
            }
        }
    }

    //
    // Update camera.
    //

    quaternion camera_rotation =
        quaternion::rotation(float3(0.f, 1.f, 0.f), yaw) *
        quaternion::rotation(float3(1.f, 0.f, 0.f), pitch);

    float3 camera_position = get_global_translation() + float3(0.f, 1.5f, 0.f);

    float aspect = static_cast<float>(m_window.get_width()) / m_window.get_height();

    std::optional<QueryResult> result = m_scene.sweep_box(transform(camera_position, camera_rotation, float3(0.1f * aspect, 0.1f, 0.05f)), float3(0.f, 0.f, -1.f) * camera_rotation, 3.f);
    if (result) {
        camera_position += float3(0.f, 0.f, -result->distance) * camera_rotation;
    } else {
        camera_position += float3(0.f, 0.f, -3.f) * camera_rotation;
    }

    Camera& camera = m_camera_manager.get_camera();
    camera.set_rotation(camera_rotation);
    camera.set_translation(camera_position);
    camera.set_fov(CAMERA_FOV);
    camera.set_aspect_ratio(aspect);
    camera.set_z_near(CAMERA_Z_NEAR);
    camera.set_z_far(CAMERA_Z_FAR);
}

} // namespace kw
