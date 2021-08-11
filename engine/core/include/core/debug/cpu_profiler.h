#pragma once

#include "core/containers/vector.h"

#include <atomic>

namespace kw {

class CpuProfiler {
public:
    class Counter {
    public:
        explicit Counter(const char* name);
        ~Counter();

    private:
        const char* m_scope_name;
        uint64_t m_begin_timestamp;
    };

    struct Scope {
        const char* scope_name;
        const char* thread_name;
        uint64_t begin_timestamp;
        uint64_t end_timestamp;
    };

    static CpuProfiler& instance();

    // Must be called after frame execution.
    void update();

    bool is_paused() const;
    void toggle_pause(bool value);

    size_t get_frame_count() const;

    // `relative_frame = 0` is current frame. `relative_frame = -1` is previous frame. And so on.
    Vector<Scope> get_scopes(MemoryResource& memory_resource, size_t relative_frame = 0) const;

private:
    CpuProfiler();

    Vector<Scope> m_scopes;
    std::atomic_size_t m_current_scope;

    Vector<size_t> m_frames;
    size_t m_current_frame;

    bool m_is_paused;

    bool m_is_pause_scheduled;
    bool m_is_resume_scheduled;
};

} // namespace kw

#ifndef KW_CPU_PROFILER_DISABLE
#define KW_CONCAT_IMPL(x, y) x##y
#define KW_CONCAT(x, y) KW_CONCAT_IMPL(x, y)
#define KW_CPU_PROFILER(name) CpuProfiler::Counter KW_CONCAT(__CPU_PROFILER_COUNTER_, __COUNTER__ )(name)
#else
#define KW_CPU_PROFILER(name) ((void)0)
#endif // KW_CPU_PROFILER_DISABLE
