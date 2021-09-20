#pragma once

#include <core/containers/string.h>
#include <core/prefab/prefab_primitive.h>

namespace kw {

class CameraManager;
class CapsuleControllerPrimitive;
class DebugDrawManager;
class GameScene;
class GeometryManager;
class Input;
class MaterialManager;
class MotionGeometryPrimitive;
class MotionGraphManager;
class Timer;
class Window;

struct PlayerDescriptor {
    DebugDrawManager* debug_draw_manager;
    MotionGraphManager* motion_graph_manager;
    GeometryManager* geometry_manager;
    MaterialManager* material_manager;
    MemoryResource* memory_resource;
    GameScene* scene;
    Window* window;
    Input* input;
    Timer* timer;
    CameraManager* camera_manager;
};

class Player : public PrefabPrimitive {
public:
    Player(const PlayerDescriptor& descriptor);

    void update();

    DebugDrawManager& m_debug_draw_manager;

    GameScene& m_scene;
    Window& m_window;
    Input& m_input;
    Timer& m_timer;
    CameraManager& m_camera_manager;

    String m_is_idle_attribute;
    String m_angle_attribute;
    String m_walk_event;
    String m_crouch_event;
    String m_left_thigh_joint;
    String m_left_calf_joint;
    String m_left_foot_joint;
    String m_right_thigh_joint;
    String m_right_calf_joint;
    String m_right_foot_joint;

    MotionGeometryPrimitive* m_geometry_primitive;
    CapsuleControllerPrimitive* m_controller_primitive;

    float yaw = 0.f;
    float pitch = 0.f;
    float movement_yaw = 0.f;

    float3 m_velocity;

    float m_geometry_primitive_offset;
    float m_left_foot_offset;
    float m_right_foot_offset;
};

} // namespace kw
