#pragma once

#include <core/scene/primitive_reflection.h>

namespace kw {

class HeightFieldManager;
class PhysicsManager;

struct PhysicsPrimitiveReflectionDescriptor {
    PhysicsManager* physics_manager;
    HeightFieldManager* height_field_manager;
    PrefabManager* prefab_manager;
    MemoryResource* memory_resource;
};

class PhysicsPrimitiveReflection : public virtual PrimitiveReflection {
public:
    explicit PhysicsPrimitiveReflection(const PhysicsPrimitiveReflectionDescriptor& descriptor);

    PhysicsManager& physics_manager;
    HeightFieldManager& height_field_manager;
};

} // namespace kw
