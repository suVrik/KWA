#include "core/prefab/prefab_prototype_notifier.h"
#include "core/prefab/prefab_prototype_listener.h"

namespace kw {

PrefabPrototypeNotifier::PrefabPrototypeNotifier(MemoryResource& memory_resource)
    : m_memory_resource(memory_resource)
    , m_listeners(memory_resource)
{
}

void PrefabPrototypeNotifier::subscribe(PrefabPrototype& prefab_prototype, PrefabPrototypeListener& prefab_prototype_listener) {
    std::lock_guard lock(m_mutex);

    auto it = m_listeners.find(&prefab_prototype);
    if (it != m_listeners.end()) {
        it->second.push_back(&prefab_prototype_listener);
    } else {
        m_listeners.emplace(&prefab_prototype, Vector<PrefabPrototypeListener*>(1, &prefab_prototype_listener, m_memory_resource));
    }
}

void PrefabPrototypeNotifier::unsubscribe(PrefabPrototype& prefab_prototype, PrefabPrototypeListener& prefab_prototype_listener) {
    std::lock_guard lock(m_mutex);

    auto it1 = m_listeners.find(&prefab_prototype);
    if (it1 != m_listeners.end()) {
        auto it2 = std::find(it1->second.begin(), it1->second.end(), &prefab_prototype_listener);
        if (it2 != it1->second.end()) {
            std::swap(*it2, it1->second.back());
            it1->second.pop_back();
        }
    }
}

void PrefabPrototypeNotifier::notify(PrefabPrototype& prefab_prototype) {
    std::lock_guard lock(m_mutex);

    auto it = m_listeners.find(&prefab_prototype);
    if (it != m_listeners.end()) {
        for (PrefabPrototypeListener* prefab_prototype_listener : it->second) {
            prefab_prototype_listener->prefab_prototype_loaded();
        }
        m_listeners.erase(it);
    }
}

} // namespace kw
