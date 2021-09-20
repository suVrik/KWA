#pragma once

#include <render/scene/render_primitive_reflection.h>

#include <physics/scene/physics_primitive_reflection.h>

namespace kw {

class AnimationManager;
class GeometryManager;
class HeightFieldManager;
class MaterialManager;
class MemoryResource;
class MotionGraphManager;
class ParticleSystemManager;
class PhysicsManager;
class PrefabManager;
class TextureManager;

struct GamePrimitiveReflectionDescriptor {
    PhysicsManager* physics_manager;
    HeightFieldManager* height_field_manager;
    TextureManager* texture_manager;
    GeometryManager* geometry_manager;
    MaterialManager* material_manager;
    AnimationManager* animation_manager;
    MotionGraphManager* motion_graph_manager;
    ParticleSystemManager* particle_system_manager;
    PrefabManager* prefab_manager;
    MemoryResource* memory_resource;
};

class GamePrimitiveReflection : public RenderPrimitiveReflection, public PhysicsPrimitiveReflection {
public:
    explicit GamePrimitiveReflection(const GamePrimitiveReflectionDescriptor& descriptor);
};

} // namespace kw
