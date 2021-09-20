#pragma once

#include "physics/physics_ptr.h"

namespace physx {

class PxHeightField;

} // namespace physx

namespace kw {

class HeightFieldListener;
class HeightFieldNotifier;

class HeightField {
public:
    explicit HeightField(HeightFieldNotifier& height_field_notifier);
    HeightField(HeightFieldNotifier& height_field_notifier, PhysicsPtr<physx::PxHeightField>&& height_field);
    HeightField(HeightField&& other) noexcept;
    ~HeightField();
    HeightField& operator=(HeightField&& other) noexcept;

    // This height field listener will be notified when this height field is loaded.
    // If this height field is already loaded, the listener will be notified immediately.
    void subscribe(HeightFieldListener& geometry_listener);
    void unsubscribe(HeightFieldListener& geometry_listener);

    physx::PxHeightField* get_height_field();

    bool is_loaded() const;

private:
    HeightFieldNotifier& m_height_field_notifier;

    PhysicsPtr<physx::PxHeightField> m_height_field;
};

} // namespace kw
