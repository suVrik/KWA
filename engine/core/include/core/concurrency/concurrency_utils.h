#pragma once

#include <thread>

namespace kw::ConcurrencyUtils {

// Each enabled bit allows the thread to run on the corresponding CPU core.
void set_thread_affinity(std::thread& thread, uint64_t affinity_mask);

// Set thread name for debugging convenience.
void set_thread_name(std::thread& thread, const char* name);

} // namespace kw::ConcurrencyUtils
