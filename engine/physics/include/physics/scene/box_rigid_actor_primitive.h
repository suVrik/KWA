#pragma once

#include "physics/scene/rigid_actor_primitive.h"

namespace physx {

class PxBoxGeometry;
class PxShape;

} // namespace physx

namespace kw {

class ObjectNode;
class PhysicsManager;
class PrimitiveReflection;

class BoxRigidActorPrimitive : public RigidActorPrimitive {
public:
    static UniquePtr<Primitive> create_from_markdown(PrimitiveReflection& reflection, const ObjectNode& node);

    BoxRigidActorPrimitive() = default;
    explicit BoxRigidActorPrimitive(PhysicsManager& physics_manager,
                                    uint32_t collision_category = 1,
                                    uint32_t collision_mask = 0xFFFFFFFFU,
                                    const transform& local_transform = transform());
    BoxRigidActorPrimitive(const BoxRigidActorPrimitive& other);
    BoxRigidActorPrimitive(BoxRigidActorPrimitive&& other) noexcept;
    ~BoxRigidActorPrimitive() override;
    BoxRigidActorPrimitive& operator=(const BoxRigidActorPrimitive& other);
    BoxRigidActorPrimitive& operator=(BoxRigidActorPrimitive&& other) noexcept;
    
    physx::PxShape* get_shape() const;

    UniquePtr<Primitive> clone(MemoryResource& memory_resource) const override;

protected:
    void global_transform_updated() override;

private:
    physx::PxBoxGeometry create_geometry();
    physx::PxShape* create_shape();

    PhysicsPtr<physx::PxShape> m_shape;

    // TODO: Expose getters and setters.
    uint32_t m_collision_category;
    uint32_t m_collision_mask;
};

} // namespace kw
