#include "render/particles/particle_system_player.h"
#include "render/particles/emitters/particle_system_emitter.h"
#include "render/particles/generators/particle_system_generator.h"
#include "render/particles/particle_system.h"
#include "render/particles/particle_system_primitive.h"
#include "render/particles/updaters/particle_system_updater.h"

#include <system/timer.h>

#include <core/concurrency/task.h>
#include <core/concurrency/task_scheduler.h>
#include <core/debug/assert.h>

#include <algorithm>

namespace kw {

class ParticleSystemPlayer::WorkerTask : public Task {
public:
    WorkerTask(ParticleSystemPlayer& particle_system_player, size_t primitive_index)
        : m_particle_system_player(particle_system_player)
        , m_primitive_index(primitive_index)
    {
    }

    void run() override {
        std::shared_lock lock(m_particle_system_player.m_primitives_mutex);

        ParticleSystemPrimitive* particle_system_primitive = m_particle_system_player.m_primitives[m_primitive_index];
        if (particle_system_primitive != nullptr) {
            SharedPtr<ParticleSystem> particle_system = particle_system_primitive->get_particle_system();
            if (particle_system && particle_system->is_loaded()) {
                kill(*particle_system_primitive);
                emit(*particle_system_primitive, *particle_system, m_particle_system_player.m_timer.get_elapsed_time());
                update(*particle_system_primitive, *particle_system, m_particle_system_player.m_timer.get_elapsed_time());
            }
        }
    }

    const char* get_name() const override {
        return "Particle System Player Worker";
    }

private:
    void kill(ParticleSystemPrimitive& particle_system_primitive) {
        float* current_lifetime_stream = particle_system_primitive.get_particle_system_stream(ParticleSystemStream::CURRENT_LIFETIME);
        KW_ASSERT(current_lifetime_stream != nullptr);

        float* total_lifetime_stream = particle_system_primitive.get_particle_system_stream(ParticleSystemStream::TOTAL_LIFETIME);
        KW_ASSERT(total_lifetime_stream != nullptr);

        size_t particles_killed = 0;

        for (size_t i = 0; i < particle_system_primitive.m_particle_count; i++) {
            if (current_lifetime_stream[i] >= total_lifetime_stream[i]) {
                particles_killed++;
            } else {
                if (particles_killed > 0) {
                    for (size_t j = 0; j < PARTICLE_SYSTEM_STREAM_COUNT; j++) {
                        float* stream = particle_system_primitive.m_particle_system_streams[j].get();
                        if (stream != nullptr) {
                            stream[i - particles_killed] = stream[i];
                        }
                    }
                }
            }
        }

        particle_system_primitive.m_particle_count -= particles_killed;
    }

    void emit(ParticleSystemPrimitive& particle_system_primitive, ParticleSystem& particle_system, float elapsed_time) {
        particle_system_primitive.m_particle_system_time += elapsed_time;
        if (particle_system_primitive.m_particle_system_time >= particle_system.get_duration()) {
            particle_system_primitive.m_particle_system_time = 0.f;
        }

        size_t begin_index = particle_system_primitive.m_particle_count;
        size_t end_index = begin_index;

        for (const UniquePtr<ParticleSystemEmitter>& emitter : particle_system.get_emitters()) {
            end_index += emitter->emit(particle_system_primitive, elapsed_time);
        }

        end_index = std::min(end_index, particle_system.get_max_particle_count());

        if (begin_index != end_index) {
            particle_system_primitive.m_particle_count = end_index;

            for (const UniquePtr<ParticleSystemGenerator>& generator : particle_system.get_generators()) {
                generator->generate(particle_system_primitive, begin_index, end_index);
            }
        }
    }

    void update(ParticleSystemPrimitive& particle_system_primitive, ParticleSystem& particle_system, float elapsed_time) {
        for (const UniquePtr<ParticleSystemUpdater>& updater : particle_system.get_updaters()) {
            updater->update(particle_system_primitive, elapsed_time);
        }
    }

    ParticleSystemPlayer& m_particle_system_player;
    size_t m_primitive_index;
};

class ParticleSystemPlayer::BeginTask : public Task {
public:
    BeginTask(ParticleSystemPlayer& particle_system_player, Task* end_task)
        : m_particle_system_player(particle_system_player)
        , m_end_task(end_task)
    {
    }

    void run() override {
        std::lock_guard lock(m_particle_system_player.m_primitives_mutex);

        for (size_t i = 0; i < m_particle_system_player.m_primitives.size(); i++) {
            WorkerTask* worker_task = m_particle_system_player.m_transient_memory_resource.construct<WorkerTask>(m_particle_system_player, i);
            KW_ASSERT(worker_task != nullptr);

            worker_task->add_output_dependencies(m_particle_system_player.m_transient_memory_resource, { m_end_task });

            m_particle_system_player.m_task_scheduler.enqueue_task(m_particle_system_player.m_transient_memory_resource, worker_task);
        }
    }

    const char* get_name() const override {
        return "Particle System Player Begin";
    }

private:
    ParticleSystemPlayer& m_particle_system_player;
    Task* m_end_task;
};

ParticleSystemPlayer::ParticleSystemPlayer(const ParticleSystemPlayerDescriptor& descriptor)
    : m_timer(*descriptor.timer)
    , m_task_scheduler(*descriptor.task_scheduler)
    , m_persistent_memory_resource(*descriptor.persistent_memory_resource)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
    , m_primitives(*descriptor.persistent_memory_resource)
{
    KW_ASSERT(descriptor.timer != nullptr);
    KW_ASSERT(descriptor.task_scheduler != nullptr);
    KW_ASSERT(descriptor.persistent_memory_resource != nullptr);
    KW_ASSERT(descriptor.transient_memory_resource != nullptr);

    m_primitives.reserve(32);
}

void ParticleSystemPlayer::add(ParticleSystemPrimitive& primitive) {
    std::lock_guard lock(m_primitives_mutex);

    KW_ASSERT(primitive.m_particle_system_player == nullptr);
    primitive.m_particle_system_player = this;

    for (ParticleSystemPrimitive*& particle_system_primitive : m_primitives) {
        if (particle_system_primitive == nullptr) {
            particle_system_primitive = &primitive;
            return;
        }
    }

    m_primitives.push_back(&primitive);
}

void ParticleSystemPlayer::remove(ParticleSystemPrimitive& primitive) {
    std::lock_guard lock(m_primitives_mutex);

    KW_ASSERT(primitive.m_particle_system_player == this);
    primitive.m_particle_system_player = nullptr;

    auto it = std::find(m_primitives.begin(), m_primitives.end(), &primitive);
    KW_ASSERT(it != m_primitives.end());

    *it = nullptr;
}

Pair<Task*, Task*> ParticleSystemPlayer::create_tasks() {
    Task* end_task = m_transient_memory_resource.construct<NoopTask>("Particle System Player End");
    Task* begin_task = m_transient_memory_resource.construct<BeginTask>(*this, end_task);

    return { begin_task, end_task };
}

} // namespace kw
