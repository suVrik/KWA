#include "physics/scene/height_field_rigid_actor_primitive.h"
#include "physics/height_field/height_field.h"
#include "physics/height_field/height_field_manager.h"
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

UniquePtr<Primitive> HeightFieldRigidActorPrimitive::create_from_markdown(PrimitiveReflection& reflection, const ObjectNode& node) {
    PhysicsPrimitiveReflection& physics_reflection = dynamic_cast<PhysicsPrimitiveReflection&>(reflection);

    StringNode& height_field_node = node["height_field"].as<StringNode>();

    SharedPtr<HeightField> height_field = physics_reflection.height_field_manager.load(height_field_node.get_value().c_str());
    uint32_t collision_category = static_cast<uint32_t>(node["collision_category"].as<NumberNode>().get_value());
    uint32_t collision_mask = static_cast<uint32_t>(node["collision_mask"].as<NumberNode>().get_value());
    transform local_transform = MarkdownUtils::transform_from_markdown(node["local_transform"]);

    return static_pointer_cast<Primitive>(allocate_unique<HeightFieldRigidActorPrimitive>(
        reflection.memory_resource, physics_reflection.physics_manager, height_field, collision_category, collision_mask, local_transform
    ));
}

HeightFieldRigidActorPrimitive::HeightFieldRigidActorPrimitive(PhysicsManager& physics_manager, SharedPtr<HeightField> height_field,
                                                               uint32_t collision_category, uint32_t collision_mask,
                                                               const transform& local_transform)
    : RigidActorPrimitive(physics_manager, local_transform)
    , m_height_field(std::move(height_field))
    , m_collision_category(collision_category)
    , m_collision_mask(collision_mask)
{
    if (m_height_field) {
        m_height_field->subscribe(*this);
    }
}

HeightFieldRigidActorPrimitive::HeightFieldRigidActorPrimitive(const HeightFieldRigidActorPrimitive& other)
    : RigidActorPrimitive(other)
    , m_height_field(other.m_height_field)
    , m_collision_category(other.m_collision_category)
    , m_collision_mask(other.m_collision_mask)
{
    if (m_height_field) {
        m_height_field->subscribe(*this);
    }
}

HeightFieldRigidActorPrimitive::HeightFieldRigidActorPrimitive(HeightFieldRigidActorPrimitive&& other)
    : RigidActorPrimitive(std::move(other))
    , m_collision_category(other.m_collision_category)
    , m_collision_mask(other.m_collision_mask)
{
    if (other.m_shape) {
        m_shape = std::move(other.m_shape);
        m_shape->userData = this;

        KW_ASSERT(m_shape->getActor() == get_rigid_actor(), "Shape is expected to be attached.");
    }

    if (other.m_height_field) {
        other.m_height_field->unsubscribe(other);

        m_height_field = std::move(other.m_height_field);

        m_height_field->subscribe(*this);
    }
}

HeightFieldRigidActorPrimitive::~HeightFieldRigidActorPrimitive() {
    if (m_height_field) {
        m_height_field->unsubscribe(*this);
    }
}

HeightFieldRigidActorPrimitive& HeightFieldRigidActorPrimitive::operator=(const HeightFieldRigidActorPrimitive& other) {
    if (&other != this) {
        RigidActorPrimitive::operator=(other);

        if (m_height_field) {
            m_height_field->unsubscribe(*this);
        }

        m_height_field = other.m_height_field;
        m_collision_category = other.m_collision_category;
        m_collision_mask = other.m_collision_mask;

        if (m_height_field) {
            m_height_field->subscribe(*this);
        }
    }

    return *this;
}

HeightFieldRigidActorPrimitive& HeightFieldRigidActorPrimitive::operator=(HeightFieldRigidActorPrimitive&& other) {
    if (&other != this) {
        RigidActorPrimitive::operator=(std::move(other));

        if (m_height_field) {
            m_height_field->unsubscribe(*this);
        }

        if (other.m_shape) {
            m_shape = std::move(other.m_shape);
            m_shape->userData = this;

            KW_ASSERT(m_shape->getActor() == get_rigid_actor(), "Shape is expected to be attached.");
        }

        if (other.m_height_field) {
            other.m_height_field->unsubscribe(other);

            m_height_field = std::move(other.m_height_field);
            m_collision_category = other.m_collision_category;
            m_collision_mask = other.m_collision_mask;

            m_height_field->subscribe(*this);
        }
    }

    return *this;
}

const SharedPtr<HeightField>& HeightFieldRigidActorPrimitive::get_height_field() const {
    return m_height_field;
}

void HeightFieldRigidActorPrimitive::set_height_field(SharedPtr<HeightField> value) {
    m_height_field = std::move(value);
}

physx::PxShape* HeightFieldRigidActorPrimitive::get_shape() const {
    return m_shape.get();
}

UniquePtr<Primitive> HeightFieldRigidActorPrimitive::clone(MemoryResource& memory_resource) const {
    return static_pointer_cast<Primitive>(allocate_unique<HeightFieldRigidActorPrimitive>(memory_resource, *this));
}

void HeightFieldRigidActorPrimitive::global_transform_updated() {
    KW_ASSERT(get_physics_manager() != nullptr, "Invalid primitives must not be used.");

    if (m_shape && m_height_field && m_height_field->is_loaded()) {
        m_shape->setGeometry(create_geometry());
    }

    RigidActorPrimitive::global_transform_updated();
}

void HeightFieldRigidActorPrimitive::height_field_loaded() {
    KW_ASSERT(get_physics_manager() != nullptr, "Height field actor primitive is expected to be valid.");
    KW_ASSERT(m_height_field && m_height_field->is_loaded(), "Height field is expected to be loaded.");

    if (m_shape) {
        m_shape->setGeometry(create_geometry());
        m_shape->setSimulationFilterData(physx::PxFilterData(m_collision_category, m_collision_mask, 0, 0));
        m_shape->setQueryFilterData(physx::PxFilterData(m_collision_category, m_collision_mask, 0, 0));

        KW_ASSERT(m_shape->userData == this, "User data is expected to be set.");
        KW_ASSERT(m_shape->getActor() == get_rigid_actor(), "Shape is expected to be attached.");
    } else {
        m_shape = get_physics_manager()->get_physics().createShape(create_geometry(), get_physics_manager()->get_default_material(), true);
        m_shape->setSimulationFilterData(physx::PxFilterData(m_collision_category, m_collision_mask, 0, 0));
        m_shape->setQueryFilterData(physx::PxFilterData(m_collision_category, m_collision_mask, 0, 0));
        m_shape->userData = this;

        get_rigid_actor()->attachShape(*m_shape);
    }
}

physx::PxHeightFieldGeometry HeightFieldRigidActorPrimitive::create_geometry() const {
    KW_ASSERT(m_height_field && m_height_field->is_loaded(), "Height field is expected to be loaded.");
    
    const float3& scale = get_global_scale();

    // TODO: Take the number of columns and rows here into account? Heightfield scale is heightfield scale, not row and column scale?
    return physx::PxHeightFieldGeometry(m_height_field->get_height_field(), physx::PxMeshGeometryFlags(), scale.y / INT16_MAX, scale.x, scale.z);
}

} // namespace kw
