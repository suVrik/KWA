#include "concurrency/concurrency_utils.h"

#include <Windows.h>

namespace kw::ConcurrencyUtils {

void set_thread_affinity(std::thread& thread, uint64_t affinity_mask) {
    SetThreadAffinityMask(thread.native_handle(), static_cast<DWORD_PTR>(affinity_mask));
}

} // namespace kw::ConcurrencyUtils
