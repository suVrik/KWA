#include "game_primitive_reflection.h"

namespace kw {

GamePrimitiveReflection::GamePrimitiveReflection(const GamePrimitiveReflectionDescriptor& descriptor)
    : PrimitiveReflection(PrimitiveReflectionDescriptor{
        descriptor.prefab_manager, descriptor.memory_resource
    })
    , RenderPrimitiveReflection(RenderPrimitiveReflectionDescriptor{
        descriptor.texture_manager, descriptor.geometry_manager, descriptor.material_manager,
        descriptor.animation_manager, descriptor.motion_graph_manager, descriptor.particle_system_manager,
        descriptor.prefab_manager, descriptor.memory_resource
    })
    , PhysicsPrimitiveReflection(PhysicsPrimitiveReflectionDescriptor{
        descriptor.physics_manager, descriptor.height_field_manager, descriptor.prefab_manager,
        descriptor.memory_resource
    })
{
}

} // namespace kw
