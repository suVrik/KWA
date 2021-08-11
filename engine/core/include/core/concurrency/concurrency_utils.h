#pragma once

#include <thread>

namespace kw::ConcurrencyUtils {

// Each enabled bit allows the thread to run on the corresponding CPU core.
void set_thread_affinity(std::thread& thread, uint64_t affinity_mask);

// Access thread name for debugging convenience.
const char* get_current_thread_name();
void set_current_thread_name(const char* name);

} // namespace kw::ConcurrencyUtils
