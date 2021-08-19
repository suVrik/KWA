#include "render/particles/particle_system_primitive.h"
#include "render/particles/particle_system.h"
#include "render/particles/particle_system_player.h"
#include "render/particles/particle_system_stream_mask.h"

#include <core/debug/assert.h>

namespace kw {

ParticleSystemPrimitive::ParticleSystemPrimitive(MemoryResource& memory_resource,
                                                 SharedPtr<ParticleSystem> particle_system,
                                                 const transform& local_transform)
    : AccelerationStructurePrimitive(local_transform)
    , m_particle_system_player(nullptr)
    , m_particle_system(std::move(particle_system))
    , m_particle_system_time(0.f)
    , m_memory_resource(memory_resource)
    , m_particle_count(0)
{
    if (m_particle_system) {
        // If particle system is already loaded, `particle_system_loaded` will be called immediately.
        m_particle_system->subscribe(*this);
    }
}

// Copy constructor and copy assignment copy only particle system, but not the particles themselves.
ParticleSystemPrimitive::ParticleSystemPrimitive(const ParticleSystemPrimitive& other)
    : AccelerationStructurePrimitive(other)
    , m_particle_system_player(nullptr)
    , m_particle_system(other.m_particle_system)
    , m_particle_system_time(0.f)
    , m_memory_resource(other.m_memory_resource)
    , m_particle_count(0)
{
    KW_ASSERT(
        other.m_particle_system_player == nullptr,
        "Copying particle system primitives with a particle system player assigned is not allowed."
    );

    if (m_particle_system) {
        // If particle system is already loaded, `particle_system_loaded` will be called immediately.
        m_particle_system->subscribe(*this);
    }
}

ParticleSystemPrimitive::~ParticleSystemPrimitive() {
    if (m_particle_system) {
        // No effect if `particle_system_loaded` for this primitive & particle system was already called.
        m_particle_system->unsubscribe(*this);
    }

    if (m_particle_system_player != nullptr) {
        m_particle_system_player->remove(*this);
    }
}

// Copy constructor and copy assignment copy only particle system, but not the particles themselves.
ParticleSystemPrimitive& ParticleSystemPrimitive::operator=(const ParticleSystemPrimitive& other) {
    AccelerationStructurePrimitive::operator=(other);

    if (m_particle_system) {
        // No effect if `particle_system_loaded` for this primitive & particle system was already called.
        m_particle_system->unsubscribe(*this);
    }

    if (m_particle_system_player != nullptr) {
        m_particle_system_player->remove(*this);
    }

    m_particle_system_player = nullptr;
    m_particle_system = other.m_particle_system;
    std::fill(std::begin(m_particle_system_streams), std::end(m_particle_system_streams), nullptr);
    m_particle_count = 0;
    m_particle_system_time = 0.f;

    KW_ASSERT(
        other.m_particle_system_player == nullptr,
        "Copying particle system primitives with a particle system player assigned is not allowed."
    );

    if (m_particle_system) {
        // If particle system is already loaded, `particle_system_loaded` will be called immediately.
        m_particle_system->subscribe(*this);
    }

    return *this;
}

ParticleSystemPlayer* ParticleSystemPrimitive::get_particle_system_player() const {
    return m_particle_system_player;
}

const SharedPtr<ParticleSystem>& ParticleSystemPrimitive::get_particle_system() const {
    return m_particle_system;
}

void ParticleSystemPrimitive::set_particle_system(SharedPtr<ParticleSystem> particle_system) {
    if (particle_system != m_particle_system) {
        if (m_particle_system) {
            // No effect if `particle_system_loaded` for this primitive & particle system was already called.
            m_particle_system->unsubscribe(*this);
        }

        m_particle_system = std::move(particle_system);

        if (m_particle_system) {
            // If particle system is already loaded, `particle_system_loaded` will be called immediately.
            m_particle_system->subscribe(*this);
        }
    }
}

float* ParticleSystemPrimitive::get_particle_system_stream(ParticleSystemStream stream) const {
    size_t stream_index = static_cast<size_t>(stream);
    KW_ASSERT(stream_index < std::size(m_particle_system_streams));

    return m_particle_system_streams[stream_index].get();
}

size_t ParticleSystemPrimitive::get_particle_count() const {
    return m_particle_count;
}

float ParticleSystemPrimitive::get_particle_system_time() const {
    return m_particle_system_time;
}

void ParticleSystemPrimitive::set_particle_system_time(float value) {
    m_particle_system_time = value;
}

void ParticleSystemPrimitive::global_transform_updated() {
    if (m_particle_system && m_particle_system->is_loaded()) {
        m_bounds = m_particle_system->get_max_bounds() * get_global_transform();
    }

    AccelerationStructurePrimitive::global_transform_updated();
}

void ParticleSystemPrimitive::particle_system_loaded() {
    KW_ASSERT(m_particle_system && m_particle_system->is_loaded(), "Particle system must be loaded.");

    ParticleSystemStreamMask stream_mask = m_particle_system->get_stream_mask();
    size_t max_particle_count = m_particle_system->get_max_particle_count();

    for (size_t i = 0; i < PARTICLE_SYSTEM_STREAM_COUNT; i++) {
        if ((stream_mask & static_cast<ParticleSystemStreamMask>(1 << i)) != ParticleSystemStreamMask::NONE) {
            // Manually allocate rather than `allocate_unique` to specify custom alignment.
            float* memory = static_cast<float*>(m_memory_resource.allocate(sizeof(float) * max_particle_count, 16));
            m_particle_system_streams[i] = UniquePtr<float[]>(memory, m_memory_resource);
        } else {
            m_particle_system_streams[i] = nullptr;
        }
    }

    m_bounds = m_particle_system->get_max_bounds() * get_global_transform();

    // Update acceleration structure's node.
    AccelerationStructurePrimitive::global_transform_updated();
}

} // namespace kw
