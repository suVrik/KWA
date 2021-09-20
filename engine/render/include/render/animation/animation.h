#pragma once

#include <core/containers/vector.h>
#include <core/math/transform.h>

namespace kw {

class Animation {
public:
    struct JointKeyframe {
        float timestamp;
        transform transform;
    };

    struct JointAnimation {
        Vector<JointKeyframe> keyframes;
    };

    Animation();
    explicit Animation(Vector<JointAnimation>&& joint_animations);

    // Return joint transform at given timestamp.
    transform get_joint_transform(uint32_t joint_index, float timestamp) const;

    // Return the number of joints of target skeleton.
    size_t get_joint_count() const;

    bool is_loaded() const;

private:
    float m_duration;
    Vector<JointAnimation> m_joint_animations;
};

} // namespace kw
