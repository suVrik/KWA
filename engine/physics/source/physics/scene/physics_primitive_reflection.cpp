#include "physics/scene/physics_primitive_reflection.h"
#include "physics/scene/box_rigid_actor_primitive.h"
#include "physics/scene/capsule_controller_primitive.h"
#include "physics/scene/height_field_rigid_actor_primitive.h"

#include <core/debug/assert.h>

namespace kw {

#define KW_NAME_AND_CALLBACK(Type) String(#Type, MallocMemoryResource::instance()), &Type::create_from_markdown

PhysicsPrimitiveReflection::PhysicsPrimitiveReflection(const PhysicsPrimitiveReflectionDescriptor& descriptor)
    : PrimitiveReflection(PrimitiveReflectionDescriptor{ descriptor.prefab_manager, descriptor.memory_resource })
    , physics_manager(*descriptor.physics_manager)
    , height_field_manager(*descriptor.height_field_manager)
{
    KW_ASSERT(descriptor.physics_manager != nullptr, "Invalid physics manager.");
    KW_ASSERT(descriptor.height_field_manager != nullptr, "Invalid height field manager.");

    m_primitives.emplace(KW_NAME_AND_CALLBACK(BoxRigidActorPrimitive));
    m_primitives.emplace(KW_NAME_AND_CALLBACK(CapsuleControllerPrimitive));
    m_primitives.emplace(KW_NAME_AND_CALLBACK(HeightFieldRigidActorPrimitive));
}

#undef KW_NAME_AND_CALLBACK

} // namespace kw
