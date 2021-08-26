#include "render/container/container_prototype_notifier.h"
#include "render/container/container_prototype_listener.h"

namespace kw {

ContainerPrototypeNotifier::ContainerPrototypeNotifier(MemoryResource& memory_resource)
    : m_memory_resource(memory_resource)
    , m_listeners(memory_resource)
{
}

void ContainerPrototypeNotifier::subscribe(ContainerPrototype& container_prototype, ContainerPrototypeListener& container_prototype_listener) {
    std::lock_guard lock(m_mutex);

    auto it = m_listeners.find(&container_prototype);
    if (it != m_listeners.end()) {
        it->second.push_back(&container_prototype_listener);
    } else {
        m_listeners.emplace(&container_prototype, Vector<ContainerPrototypeListener*>(1, &container_prototype_listener, m_memory_resource));
    }
}

void ContainerPrototypeNotifier::unsubscribe(ContainerPrototype& container_prototype, ContainerPrototypeListener& container_prototype_listener) {
    std::lock_guard lock(m_mutex);

    auto it1 = m_listeners.find(&container_prototype);
    if (it1 != m_listeners.end()) {
        auto it2 = std::find(it1->second.begin(), it1->second.end(), &container_prototype_listener);
        if (it2 != it1->second.end()) {
            std::swap(*it2, it1->second.back());
            it1->second.pop_back();
        }
    }
}

void ContainerPrototypeNotifier::notify(ContainerPrototype& container_prototype) {
    std::lock_guard lock(m_mutex);

    auto it = m_listeners.find(&container_prototype);
    if (it != m_listeners.end()) {
        for (ContainerPrototypeListener* container_prototype_listener : it->second) {
            container_prototype_listener->container_prototype_loaded();
        }
        m_listeners.erase(it);
    }
}

} // namespace kw
