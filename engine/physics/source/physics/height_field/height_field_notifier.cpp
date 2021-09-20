#include "physics/height_field/height_field_notifier.h"
#include "physics/height_field/height_field.h"
#include "physics/height_field/height_field_listener.h"

namespace kw {

HeightFieldNotifier::HeightFieldNotifier(MemoryResource& memory_resource)
    : m_memory_resource(memory_resource)
    , m_listeners(memory_resource)
{
}

void HeightFieldNotifier::subscribe(HeightField& height_field, HeightFieldListener& height_field_listener) {
    std::lock_guard lock(m_mutex);

    if (height_field.is_loaded()) {
        height_field_listener.height_field_loaded();
        return;
    }

    auto it = m_listeners.find(&height_field);
    if (it != m_listeners.end()) {
        it->second.push_back(&height_field_listener);
    } else {
        m_listeners.emplace(&height_field, Vector<HeightFieldListener*>(1, &height_field_listener, m_memory_resource));
    }
}

void HeightFieldNotifier::unsubscribe(HeightField& height_field, HeightFieldListener& height_field_listener) {
    std::lock_guard lock(m_mutex);

    if (height_field.is_loaded()) {
        return;
    }

    auto it1 = m_listeners.find(&height_field);
    if (it1 != m_listeners.end()) {
        auto it2 = std::find(it1->second.begin(), it1->second.end(), &height_field_listener);
        if (it2 != it1->second.end()) {
            std::swap(*it2, it1->second.back());
            it1->second.pop_back();
        }
    }
}

void HeightFieldNotifier::notify(HeightField& height_field) {
    std::lock_guard lock(m_mutex);

    auto it = m_listeners.find(&height_field);
    if (it != m_listeners.end()) {
        for (HeightFieldListener* height_field_listener : it->second) {
            height_field_listener->height_field_loaded();
        }
        m_listeners.erase(it);
    }
}

} // namespace kw
