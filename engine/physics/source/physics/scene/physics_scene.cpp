#include "physics/scene/physics_scene.h"
#include "physics/physics_manager.h"
#include "physics/physics_utils.h"
#include "physics/scene/capsule_controller_primitive.h"
#include "physics/scene/rigid_actor_primitive.h"

#include <core/concurrency/task.h>
#include <core/debug/assert.h>
#include <core/error.h>

#include <PxFiltering.h>
#include <PxPhysics.h>
#include <PxRigidActor.h>
#include <PxScene.h>
#include <PxShape.h>
#include <characterkinematic/PxCapsuleController.h>
#include <characterkinematic/PxControllerManager.h>
#include <extensions/PxDefaultSimulationFilterShader.h>

namespace kw {

class QueryFilter : public physx::PxQueryFilterCallback {
public:
    physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData& filterData, const physx::PxShape* shape,
                                          const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags) override
    {
        return (shape->getQueryFilterData().word0 & filterData.word0) != 0 ? physx::PxQueryHitType::eBLOCK : physx::PxQueryHitType::eNONE;
    }

    physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData& filterData, const physx::PxQueryHit& hit) override {
        return physx::PxQueryHitType::eBLOCK;
    }
};

class PhysicsScene::EndTask : public Task {
public:
    EndTask(PhysicsScene& scene)
        : m_scene(scene)
    {
    }

    void run() override {
        m_scene.m_scene->fetchResults(true);
    }

    const char* get_name() const override {
        return "Physics Scene Fetch";
    }

private:
    PhysicsScene& m_scene;
};

class PhysicsScene::BeginTask : public Task {
public:
    BeginTask(PhysicsScene& scene)
        : m_scene(scene)
    {
    }

    void run() override {
        // TODO: Use real elapsed time.
        m_scene.m_scene->simulate(1.f / 60.f);
    }

    const char* get_name() const override {
        return "Physics Scene Simulate";
    }

private:
    PhysicsScene& m_scene;
};

PhysicsScene::PhysicsScene(const PhysicsSceneDescriptor& descriptor)
    : Scene(SceneDescriptor{ descriptor.persistent_memory_resource, descriptor.transient_memory_resource })
    , m_physics_manager(*descriptor.physics_manager)
{
    KW_ASSERT(descriptor.physics_manager != nullptr, "Invalid physics manager.");

    physx::PxTolerancesScale scale;

    physx::PxSceneDesc scene_descriptor(scale);
    scene_descriptor.gravity = physx::PxVec3(0.f, -9.8f, 0.f);
    scene_descriptor.filterShader = physx::PxDefaultSimulationFilterShader;
    scene_descriptor.cpuDispatcher = &m_physics_manager.get_cpu_dispatcher();
    scene_descriptor.simulationEventCallback = nullptr;

    KW_ASSERT(scene_descriptor.isValid(), "Invalid scene descriptor.");
    m_scene = m_physics_manager.get_physics().createScene(scene_descriptor);
    KW_ERROR(m_scene != nullptr, "Failed to create PhysX scene.");

    m_controller_manager = PxCreateControllerManager(*m_scene);
    KW_ERROR(m_controller_manager != nullptr, "Failed to create controller manager.");
}

PhysicsScene::~PhysicsScene() {
    // TODO: When `PrefabPrimitive` removes children, controller release crashes because controller manager is already
    //   destroyed at that point. Perhaps `PhysicsScene` shoudn't own a controller manager?
    while (!get_children().empty()) {
        remove_child(*get_children().front());
    }
}

