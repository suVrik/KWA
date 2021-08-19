#include "render/particles/particle_system_notifier.h"
#include "render/particles/particle_system_listener.h"

namespace kw {

ParticleSystemNotifier::ParticleSystemNotifier(MemoryResource& memory_resource)
    : m_memory_resource(memory_resource)
    , m_listeners(memory_resource)
{
}

void ParticleSystemNotifier::subscribe(ParticleSystem& particle_system, ParticleSystemListener& particle_system_listener) {
    std::lock_guard lock(m_mutex);

    auto it = m_listeners.find(&particle_system);
    if (it != m_listeners.end()) {
        it->second.push_back(&particle_system_listener);
    } else {
        m_listeners.emplace(&particle_system, Vector<ParticleSystemListener*>(1, &particle_system_listener, m_memory_resource));
    }
}

void ParticleSystemNotifier::unsubscribe(ParticleSystem& particle_system, ParticleSystemListener& particle_system_listener) {
    std::lock_guard lock(m_mutex);

    auto it1 = m_listeners.find(&particle_system);
    if (it1 != m_listeners.end()) {
        auto it2 = std::find(it1->second.begin(), it1->second.end(), &particle_system_listener);
        if (it2 != it1->second.end()) {
            std::swap(*it2, it1->second.back());
            it1->second.pop_back();
        }
    }
}

void ParticleSystemNotifier::notify(ParticleSystem& particle_system) {
    std::lock_guard lock(m_mutex);

    auto it = m_listeners.find(&particle_system);
    if (it != m_listeners.end()) {
        for (ParticleSystemListener* particle_system_listener : it->second) {
            particle_system_listener->particle_system_loaded();
        }
        m_listeners.erase(it);
    }
}

} // namespace kw
