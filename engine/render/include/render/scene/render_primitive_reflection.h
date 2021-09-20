#pragma once

#include <core/scene/primitive_reflection.h>

namespace kw {

class AnimationManager;
class GeometryManager;
class MaterialManager;
class MotionGraphManager;
class ObjectNode;
class ParticleSystemManager;
class Primitive;
class TextureManager;

struct RenderPrimitiveReflectionDescriptor {
    TextureManager* texture_manager;
    GeometryManager* geometry_manager;
    MaterialManager* material_manager;
    AnimationManager* animation_manager;
    MotionGraphManager* motion_graph_manager;
    ParticleSystemManager* particle_system_manager;
    PrefabManager* prefab_manager;
    MemoryResource* memory_resource;
};

class RenderPrimitiveReflection : public virtual PrimitiveReflection {
public:
    explicit RenderPrimitiveReflection(const RenderPrimitiveReflectionDescriptor& descriptor);

    TextureManager& texture_manager;
    GeometryManager& geometry_manager;
    MaterialManager& material_manager;
    AnimationManager& animation_manager;
    MotionGraphManager& motion_graph_manager;
    ParticleSystemManager& particle_system_manager;
};

} // namespace kw
