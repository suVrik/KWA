#include "render/geometry/geometry_notifier.h"
#include "render/geometry/geometry_listener.h"

namespace kw {

GeometryNotifier::GeometryNotifier(MemoryResource& memory_resource)
    : m_memory_resource(memory_resource)
    , m_listeners(memory_resource)
{
}

void GeometryNotifier::subscribe(Geometry& geometry, GeometryListener& geometry_listener) {
    std::lock_guard lock(m_mutex);

    auto it = m_listeners.find(&geometry);
    if (it != m_listeners.end()) {
        it->second.push_back(&geometry_listener);
    } else {
        m_listeners.emplace(&geometry, Vector<GeometryListener*>(1, &geometry_listener, m_memory_resource));
    }
}

void GeometryNotifier::unsubscribe(Geometry& geometry, GeometryListener& geometry_listener) {
    std::lock_guard lock(m_mutex);

    auto it1 = m_listeners.find(&geometry);
    if (it1 != m_listeners.end()) {
        auto it2 = std::find(it1->second.begin(), it1->second.end(), &geometry_listener);
        if (it2 != it1->second.end()) {
            std::swap(*it2, it1->second.back());
            it1->second.pop_back();
        }
    }
}

void GeometryNotifier::notify(Geometry& geometry) {
    std::lock_guard lock(m_mutex);

    auto it = m_listeners.find(&geometry);
    if (it != m_listeners.end()) {
        for (GeometryListener* geometry_listener : it->second) {
            geometry_listener->geometry_loaded();
        }
        m_listeners.erase(it);
    }
}

} // namespace kw
