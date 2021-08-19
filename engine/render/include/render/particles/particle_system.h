#pragma once

#include "render/particles/particle_system_stream_mask.h"

#include <core/containers/shared_ptr.h>
#include <core/containers/unique_ptr.h>
#include <core/containers/vector.h>
#include <core/math/aabbox.h>

namespace kw {

class Geometry;
class Material;
class ParticleSystemEmitter;
class ParticleSystemGenerator;
class ParticleSystemListener;
class ParticleSystemNotifier;
class ParticleSystemUpdater;

// Whether particles should face the camera by rotating along given axes.
enum class ParticleSystemAxes {
    NONE,
    Y,
    YZ,
};

struct ParticleSystemDescriptor {
    explicit ParticleSystemDescriptor(MemoryResource& memory_resource);
    ParticleSystemDescriptor(ParticleSystemDescriptor&& other);
    ~ParticleSystemDescriptor();
    ParticleSystemDescriptor& operator=(ParticleSystemDescriptor&& other);

    ParticleSystemNotifier* particle_system_notifier;

    float duration;
    uint32_t loop_count; // 0 is interpreted as infinity.
    size_t max_particle_count;
    aabbox max_bounds;
    SharedPtr<Geometry> geometry;
    SharedPtr<Material> material;
    SharedPtr<Material> shadow_material;
    uint32_t spritesheet_x; // 0 is interpreted as 1.
    uint32_t spritesheet_y; // 0 is interpreted as 1.
    ParticleSystemAxes axes;

    Vector<UniquePtr<ParticleSystemEmitter>> emitters;
    Vector<UniquePtr<ParticleSystemGenerator>> generators;
    Vector<UniquePtr<ParticleSystemUpdater>> updaters;
};

class ParticleSystem {
public:
    explicit ParticleSystem(ParticleSystemNotifier& particle_system_notifier);
    explicit ParticleSystem(ParticleSystemDescriptor&& descriptor);
    ParticleSystem(ParticleSystem&& other);
    ~ParticleSystem();
    ParticleSystem& operator=(ParticleSystem&& other);

    // This particle system listener will be notified when this particle system is loaded.
    // If this particle system is already loaded, the listener will be notified immediately.
    void subscribe(ParticleSystemListener& particle_system_listener);
    void unsubscribe(ParticleSystemListener& particle_system_listener);

    const Vector<UniquePtr<ParticleSystemEmitter>>& get_emitters() const;
    const Vector<UniquePtr<ParticleSystemGenerator>>& get_generators() const;
    const Vector<UniquePtr<ParticleSystemUpdater>>& get_updaters() const;

    // Calculated automatically from generators and updaters.
    ParticleSystemStreamMask get_stream_mask() const;

    // Calculated automatically from emitters.
    size_t get_max_particle_count() const;

    // Calculated automatically from generators and updaters.
    const aabbox& get_max_bounds() const;

    float get_duration() const;
    const SharedPtr<Geometry>& get_geometry() const;
    const SharedPtr<Material>& get_material() const;
    const SharedPtr<Material>& get_shadow_material() const;
    uint32_t get_spritesheet_x() const;
    uint32_t get_spritesheet_y() const;
    ParticleSystemAxes get_axes() const;

    // Whether particle system is loaded (doesn't mean material or geometry is loaded too).
    bool is_loaded() const;

private:
    ParticleSystemNotifier& m_particle_system_notifier;

    float m_duration;
    uint32_t m_loop_count;
    size_t m_max_particle_count;
    aabbox m_max_bounds;
    SharedPtr<Geometry> m_geometry;
    SharedPtr<Material> m_material;
    SharedPtr<Material> m_shadow_material;
    uint32_t m_spritesheet_x;
    uint32_t m_spritesheet_y;
    ParticleSystemAxes m_axes;
    ParticleSystemStreamMask m_stream_mask;

    Vector<UniquePtr<ParticleSystemEmitter>> m_emitters;
    Vector<UniquePtr<ParticleSystemGenerator>> m_generators;
    Vector<UniquePtr<ParticleSystemUpdater>> m_updaters;
};

} // namespace kw
