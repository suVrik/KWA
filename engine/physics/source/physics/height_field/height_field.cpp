#include "physics/height_field/height_field.h"
#include "physics/height_field/height_field_notifier.h"

#include <geometry/PxHeightField.h>

#include <core/debug/assert.h>

namespace kw {

HeightField::HeightField(HeightFieldNotifier& height_field_notifier)
    : m_height_field_notifier(height_field_notifier)
{
}

HeightField::HeightField(HeightFieldNotifier& height_field_notifier, PhysicsPtr<physx::PxHeightField>&& height_field)
    : m_height_field_notifier(height_field_notifier)
    , m_height_field(std::move(height_field))
{
}

HeightField::HeightField(HeightField&& other) noexcept
    : m_height_field_notifier(other.m_height_field_notifier)
    , m_height_field(std::move(other.m_height_field))
{
    // Guarantees that there's no listeners subscribed to `other`.
    KW_ASSERT(other.is_loaded(), "The move source is expected to be loaded.");
}

HeightField::~HeightField() = default;

HeightField& HeightField::operator=(HeightField&& other) noexcept {
    if (&other != this) {
        KW_ASSERT(&m_height_field_notifier == &other.m_height_field_notifier, "Mismatching height field notifiers.");

        // We want our resources to go from unloaded to loaded and never vice versa.
        KW_ASSERT(!is_loaded(), "The move destination is expected to be unloaded.");
        KW_ASSERT(other.is_loaded(), "The move source is expected to be loaded.");

        m_height_field = std::move(other.m_height_field);
    }

    return *this;
}

void HeightField::subscribe(HeightFieldListener& height_field_listener) {
    m_height_field_notifier.subscribe(*this, height_field_listener);
}

void HeightField::unsubscribe(HeightFieldListener& height_field_listener) {
    m_height_field_notifier.unsubscribe(*this, height_field_listener);
}

physx::PxHeightField* HeightField::get_height_field() {
    return m_height_field.get();
}

bool HeightField::is_loaded() const {
    return static_cast<bool>(m_height_field);
}

} // namespace kw
