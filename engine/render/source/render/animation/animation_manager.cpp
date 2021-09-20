#include "render/animation/animation_manager.h"
#include "render/animation/animation.h"

#include <core/concurrency/task.h>
#include <core/concurrency/task_scheduler.h>
#include <core/debug/assert.h>
#include <core/error.h>
#include <core/io/binary_reader.h>
#include <core/memory/malloc_memory_resource.h>

namespace kw {

namespace EndianUtils {

static float3 swap_le(float3 vector) {
    vector.x = swap_le(vector.x);
    vector.y = swap_le(vector.y);
    vector.z = swap_le(vector.z);
    return vector;
}

static quaternion swap_le(quaternion quaternion) {
    quaternion.x = swap_le(quaternion.x);
    quaternion.y = swap_le(quaternion.y);
    quaternion.z = swap_le(quaternion.z);
    quaternion.w = swap_le(quaternion.w);
    return quaternion;
}

static transform swap_le(transform transform) {
    transform.translation = swap_le(transform.translation);
    transform.rotation = swap_le(transform.rotation);
    transform.scale = swap_le(transform.scale);
    return transform;
}

static Animation::JointKeyframe swap_le(Animation::JointKeyframe keyframe) {
    keyframe.timestamp = swap_le(keyframe.timestamp);
    keyframe.transform = swap_le(keyframe.transform);
    return keyframe;
}

} // namespace EndianUtils

constexpr uint32_t KWA_SIGNATURE = ' AWK';

class AnimationManager::PendingTask final : public Task {
public:
    PendingTask(AnimationManager& manager, Animation& animation, const char* relative_path)
        : m_manager(manager)
        , m_animation(animation)
        , m_relative_path(relative_path)
    {
    }

    void run() override {
        BinaryReader reader(m_relative_path);
        KW_ERROR(reader, "Failed to open animation \"%s\".", m_relative_path);
        KW_ERROR(read_next(reader) == KWA_SIGNATURE, "Invalid animation \"%s\" signature.", m_relative_path);

        uint32_t joint_count = read_next(reader);

        Vector<Animation::JointAnimation> joint_animations(m_manager.m_persistent_memory_resource);
        joint_animations.reserve(joint_count);

        for (uint32_t i = 0; i < joint_count; i++) {
            std::optional<uint32_t> frame_count = reader.read_le<uint32_t>();
            KW_ERROR(frame_count, "Failed to read joint animation frame count.");

            Animation::JointAnimation joint_animation{ Vector<Animation::JointKeyframe>(m_manager.m_persistent_memory_resource) };
            joint_animation.keyframes.reserve(*frame_count);

            for (uint32_t j = 0; j < *frame_count; j++) {
                std::optional<Animation::JointKeyframe> keyframe = reader.read_le<Animation::JointKeyframe>();
                KW_ERROR(keyframe, "Failed to read joint animation keyframe.");

                joint_animation.keyframes.push_back(*keyframe);
            }

            joint_animations.push_back(std::move(joint_animation));
        }

        m_animation = Animation(std::move(joint_animations));
    }

    const char* get_name() const override {
        return "Animation Manager Pending";
    }

private:
    uint32_t read_next(BinaryReader& reader) {
        std::optional<uint32_t> value = reader.read_le<uint32_t>();
        KW_ERROR(value, "Failed to read animation header.");
        return *value;
    }

    AnimationManager& m_manager;
    Animation& m_animation;
    const char* m_relative_path;
};

class AnimationManager::BeginTask final : public Task {
public:
    BeginTask(AnimationManager& manager, Task* end_task)
        : m_manager(manager)
        , m_end_task(end_task)
    {
    }

    void run() override {
        // Tasks that load animations are expected to run before begin task, so this shouldn't block anyone.
        std::lock_guard lock_guard(m_manager.m_animations_mutex);

        //
        // Start loading brand new animations.
        //

        for (auto& [relative_path, animation] : m_manager.m_pending_animations) {
            PendingTask* pending_task = m_manager.m_transient_memory_resource.construct<PendingTask>(m_manager, *animation, relative_path.c_str());
            KW_ASSERT(pending_task != nullptr);

            pending_task->add_output_dependencies(m_manager.m_transient_memory_resource, { m_end_task });

            m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, pending_task);
        }

        m_manager.m_pending_animations.clear();

        //
        // Destroy animations that only referenced from `AnimationManager`.
        //

        for (auto it = m_manager.m_animations.begin(); it != m_manager.m_animations.end(); ) {
            if (it->second.use_count() == 1) {
                it = m_manager.m_animations.erase(it);
            } else {
                ++it;
            }
        }
    }

    const char* get_name() const override {
        return "Animation Manager Begin";
    }

private:
    AnimationManager& m_manager;
    Task* m_end_task;
};

AnimationManager::AnimationManager(const AnimationManagerDescriptor& descriptor)
    : m_task_scheduler(*descriptor.task_scheduler)
    , m_persistent_memory_resource(*descriptor.persistent_memory_resource)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
    , m_animations(*descriptor.persistent_memory_resource)
    , m_pending_animations(*descriptor.persistent_memory_resource)
{
    KW_ASSERT(descriptor.task_scheduler != nullptr);
    KW_ASSERT(descriptor.persistent_memory_resource != nullptr);
    KW_ASSERT(descriptor.transient_memory_resource != nullptr);

    m_animations.reserve(32);
    m_pending_animations.reserve(32);
}

AnimationManager::~AnimationManager() {
    m_pending_animations.clear();

    for (auto& [relative_path, animation] : m_animations) {
        KW_ASSERT(animation.use_count() == 1, "Not all animation are released.");
    }
}

SharedPtr<Animation> AnimationManager::load(const char* relative_path) {
    if (relative_path[0] == '\0') {
        // Empty string is allowed.
        return nullptr;
    }

    {
        std::shared_lock shared_lock(m_animations_mutex);

        auto it = m_animations.find(String(relative_path, m_transient_memory_resource));
        if (it != m_animations.end()) {
            return it->second;
        }
    }

    {
        std::lock_guard lock_guard(m_animations_mutex);

        auto [it, success] = m_animations.emplace(String(relative_path, m_persistent_memory_resource), SharedPtr<Animation>());
        if (!success) {
            // Could return here if animation is enqueued from multiple threads.
            return it->second;
        }

        it->second = allocate_shared<Animation>(m_persistent_memory_resource);

        m_pending_animations.emplace_back(it->first, it->second);

        return it->second;
    }
}

const String& AnimationManager::get_relative_path(const SharedPtr<Animation>& animation) const {
    for (auto& [relative_path, stored_animation] : m_animations) {
        if (animation == stored_animation) {
            return relative_path;
        }
    }

    static String EMPTY_STRING(MallocMemoryResource::instance());
    return EMPTY_STRING;
}

Pair<Task*, Task*> AnimationManager::create_tasks() {
    Task* end_task = m_transient_memory_resource.construct<NoopTask>("Animation Manager End");
    Task* begin_task = m_transient_memory_resource.construct<BeginTask>(*this, end_task);

    return { begin_task, end_task };
}

} // namespace kw
