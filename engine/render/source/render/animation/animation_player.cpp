#include "render/animation/animation_player.h"
#include "render/animation/animated_geometry_primitive.h"
#include "render/animation/animation.h"
#include "render/geometry/geometry.h"

#include <system/timer.h>

#include <core/concurrency/task.h>
#include <core/concurrency/task_scheduler.h>
#include <core/debug/assert.h>

#include <algorithm>

namespace kw {

class AnimationPlayer::WorkerTask : public Task {
public:
    WorkerTask(AnimationPlayer& animation_player, size_t primitive_index, float elapsed_time)
        : m_animation_player(animation_player)
        , m_primitive_index(primitive_index)
        , m_elapsed_time(elapsed_time)
    {
    }

    void run() override {
        std::shared_lock lock(m_animation_player.m_primitives_mutex);

        AnimatedGeometryPrimitive* animated_geometry_primitive = m_animation_player.m_primitives[m_primitive_index];
        if (animated_geometry_primitive != nullptr) {
            const SharedPtr<Geometry>& geometry = animated_geometry_primitive->get_geometry();
            const SharedPtr<Animation>& animation = animated_geometry_primitive->get_animation();

            if (geometry && geometry->is_loaded() && animation && animation->is_loaded()) {
                SkeletonPose& skeleton_pose = animated_geometry_primitive->get_skeleton_pose();

                KW_ASSERT(
                    skeleton_pose.get_joint_count() == animation->get_joint_count(),
                    "Mismatching animation and skeleton."
                );

                float old_time = animated_geometry_primitive->get_animation_time();
                float new_time = old_time + m_elapsed_time * animated_geometry_primitive->get_animation_speed();
                animated_geometry_primitive->set_animation_time(new_time);

                for (uint32_t i = 0; i < animation->get_joint_count(); i++) {
                    skeleton_pose.set_joint_space_matrix(i, animation->get_joint_transform(i, new_time));
                }

                skeleton_pose.build_model_space_matrices(*geometry->get_skeleton());
            }
        }
    }

    const char* get_name() const override {
        return "Animation Player Worker";
    }

private:
    AnimationPlayer& m_animation_player;
    size_t m_primitive_index;
    float m_elapsed_time;
};

class AnimationPlayer::BeginTask : public Task {
public:
    BeginTask(AnimationPlayer& animation_player, Task* end_task, float elapsed_time)
        : m_animation_player(animation_player)
        , m_end_task(end_task)
        , m_elapsed_time(elapsed_time)
    {
    }

    void run() override {
        std::lock_guard lock(m_animation_player.m_primitives_mutex);

        for (size_t i = 0; i < m_animation_player.m_primitives.size(); i++) {
            WorkerTask* worker_task = m_animation_player.m_transient_memory_resource.construct<WorkerTask>(m_animation_player, i, m_elapsed_time);
            KW_ASSERT(worker_task != nullptr);

            worker_task->add_output_dependencies(m_animation_player.m_transient_memory_resource, { m_end_task });

            m_animation_player.m_task_scheduler.enqueue_task(m_animation_player.m_transient_memory_resource, worker_task);
        }
    }

    const char* get_name() const override {
        return "Animation Player Begin";
    }

private:
    AnimationPlayer& m_animation_player;
    Task* m_end_task;
    float m_elapsed_time;
};

AnimationPlayer::AnimationPlayer(const AnimationPlayerDescriptor& descriptor)
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

void AnimationPlayer::add(AnimatedGeometryPrimitive& primitive) {
    std::lock_guard lock(m_primitives_mutex);

    for (AnimatedGeometryPrimitive*& animated_geometry_primitive : m_primitives) {
        if (animated_geometry_primitive == nullptr) {
            animated_geometry_primitive = &primitive;
            return;
        }
    }

    m_primitives.push_back(&primitive);
}

void AnimationPlayer::remove(AnimatedGeometryPrimitive& primitive) {
    std::lock_guard lock(m_primitives_mutex);

    auto it = std::find(m_primitives.begin(), m_primitives.end(), &primitive);
    KW_ASSERT(it != m_primitives.end());

    *it = nullptr;
}

Pair<Task*, Task*> AnimationPlayer::create_tasks() {
    Task* end_task = m_transient_memory_resource.construct<NoopTask>("Animation Player End");
    Task* begin_task = m_transient_memory_resource.construct<BeginTask>(*this, end_task, m_timer.get_elapsed_time());

    return { begin_task, end_task };
}

} // namespace kw
