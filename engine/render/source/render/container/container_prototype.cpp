#include "render/container/container_prototype.h"
#include "render/container/container_prototype_listener.h"
#include "render/container/container_prototype_notifier.h"
#include "render/scene/primitive.h"

#include <core/debug/assert.h>

namespace kw {

ContainerPrototype::ContainerPrototype(ContainerPrototypeNotifier& container_prototype_notifier, MemoryResource& persistent_memory_resource)
    : m_container_prototype_notifier(container_prototype_notifier)
    , m_primitives(persistent_memory_resource)
{
}

ContainerPrototype::ContainerPrototype(ContainerPrototypeNotifier& container_prototype_notifier, Vector<UniquePtr<Primitive>>&& primitives)
    : m_container_prototype_notifier(container_prototype_notifier)
    , m_primitives(std::move(primitives))
{
}

ContainerPrototype::ContainerPrototype(ContainerPrototype&& other) = default;
ContainerPrototype::~ContainerPrototype() = default;

ContainerPrototype& ContainerPrototype::operator=(ContainerPrototype&& other) {
    KW_ASSERT(&m_container_prototype_notifier == &other.m_container_prototype_notifier);
    m_primitives = std::move(other.m_primitives);
    return *this;
}

void ContainerPrototype::subscribe(ContainerPrototypeListener& container_prototype_listener) {
    if (is_loaded()) {
        container_prototype_listener.container_prototype_loaded();
    } else {
        m_container_prototype_notifier.subscribe(*this, container_prototype_listener);
    }
}

void ContainerPrototype::unsubscribe(ContainerPrototypeListener& container_prototype_listener) {
    if (!is_loaded()) {
        m_container_prototype_notifier.unsubscribe(*this, container_prototype_listener);
    }
}

const Vector<UniquePtr<Primitive>>& ContainerPrototype::get_primitives() const {
    return m_primitives;
}

bool ContainerPrototype::is_loaded() const {
    return !m_primitives.empty();
}

} // namespace kw
