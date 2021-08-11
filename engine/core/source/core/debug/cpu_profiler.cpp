#include "core/debug/cpu_profiler.h"
#include "core/concurrency/concurrency_utils.h"
#include "core/memory/malloc_memory_resource.h"

#include <algorithm>
#include <chrono>

namespace kw {

#ifndef KW_CPU_PROFILER_DISABLE
static constexpr size_t SCOPE_COUNT = 1048576;
static constexpr size_t FRAME_COUNT = 64;
#else
static constexpr size_t SCOPE_COUNT = 0;
static constexpr size_t FRAME_COUNT = 0;
#endif

static constexpr size_t SCOPE_MASK = SCOPE_COUNT - 1;
static constexpr size_t FRAME_MASK = FRAME_COUNT - 1;

inline uint64_t get_current_timestamp() {
    return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

CpuProfiler::Counter::Counter(const char* name)
    : m_scope_name(name)
    , m_begin_timestamp(get_current_timestamp())
{
}

CpuProfiler::Counter::~Counter() {
    CpuProfiler& cpu_profiler = CpuProfiler::instance();
    if (!cpu_profiler.m_is_paused) {
        Scope& scope = cpu_profiler.m_scopes[++cpu_profiler.m_current_scope & SCOPE_MASK];
        scope.scope_name = m_scope_name;
        scope.thread_name = ConcurrencyUtils::get_current_thread_name();
        scope.begin_timestamp = m_begin_timestamp;
        scope.end_timestamp = get_current_timestamp();
    }
}

CpuProfiler::CpuProfiler()
    : m_scopes(SCOPE_COUNT, MallocMemoryResource::instance())
    , m_current_scope(0)
    , m_frames(FRAME_COUNT, MallocMemoryResource::instance())
    , m_current_frame(0)
    , m_is_paused(false)
    , m_is_pause_scheduled(false)
{
}

CpuProfiler& CpuProfiler::instance() {
    static CpuProfiler cpu_profiler;
    return cpu_profiler;
}

void CpuProfiler::update() {
    if (!m_is_paused) {
        m_frames[++m_current_frame & FRAME_MASK] = m_current_scope & SCOPE_MASK;
    }

    if (m_is_resume_scheduled) {
        m_is_paused = false;
        m_is_resume_scheduled = false;
    } else if (m_is_pause_scheduled) {
        m_is_paused = true;
        m_is_pause_scheduled = false;
    }
}

bool CpuProfiler::is_paused() const {
    // `is_paused` acts as if `toggle_pause` is immediate.
    return (m_is_paused || m_is_pause_scheduled) & !m_is_resume_scheduled;
}

void CpuProfiler::toggle_pause(bool value) {
    // Finish recording the current frame / start recording the next frame from start.
    if (value != is_paused()) {
        if (value) {
            if (m_is_resume_scheduled) {
                m_is_resume_scheduled = false;
            } else {
                m_is_pause_scheduled = true;
            }
        } else {
            if (m_is_pause_scheduled) {
                m_is_pause_scheduled = false;
            } else {
                m_is_resume_scheduled = true;
            }
        }
    }
}

size_t CpuProfiler::get_frame_count() const {
    // One extra frame is needed for `rend` calculation.
    return FRAME_COUNT - 1;
}

Vector<CpuProfiler::Scope> CpuProfiler::get_scopes(MemoryResource& memory_resource, size_t relative_frame) const {
    Vector<Scope> result(memory_resource);

    // One extra frame is needed for `rend` calculation.
    relative_frame = std::min(relative_frame, FRAME_COUNT - 2);

    size_t scope_index_rbegin = m_frames[(m_current_frame + FRAME_COUNT - relative_frame) & FRAME_MASK];
    size_t scope_index_rend = m_frames[(m_current_frame + FRAME_COUNT - relative_frame - 1) & FRAME_MASK];

    size_t scope_count;
    if (scope_index_rbegin >= scope_index_rend) {
        scope_count = scope_index_rbegin - scope_index_rend;
    } else {
        scope_count = SCOPE_COUNT - scope_index_rend + scope_index_rbegin;
    }

    result.resize(scope_count);

    for (size_t i = 0, j = scope_index_rend + 1; i < scope_count; i++, j++) {
        result[i] = m_scopes[j & SCOPE_MASK];
    }

    std::sort(result.begin(), result.end(), [](const Scope& lhs, const Scope& rhs) {
        if (lhs.begin_timestamp == rhs.begin_timestamp) {
            return lhs.end_timestamp < rhs.end_timestamp;
        }
        return lhs.begin_timestamp < rhs.begin_timestamp;
    });

    return result;
}

} // namespace kw
