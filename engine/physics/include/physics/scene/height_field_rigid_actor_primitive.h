#pragma once

#include "physics/height_field/height_field_listener.h"
#include "physics/scene/rigid_actor_primitive.h"

#include <core/containers/shared_ptr.h>

namespace physx {

class PxHeightFieldGeometry;
class PxShape;

} // namespace physx

namespace kw {

class HeightField;
class ObjectNode;
class PhysicsManager;
class PrimitiveReflection;

class HeightFieldRigidActorPrimitive : public RigidActorPrimitive, public HeightFieldListener {
public:
    static UniquePtr<Primitive> create_from_markdown(PrimitiveReflection& reflection, const ObjectNode& node);

    HeightFieldRigidActorPrimitive() = default;
    explicit HeightFieldRigidActorPrimitive(PhysicsManager& physics_manager,
                                            SharedPtr<HeightField> height_field = nullptr,
                                            uint32_t collision_category = 1,
                                            uint32_t collision_mask = 0xFFFFFFFFU,
                                            const transform& local_transform = transform());
    HeightFieldRigidActorPrimitive(const HeightFieldRigidActorPrimitive& other);
    HeightFieldRigidActorPrimitive(HeightFieldRigidActorPrimitive&& other);
    ~HeightFieldRigidActorPrimitive() override;
    HeightFieldRigidActorPrimitive& operator=(const HeightFieldRigidActorPrimitive& other);
    HeightFieldRigidActorPrimitive& operator=(HeightFieldRigidActorPrimitive&& other);
    
    const SharedPtr<HeightField>& get_height_field() const;
    void set_height_field(SharedPtr<HeightField> value);

    physx::PxShape* get_shape() const;

    UniquePtr<Primitive> clone(MemoryResource& memory_resource) const override;

protected:
    void global_transform_updated() override;
    void height_field_loaded() override;

private:
    physx::PxHeightFieldGeometry create_geometry() const;

    SharedPtr<HeightField> m_height_field;
    PhysicsPtr<physx::PxShape> m_shape;

    // TODO: Expose getters and setters.
    uint32_t m_collision_category;
    uint32_t m_collision_mask;
};

} // namespace kw
