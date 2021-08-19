#pragma once

#include <core/containers/unordered_map.h>
#include <core/containers/vector.h>

#include <mutex>

namespace kw {

class ParticleSystem;
class ParticleSystemListener;

class ParticleSystemNotifier {
public:
    explicit ParticleSystemNotifier(MemoryResource& memory_resource);

    void subscribe(ParticleSystem& particle_system, ParticleSystemListener& particle_system_listener);
    void unsubscribe(ParticleSystem& particle_system, ParticleSystemListener& particle_system_listener);

    void notify(ParticleSystem& particle_system);

private:
    MemoryResource& m_memory_resource;
    UnorderedMap<ParticleSystem*, Vector<ParticleSystemListener*>> m_listeners;
    std::mutex m_mutex;
};

} // namespace kw
