#include "game_scene.h"

namespace kw {

GameScene::GameScene(const GameSceneDescriptor& descriptor)
    : Scene(SceneDescriptor{
        descriptor.persistent_memory_resource, descriptor.transient_memory_resource
    })
    , RenderScene(RenderSceneDescriptor{
        descriptor.animation_player, descriptor.particle_system_player, descriptor.reflection_probe_manager,
        descriptor.geometry_acceleration_structure, descriptor.light_acceleration_structure,
        descriptor.particle_system_acceleration_structure, descriptor.reflection_probe_acceleration_structure,
        descriptor.persistent_memory_resource, descriptor.transient_memory_resource
    })
    , PhysicsScene(PhysicsSceneDescriptor{
        descriptor.physics_manager, descriptor.persistent_memory_resource, descriptor.transient_memory_resource
    })
{
}

void GameScene::child_added(Primitive& primitive) {
    RenderScene::child_added(primitive);
    PhysicsScene::child_added(primitive);
    Scene::child_added(primitive);
}

} // namespace kw
