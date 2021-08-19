#include "render/particles/particle_system.h"
#include "render/particles/emitters/particle_system_emitter.h"
#include "render/particles/generators/particle_system_generator.h"
#include "render/particles/particle_system_listener.h"
#include "render/particles/particle_system_notifier.h"
#include "render/particles/updaters/particle_system_updater.h"

#include <core/debug/assert.h>
#include <core/memory/malloc_memory_resource.h>

namespace kw {

ParticleSystemDescriptor::ParticleSystemDescriptor(MemoryResource& memory_resource)
    : duration(0.f)
    , loop_count(UINT32_MAX)
    , max_particle_count(0)
    , spritesheet_x(0)
    , spritesheet_y(0)
    , emitters(memory_resource)
    , generators(memory_resource)
    , updaters(memory_resource)
{
}

ParticleSystemDescriptor::ParticleSystemDescriptor(ParticleSystemDescriptor&& other) = default;
ParticleSystemDescriptor::~ParticleSystemDescriptor() = default;
ParticleSystemDescriptor& ParticleSystemDescriptor::operator=(ParticleSystemDescriptor&& other) = default;

ParticleSystem::ParticleSystem(ParticleSystemNotifier& particle_system_notifier)
    : m_particle_system_notifier(particle_system_notifier)
    , m_duration(0.f)
    , m_loop_count(UINT32_MAX)
    , m_max_particle_count(0)
    , m_spritesheet_x(1)
    , m_spritesheet_y(1)
    , m_axes(ParticleSystemAxes::NONE)
    , m_stream_mask(ParticleSystemStreamMask::NONE)
    , m_emitters(MallocMemoryResource::instance())
    , m_generators(MallocMemoryResource::instance())
    , m_updaters(MallocMemoryResource::instance())
{
}

ParticleSystem::ParticleSystem(ParticleSystemDescriptor&& descriptor)
    : m_particle_system_notifier(*descriptor.particle_system_notifier)
    , m_duration(descriptor.duration)
    , m_loop_count(descriptor.loop_count == 0 ? UINT32_MAX : descriptor.loop_count)
    , m_max_particle_count(align_up(descriptor.max_particle_count, size_t(4)))
    , m_max_bounds(descriptor.max_bounds)
    , m_geometry(std::move(descriptor.geometry))
    , m_material(std::move(descriptor.material))
    , m_shadow_material(std::move(descriptor.shadow_material))
    , m_spritesheet_x(std::max(descriptor.spritesheet_x, 1U))
    , m_spritesheet_y(std::max(descriptor.spritesheet_y, 1U))
    , m_axes(descriptor.axes)
    , m_stream_mask(ParticleSystemStreamMask::NONE)
    , m_emitters(std::move(descriptor.emitters))
    , m_generators(std::move(descriptor.generators))
    , m_updaters(std::move(descriptor.updaters))
{
    KW_ASSERT(descriptor.particle_system_notifier != nullptr);

    for (UniquePtr<ParticleSystemGenerator>& generator : m_generators) {
        m_stream_mask |= generator->get_stream_mask();
    }

    for (UniquePtr<ParticleSystemUpdater>& updater : m_updaters) {
        m_stream_mask |= updater->get_stream_mask();
    }
}

ParticleSystem::ParticleSystem(ParticleSystem&& other)
    : m_particle_system_notifier(other.m_particle_system_notifier)
    , m_duration(other.m_duration)
    , m_loop_count(other.m_loop_count)
    , m_max_particle_count(other.m_max_particle_count)
    , m_max_bounds(other.m_max_bounds)
    , m_geometry(std::move(other.m_geometry))
    , m_material(std::move(other.m_material))
    , m_shadow_material(std::move(other.m_shadow_material))
    , m_spritesheet_x(other.m_spritesheet_x)
    , m_spritesheet_y(other.m_spritesheet_y)
    , m_axes(other.m_axes)
    , m_stream_mask(other.m_stream_mask)
    , m_emitters(std::move(other.m_emitters))
    , m_generators(std::move(other.m_generators))
    , m_updaters(std::move(other.m_updaters))
{
}

ParticleSystem::~ParticleSystem() = default;

ParticleSystem& ParticleSystem::operator=(ParticleSystem&& other) {
    KW_ASSERT(!is_loaded(), "Move assignemt is allowed only for unloaded particle system.");
    KW_ASSERT(&m_particle_system_notifier == &other.m_particle_system_notifier, "Invalid particle system move assignment.");

    m_duration = other.m_duration;
    m_loop_count = other.m_loop_count;
    m_max_particle_count = other.m_max_particle_count;
    m_max_bounds = other.m_max_bounds;
    m_geometry = std::move(other.m_geometry);
    m_material = std::move(other.m_material);
    m_shadow_material = std::move(other.m_shadow_material);
    m_spritesheet_x = other.m_spritesheet_x;
    m_spritesheet_y = other.m_spritesheet_y;
    m_axes = other.m_axes;
    m_stream_mask = other.m_stream_mask;
    m_emitters = std::move(other.m_emitters);
    m_generators = std::move(other.m_generators);
    m_updaters = std::move(other.m_updaters);

    other.m_duration = 0.f;
    other.m_loop_count = UINT32_MAX;
    other.m_spritesheet_x = 1;
    other.m_spritesheet_y = 1;
    other.m_axes = ParticleSystemAxes::NONE;
    other.m_stream_mask = ParticleSystemStreamMask::NONE;
    other.m_max_particle_count = 0;
    other.m_max_bounds = {};

    return *this;
}

void ParticleSystem::subscribe(ParticleSystemListener& particle_system_listener) {
    if (is_loaded()) {
        particle_system_listener.particle_system_loaded();
    } else {
        m_particle_system_notifier.subscribe(*this, particle_system_listener);
    }
}

void ParticleSystem::unsubscribe(ParticleSystemListener& particle_system_listener) {
    if (!is_loaded()) {
        m_particle_system_notifier.unsubscribe(*this, particle_system_listener);
    }
}

const Vector<UniquePtr<ParticleSystemEmitter>>& ParticleSystem::get_emitters() const {
    return m_emitters;
}

const Vector<UniquePtr<ParticleSystemGenerator>>& ParticleSystem::get_generators() const {
    return m_generators;
}

const Vector<UniquePtr<ParticleSystemUpdater>>& ParticleSystem::get_updaters() const {
    return m_updaters;
}

ParticleSystemStreamMask ParticleSystem::get_stream_mask() const {
    return m_stream_mask;
}

size_t ParticleSystem::get_max_particle_count() const {
    return m_max_particle_count;
}

const aabbox& ParticleSystem::get_max_bounds() const {
    return m_max_bounds;
}

float ParticleSystem::get_duration() const {
    return m_duration;
}

const SharedPtr<Geometry>& ParticleSystem::get_geometry() const {
    return m_geometry;
}

const SharedPtr<Material>& ParticleSystem::get_material() const {
    return m_material;
}

const SharedPtr<Material>& ParticleSystem::get_shadow_material() const {
    return m_shadow_material;
}

uint32_t ParticleSystem::get_spritesheet_x() const {
    return m_spritesheet_x;
}

uint32_t ParticleSystem::get_spritesheet_y() const {
    return m_spritesheet_y;
}

ParticleSystemAxes ParticleSystem::get_axes() const {
    return m_axes;
}

bool ParticleSystem::is_loaded() const {
    return m_geometry != nullptr;
}

} // namespace kw
