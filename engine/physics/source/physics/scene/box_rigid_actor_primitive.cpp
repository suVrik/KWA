#include "physics/scene/box_rigid_actor_primitive.h"
#include "physics/physics_manager.h"
#include "physics/physics_utils.h"
#include "physics/scene/physics_primitive_reflection.h"

#include <core/debug/assert.h>
#include <core/io/markdown.h>
#include <core/io/markdown_utils.h>

#include <PxFiltering.h>
#include <PxPhysics.h>
#include <PxRigidActor.h>
#include <PxShape.h>

namespace kw {

UniquePtr<Primitive> BoxRigidActorPrimitive::create_from_markdown(PrimitiveReflection& reflection, const ObjectNode& node) {
    PhysicsPrimitiveReflection& physics_reflection = dynamic_cast<PhysicsPrimitiveReflection&>(reflection);

    uint32_t collision_category = static_cast<uint32_t>(node["collision_category"].as<NumberNode>().get_value());
    uint32_t collision_mask = static_cast<uint32_t>(node["collision_mask"].as<NumberNode>().get_value());
    transform local_transform = MarkdownUtils::transform_from_markdown(node["local_transform"]);

    return static_pointer_cast<Primitive>(allocate_unique<BoxRigidActorPrimitive>(
        reflection.memory_resource, physics_reflection.physics_manager, collision_category, collision_mask, local_transform
    ));
}

BoxRigidActorPrimitive::BoxRigidActorPrimitive(PhysicsManager& physics_manager,
                                               uint32_t collision_category, uint32_t collision_mask,
                                               const transform& local_transform)
    : RigidActorPrimitive(physics_manager, local_transform)
    , m_collision_category(collision_category)
    , m_collision_mask(collision_mask)
{
    m_shape = create_shape();
}

BoxRigidActorPrimitive::BoxRigidActorPrimitive(const BoxRigidActorPrimitive& other)
    : RigidActorPrimitive(other)
    , m_collision_category(other.m_collision_category)
    , m_collision_mask(other.m_collision_mask)
{
    // Physics manager and rigid actor are guaranteed to be valid because the `RigidActorPrimitive` copy constructor guarantees that.

    m_shape = create_shape();
}

BoxRigidActorPrimitive::BoxRigidActorPrimitive(BoxRigidActorPrimitive&& other) noexcept
    : RigidActorPrimitive(std::move(other))
    , m_collision_category(other.m_collision_category)
    , m_collision_mask(other.m_collision_mask)
{
    // Shape and rigid actor are guaranteed to be valid because the `RigidActorPrimitive` move constructor guarantees that.

    m_shape = std::move(other.m_shape);
    m_shape->setSimulationFilterData(physx::PxFilterData(m_collision_category, m_collision_mask, 0, 0));
    m_shape->setQueryFilterData(physx::PxFilterData(m_collision_category, m_collision_mask, 0, 0));
    m_shape->userData = this;

    KW_ASSERT(m_shape->getActor() == get_rigid_actor(), "Shape is expected to be attached.");
}

BoxRigidActorPrimitive::~BoxRigidActorPrimitive() = default;

BoxRigidActorPrimitive& BoxRigidActorPrimitive::operator=(const BoxRigidActorPrimitive& other) {
    if (&other != this) {
        RigidActorPrimitive::operator=(other);

        // Physics manager and rigid actor are guaranteed to be valid because the `RigidActorPrimitive` copy assignment operator guarantees that.

        m_collision_category = other.m_collision_category;
        m_collision_mask = other.m_collision_mask;

        if (m_shape) {
            m_shape->setGeometry(create_geometry());
            m_shape->setSimulationFilterData(physx::PxFilterData(m_collision_category, m_collision_mask, 0, 0));
            m_shape->setQueryFilterData(physx::PxFilterData(m_collision_category, m_collision_mask, 0, 0));
            KW_ASSERT(m_shape->userData == this, "User data is expected to be set.");
            KW_ASSERT(m_shape->getActor() == get_rigid_actor(), "Shape is expected to be attached.");
        } else {
            m_shape = create_shape();
        }
    }

    return *this;
}

BoxRigidActorPrimitive& BoxRigidActorPrimitive::operator=(BoxRigidActorPrimitive&& other) noexcept {
    if (&other != this) {
        RigidActorPrimitive::operator=(std::move(other));

        // Shape and rigid actor are guaranteed to be valid because the `RigidActorPrimitive` move assignment operator guarantees that.

        m_collision_category = other.m_collision_category;
        m_collision_mask = other.m_collision_mask;

        m_shape = std::move(other.m_shape);
        m_shape->setSimulationFilterData(physx::PxFilterData(m_collision_category, m_collision_mask, 0, 0));
        m_shape->setQueryFilterData(physx::PxFilterData(m_collision_category, m_collision_mask, 0, 0));
        m_shape->userData = this;

        KW_ASSERT(m_shape->getActor() == get_rigid_actor(), "Shape is expected to be attached.");
    }

    return *this;
}

physx::PxShape* BoxRigidActorPrimitive::get_shape() const {
    return m_shape.get();
}

UniquePtr<Primitive> BoxRigidActorPrimitive::clone(MemoryResource& memory_resource) const {
    return static_pointer_cast<Primitive>(allocate_unique<BoxRigidActorPrimitive>(memory_resource, *this));
}

void BoxRigidActorPrimitive::global_transform_updated() {
    KW_ASSERT(get_physics_manager() != nullptr, "Invalid primitives must not be used.");

    m_shape->setGeometry(create_geometry());

    RigidActorPrimitive::global_transform_updated();
}

physx::PxBoxGeometry BoxRigidActorPrimitive::create_geometry() {
    return physx::PxBoxGeometry(PhysicsUtils::kw_to_physx(get_global_scale()));
}

physx::PxShape* BoxRigidActorPrimitive::create_shape() {
    physx::PxShape* result = get_physics_manager()->get_physics().createShape(create_geometry(), get_physics_manager()->get_default_material(), true);
    result->setSimulationFilterData(physx::PxFilterData(m_collision_category, m_collision_mask, 0, 0));
    result->setQueryFilterData(physx::PxFilterData(m_collision_category, m_collision_mask, 0, 0));
    result->userData = this;
    
    get_rigid_actor()->attachShape(*result);

    return result;
}

} // namespace kw
