#pragma once

#include "physics/physics_ptr.h"

#include <core/scene/primitive.h>

namespace physx {

class PxRigidActor;

} // namespace physx

namespace kw {

class PhysicsManager;

class RigidActorPrimitive : public Primitive {
public:
    RigidActorPrimitive();
    explicit RigidActorPrimitive(PhysicsManager& physics_manager,
                                 const transform& local_transform = transform());
    RigidActorPrimitive(const RigidActorPrimitive& other);
    RigidActorPrimitive(RigidActorPrimitive&& other);
    ~RigidActorPrimitive() override;
    RigidActorPrimitive& operator=(const RigidActorPrimitive& other);
    RigidActorPrimitive& operator=(RigidActorPrimitive&& other);

    PhysicsManager* get_physics_manager() const;
    physx::PxRigidActor* get_rigid_actor() const;

protected:
    void global_transform_updated() override;

private:
    physx::PxRigidActor* create_rigid_actor();

    PhysicsManager* m_physics_manager;
    PhysicsPtr<physx::PxRigidActor> m_rigid_actor;
};

} // namespace kw
