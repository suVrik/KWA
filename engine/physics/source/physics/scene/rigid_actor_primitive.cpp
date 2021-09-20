#include "physics/scene/rigid_actor_primitive.h"
#include "physics/physics_manager.h"
#include "physics/physics_utils.h"

#include <core/debug/assert.h>

#include <PxPhysics.h>
#include <PxRigidStatic.h>

namespace kw {

RigidActorPrimitive::RigidActorPrimitive()
    : m_physics_manager(nullptr)
{
}

RigidActorPrimitive::RigidActorPrimitive(PhysicsManager& physics_manager, const transform& local_transform)
    : Primitive(local_transform)
    , m_physics_manager(&physics_manager)
{
    m_rigid_actor = create_rigid_actor();
}

RigidActorPrimitive::RigidActorPrimitive(const RigidActorPrimitive& other)
    : Primitive(other)
    , m_physics_manager(other.m_physics_manager)
{
    KW_ASSERT(m_physics_manager != nullptr, "Invalid primitives must not be copied.");

    m_rigid_actor = create_rigid_actor();
}

RigidActorPrimitive::RigidActorPrimitive(RigidActorPrimitive&& other)
    : Primitive(std::move(other))
    , m_physics_manager(other.m_physics_manager)
{
    KW_ASSERT(m_physics_manager != nullptr, "Invalid primitives must not be moved.");

    other.m_physics_manager = nullptr;

    m_rigid_actor = std::move(other.m_rigid_actor);
    m_rigid_actor->userData = this;
}

RigidActorPrimitive::~RigidActorPrimitive() = default;

RigidActorPrimitive& RigidActorPrimitive::operator=(const RigidActorPrimitive& other) {
    if (&other != this) {
        Primitive::operator=(other);

        KW_ASSERT(m_physics_manager != nullptr, "Invalid primitives must not be copied.");

        m_physics_manager = other.m_physics_manager;

        if (m_rigid_actor) {
            m_rigid_actor->setGlobalPose(PhysicsUtils::kw_to_physx(get_global_transform()));
            KW_ASSERT(m_rigid_actor->userData == this, "User data is expected to be set.");
        } else {
            m_rigid_actor = create_rigid_actor();
        }
    }

    return *this;
}

RigidActorPrimitive& RigidActorPrimitive::operator=(RigidActorPrimitive&& other) {
    if (&other != this) {
        Primitive::operator=(std::move(other));

        KW_ASSERT(other.m_physics_manager != nullptr, "Invalid primitives must not be moved.");

        m_physics_manager = other.m_physics_manager;

        m_rigid_actor = std::move(other.m_rigid_actor);
        m_rigid_actor->userData = this;

        other.m_physics_manager = nullptr;
    }

    return *this;
}

PhysicsManager* RigidActorPrimitive::get_physics_manager() const {
    return m_physics_manager;
}

physx::PxRigidActor* RigidActorPrimitive::get_rigid_actor() const {
    return m_rigid_actor.get();
}

void RigidActorPrimitive::global_transform_updated() {
    KW_ASSERT(m_physics_manager != nullptr, "Invalid primitives must not be used.");

    m_rigid_actor->setGlobalPose(PhysicsUtils::kw_to_physx(get_global_transform()));
}

physx::PxRigidActor* RigidActorPrimitive::create_rigid_actor() {
    physx::PxRigidActor* result = m_physics_manager->get_physics().createRigidStatic(PhysicsUtils::kw_to_physx(get_global_transform()));
    result->userData = this;
    return result;
}

} // namespace kw