void PhysicsScene::child_added(Primitive& primitive) {
    if (RigidActorPrimitive* rigid_actor_primitive = dynamic_cast<RigidActorPrimitive*>(&primitive)) {
        KW_ASSERT(rigid_actor_primitive->get_rigid_actor() != nullptr, "Invalid rigid actors must not be added to scene.");
        m_scene->addActor(*rigid_actor_primitive->get_rigid_actor());
    } else if (CapsuleControllerPrimitive* capsule_controller_primitive = dynamic_cast<CapsuleControllerPrimitive*>(&primitive)) {
        KW_ASSERT(capsule_controller_primitive->m_controller == nullptr, "Controller is expected to be unset.");

        physx::PxCapsuleControllerDesc capsule_descriptor;
        capsule_descriptor.stepOffset = capsule_controller_primitive->get_step_offset();
        capsule_descriptor.material = &m_physics_manager.get_default_material();
        capsule_descriptor.userData = static_cast<ControllerPrimitive*>(capsule_controller_primitive);
        capsule_descriptor.radius = capsule_controller_primitive->get_radius();
        capsule_descriptor.height = capsule_controller_primitive->get_height();

        KW_ASSERT(capsule_descriptor.isValid(), "Invalid capsule descriptor.");
        capsule_controller_primitive->m_controller = m_controller_manager->createController(capsule_descriptor);
        KW_ASSERT(capsule_controller_primitive->m_controller, "Failed to create controller.");

        capsule_controller_primitive->m_controller->setFootPosition(PhysicsUtils::kw_to_physx_extended(primitive.get_global_translation()));
    }
}

void PhysicsScene::child_removed(Primitive& primitive) {
    if (RigidActorPrimitive* rigid_actor_primitive = dynamic_cast<RigidActorPrimitive*>(&primitive)) {
        KW_ASSERT(rigid_actor_primitive->get_rigid_actor() != nullptr, "Unexpected invalid rigid actor.");
        m_scene->removeActor(*rigid_actor_primitive->get_rigid_actor());
    } else if (ControllerPrimitive* controller_primitive = dynamic_cast<ControllerPrimitive*>(&primitive)) {
        KW_ASSERT(controller_primitive->m_controller != nullptr, "Controller is expected to be set.");
        controller_primitive->m_controller.reset();
    }
}

std::optional<QueryResult> PhysicsScene::raycast(const float3& origin, const float3& direction, float max_distance, uint32_t collision_mask) {
    physx::PxRaycastBuffer hit;
    physx::PxQueryFilterData filter_data(physx::PxFilterData(collision_mask, 0, 0, 0), physx::PxQueryFlag::eSTATIC);
    QueryFilter query_filter;
    if (m_scene->raycast(PhysicsUtils::kw_to_physx(origin), PhysicsUtils::kw_to_physx(direction), max_distance, hit,
                         physx::PxHitFlag::eDEFAULT, filter_data, &query_filter))
    {
        QueryResult result;
        result.position = PhysicsUtils::physx_to_kw(hit.block.position);
        result.normal = PhysicsUtils::physx_to_kw(hit.block.normal);
        result.distance = hit.block.distance;
        return result;
    }
    return std::nullopt;
}

std::optional<QueryResult> PhysicsScene::sweep_box(const transform& transform, const float3& direction, float max_distance, uint32_t collision_mask) {
    physx::PxSweepBuffer hit;
    physx::PxQueryFilterData filter_data(physx::PxFilterData(collision_mask, 0, 0, 0), physx::PxQueryFlag::eSTATIC);
    QueryFilter query_filter;
    if (m_scene->sweep(physx::PxBoxGeometry(PhysicsUtils::kw_to_physx(transform.scale)),
                       PhysicsUtils::kw_to_physx(transform), PhysicsUtils::kw_to_physx(direction), max_distance, hit,
                       physx::PxHitFlag::eDEFAULT, filter_data, &query_filter))
    {
        QueryResult result;
        result.position = PhysicsUtils::physx_to_kw(hit.block.position);
        result.normal = PhysicsUtils::physx_to_kw(hit.block.normal);
        result.distance = hit.block.distance;
        return result;
    }
    return std::nullopt;
}

Pair<Task*, Task*> PhysicsScene::create_tasks() {
    Task* begin_task = m_transient_memory_resource.construct<BeginTask>(*this);
    Task* end_task = m_transient_memory_resource.construct<EndTask>(*this);

    return { begin_task, end_task };
}

} // namespace kw
