#pragma once

#include "physics/physics_ptr.h"

#include <core/containers/pair.h>
#include <core/scene/scene.h>

#include <optional>

namespace physx {

class PxControllerManager;
class PxScene;

} // namespace physx

namespace kw {

class PhysicsManager;
class Task;

struct PhysicsSceneDescriptor {
    PhysicsManager* physics_manager;

    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

struct QueryResult {
    float3 position;
    float3 normal;
    float distance;
};

class PhysicsScene : public virtual Scene {
public:
    explicit PhysicsScene(const PhysicsSceneDescriptor& descriptor);
    ~PhysicsScene() override;

    // TODO: Configurable filters. Static/dynamic. Any hit. Multiple hits. More sweep shapes. Overlaps.
    std::optional<QueryResult> raycast(const float3& origin, const float3& direction, float max_distance, uint32_t collision_mask = 0xFFFFFFFFU);
    std::optional<QueryResult> sweep_box(const transform& transform, const float3& direction, float max_distance, uint32_t collision_mask = 0xFFFFFFFFU);

    Pair<Task*, Task*> create_tasks();

protected:
    void child_added(Primitive& primitive) override;
    void child_removed(Primitive& primitive) override;

private:
    class BeginTask;
    class EndTask;

    PhysicsManager& m_physics_manager;

    PhysicsPtr<physx::PxScene> m_scene;
    PhysicsPtr<physx::PxControllerManager> m_controller_manager;
};

} // namespace kw
