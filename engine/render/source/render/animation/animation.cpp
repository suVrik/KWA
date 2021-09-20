#include "render/animation/animation.h"

#include <core/debug/assert.h>
#include <core/math/float4x4.h>
#include <core/math/transform.h>
#include <core/memory/malloc_memory_resource.h>

#include <algorithm>

namespace kw {

static bool operator<(const Animation::JointKeyframe& lhs, const Animation::JointKeyframe& rhs) {
    return lhs.timestamp < rhs.timestamp;
}

static bool operator<(const Animation::JointKeyframe& lhs, float rhs) {
    return lhs.timestamp < rhs;
}

Animation::Animation()
    : m_duration(NAN)
    , m_joint_animations(MallocMemoryResource::instance())
{
}

Animation::Animation(Vector<JointAnimation>&& joint_animations)
    : m_duration(NAN)
    , m_joint_animations(std::move(joint_animations))
{
    float duration = 0.f;
    for (JointAnimation& joint_animation : m_joint_animations) {
        KW_ASSERT(!joint_animation.keyframes.empty(), "Empty joint animations are not allowed.");
        duration = std::max(duration, joint_animation.keyframes.back().timestamp);
    }
    m_duration = duration;
}

transform Animation::get_joint_transform(uint32_t joint_index, float timestamp) const {
    KW_ASSERT(is_loaded(), "Animation is not loaded yet.");

    KW_ASSERT(joint_index < m_joint_animations.size(), "Invalid joint index.");
    const JointAnimation& joint_animation = m_joint_animations[joint_index];

    float normalized_timestamp = m_duration > 0.f ? std::fmodf(timestamp, m_duration) : 0.f;
    KW_ASSERT(normalized_timestamp < m_duration);

    auto it = std::lower_bound(joint_animation.keyframes.begin(), joint_animation.keyframes.end(), normalized_timestamp);
    if (it != joint_animation.keyframes.end()) {
        if (it != joint_animation.keyframes.begin()) {
            auto prev_it = std::prev(it);

            float factor = (normalized_timestamp - prev_it->timestamp) / (it->timestamp - prev_it->timestamp);

            return lerp(prev_it->transform, it->transform, factor);
        } else {
            auto prev_it = joint_animation.keyframes.rbegin();

            float factor = it->timestamp > EPSILON ? normalized_timestamp / it->timestamp : 1.f;

            return lerp(prev_it->transform, it->transform, factor);
        }
    } else {
        const JointKeyframe& joint_keyframe = joint_animation.keyframes.back();
        return transform(joint_keyframe.transform.translation, joint_keyframe.transform.rotation, joint_keyframe.transform.scale);
    }
}

size_t Animation::get_joint_count() const {
    return m_joint_animations.size();
}

bool Animation::is_loaded() const {
    return m_duration == m_duration;
}

} // namespace kw
