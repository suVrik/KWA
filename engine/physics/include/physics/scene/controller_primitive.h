#pragma once

#include "physics/physics_ptr.h"

#include <core/scene/primitive.h>
#include <core/utils/enum_utils.h>

namespace physx {

class PxController;

} // namespace physx

namespace kw {

enum class ControllerCollision {
    NONE            = 0,
    COLLISION_SIDES = 1 << 0,
    COLLISION_UP    = 1 << 1,
    COLLISION_DOWN  = 1 << 2,
};

KW_DEFINE_ENUM_BITMASK(ControllerCollision);

class ControllerPrimitive : public Primitive {
public:
    explicit ControllerPrimitive(float step_offset = 0.5f,
                                 const transform& local_transform = transform());
    ControllerPrimitive(const ControllerPrimitive& other);
    ControllerPrimitive(ControllerPrimitive&& other) noexcept;
    ~ControllerPrimitive() override;
    ControllerPrimitive& operator=(const ControllerPrimitive& other);
    ControllerPrimitive& operator=(ControllerPrimitive&& other) noexcept;

    ControllerCollision move(const float3& offset);

    float get_step_offset() const;
    void set_step_offset(float value);

protected:
    void global_transform_updated() override;

    PhysicsPtr<physx::PxController> m_controller;

private:
    float m_step_offset;

    friend class PhysicsScene;
};

} // namespace kw
