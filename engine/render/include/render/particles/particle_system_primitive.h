#pragma once

#include "render/acceleration_structure/acceleration_structure_primitive.h"
#include "render/particles/particle_system_listener.h"
#include "render/particles/particle_system_stream.h"

#include <core/containers/shared_ptr.h>
#include <core/containers/unique_ptr.h>

namespace kw {

class ObjectNode;
class ParticleSystem;
class ParticleSystemPlayer;
class PrimitiveReflection;

class ParticleSystemPrimitive : public AccelerationStructurePrimitive, public ParticleSystemListener {
public:
    static UniquePtr<Primitive> create_from_markdown(PrimitiveReflection& reflection, const ObjectNode& node);

    ParticleSystemPrimitive(MemoryResource& memory_resource,
                            SharedPtr<ParticleSystem> particle_system = nullptr,
                            const transform& local_transform = transform());
    ParticleSystemPrimitive(const ParticleSystemPrimitive& other);
    ~ParticleSystemPrimitive() override;
    ParticleSystemPrimitive& operator=(const ParticleSystemPrimitive& other);

    // Particle system player is set from ParticleSystemPlayer's `add` method.
    ParticleSystemPlayer* get_particle_system_player() const;

    const SharedPtr<ParticleSystem>& get_particle_system() const;
    void set_particle_system(SharedPtr<ParticleSystem> particle_system);

    // The number of floats in particle system stream is guaranteed to be a multiple of 4.
    // The stream itself is guaranteed to be aligned by the size of 4 floats.
    // Returns nullptr if the stream is not used by this particle system.
    float* get_particle_system_stream(ParticleSystemStream stream) const;
    size_t get_particle_count() const;

    float get_particle_system_time() const;
    void set_particle_system_time(float value);

    UniquePtr<Primitive> clone(MemoryResource& memory_resource) const override;

protected:
    void global_transform_updated() override;
    void particle_system_loaded() override;

private:
    ParticleSystemPlayer* m_particle_system_player;
    SharedPtr<ParticleSystem> m_particle_system;
    float m_particle_system_time;

    MemoryResource& m_memory_resource;
    UniquePtr<float[]> m_particle_system_streams[PARTICLE_SYSTEM_STREAM_COUNT];
    size_t m_particle_count;

    // Friendship is needed to access `m_particle_system_player`.
    friend class ParticleSystemPlayer;
};

} // namespace kw
