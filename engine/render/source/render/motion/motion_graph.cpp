#include "render/motion/motion_graph.h"

#include <core/memory/malloc_memory_resource.h>

namespace kw {

MotionGraph::MotionGraph()
    : m_motions(MallocMemoryResource::instance())
    , m_transitions(MallocMemoryResource::instance())
    , m_mapping(MallocMemoryResource::instance())
    , m_default_motion_index(UINT32_MAX)
{
}

MotionGraph::MotionGraph(Vector<Motion>&& motions, Vector<Transition>&& transitions, UnorderedMap<String, uint32_t>&& mapping, uint32_t default_motion_index)
    : m_motions(std::move(motions))
    , m_transitions(std::move(transitions))
    , m_mapping(std::move(mapping))
    , m_default_motion_index(default_motion_index)
{
}

MotionGraph::MotionGraph(MotionGraph&& other) noexcept = default;
MotionGraph::~MotionGraph() = default;
MotionGraph& MotionGraph::operator=(MotionGraph&& other) noexcept = default;

const Vector<MotionGraph::Motion>& MotionGraph::get_motions() const {
    return m_motions;
}

const Vector<MotionGraph::Transition>& MotionGraph::get_transitions() const {
    return m_transitions;
}

uint32_t MotionGraph::get_motion_index(const String& name) const {
    auto it = m_mapping.find(name);
    if (it != m_mapping.end()) {
        return it->second;
    }
    return UINT32_MAX;
}

uint32_t MotionGraph::get_default_motion_index() const {
    return m_default_motion_index;
}

bool MotionGraph::is_loaded() const {
    return m_default_motion_index != UINT32_MAX;
}

} // namespace kw
