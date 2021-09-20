#pragma once

#include <core/containers/shared_ptr.h>
#include <core/containers/string.h>
#include <core/containers/unordered_map.h>
#include <core/containers/vector.h>

namespace kw {

class BlendTree;

class MotionGraph {
public:
    struct Motion {
        SharedPtr<BlendTree> blend_tree;
        Vector<uint32_t> transitions; // Index within `m_transitions`.
        float duration;
    };

    struct Transition {
        uint32_t destination; // Index within `m_motions`.
        float duration;
        String trigger_event; // TODO: Prefer `Name` to `String`.
    };

    MotionGraph();
    MotionGraph(Vector<Motion>&& motions, Vector<Transition>&& transitions, UnorderedMap<String, uint32_t>&& mapping, uint32_t default_motion_index);
    MotionGraph(MotionGraph&& other) noexcept;
    ~MotionGraph();
    MotionGraph& operator=(MotionGraph&& other) noexcept;

    const Vector<Motion>& get_motions() const;
    const Vector<Transition>& get_transitions() const;
    uint32_t get_motion_index(const String& name) const;
    uint32_t get_default_motion_index() const;

    bool is_loaded() const;

private:
    Vector<Motion> m_motions;
    Vector<Transition> m_transitions;
    UnorderedMap<String, uint32_t> m_mapping; // TODO: Prefer `Name` to `String`.
    uint32_t m_default_motion_index;
};

} // namespace kw
