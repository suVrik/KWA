#include "physics/scene/controller_primitive.h"
#include "physics/physics_utils.h"

#include <core/debug/assert.h>

#include <characterkinematic/PxController.h>

namespace kw {

ControllerPrimitive::ControllerPrimitive(float step_offset, const transform& local_transform)
    : Primitive(local_transform)
    , m_step_offset(step_offset)
{
}

ControllerPrimitive::ControllerPrimitive(const ControllerPrimitive& other)
    : Primitive(other)
    , m_step_offset(other.m_step_offset)
{
    KW_ASSERT(other.m_controller == nullptr, "Controller is expected to be unset.");
}

ControllerPrimitive::ControllerPrimitive(ControllerPrimitive&& other) noexcept
    : Primitive(std::move(other))
    , m_step_offset(other.m_step_offset)
{
    KW_ASSERT(other.m_controller == nullptr, "Controller is expected to be unset.");
}

ControllerPrimitive::~ControllerPrimitive() = default;

ControllerPrimitive& ControllerPrimitive::operator=(const ControllerPrimitive& other) {
    if (&other != this) {
        Primitive::operator=(other);

        KW_ASSERT(other.m_controller == nullptr, "Controller is expected to be unset.");

        m_step_offset = other.m_step_offset;
    }

    return *this;
}

ControllerPrimitive& ControllerPrimitive::operator=(ControllerPrimitive&& other) noexcept {
    if (&other != this) {
        Primitive::operator=(std::move(other));

        KW_ASSERT(other.m_controller == nullptr, "Controller is expected to be unset.");

        m_step_offset = other.m_step_offset;
    }

    return *this;
}


ControllerCollision ControllerPrimitive::move(const float3& offset) {
    if (m_controller) {
        // TODO: Configurable collision callbacks. Static/dynamic flags.
        physx::PxControllerFilters controller_filters;
        controller_filters.mFilterFlags = physx::PxQueryFlag::eSTATIC;

        // TODO: Query elapsed time.
        physx::PxControllerCollisionFlags result = m_controller->move(PhysicsUtils::kw_to_physx(offset), 0.01f, 1.f / 60.f, controller_filters, nullptr);

        set_global_translation(PhysicsUtils::physx_extended_to_kw(m_controller->getFootPosition()));

        return static_cast<ControllerCollision>(static_cast<uint32_t>(result));
    }

    return ControllerCollision::NONE;
}

float ControllerPrimitive::get_step_offset() const {
    return m_step_offset;
}

void ControllerPrimitive::set_step_offset(float value) {
    m_step_offset = value;

    if (m_controller) {
        m_controller->setStepOffset(m_step_offset);
    }
}

void ControllerPrimitive::global_transform_updated() {
    if (m_controller) {
        m_controller->setFootPosition(PhysicsUtils::kw_to_physx_extended(get_global_translation()));
    }
}

} // namespace kw
