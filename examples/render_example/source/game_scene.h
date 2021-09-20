#pragma once

#include <render/scene/render_scene.h>

#include <physics/scene/physics_scene.h>

namespace kw {

class AccelerationStructure;
class AnimationPlayer;
class MemoryResource;
class ParticleSystemPlayer;
class PhysicsManager;
class ReflectionProbeManager;

struct GameSceneDescriptor {
    PhysicsManager* physics_manager;
    AnimationPlayer* animation_player;
    ParticleSystemPlayer* particle_system_player;
    ReflectionProbeManager* reflection_probe_manager;
    AccelerationStructure* geometry_acceleration_structure;
    AccelerationStructure* light_acceleration_structure;
    AccelerationStructure* particle_system_acceleration_structure;
    AccelerationStructure* reflection_probe_acceleration_structure;
    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

class GameScene : public RenderScene, public PhysicsScene {
public:
    GameScene(const GameSceneDescriptor& descriptor);

    void child_added(Primitive& primitive) override;
};

} // namespace kw
