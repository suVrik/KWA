#include "concurrency/concurrency_utils.h"

#include <processthreadsapi.h>
#include <Windows.h>

namespace kw::ConcurrencyUtils {

void set_thread_affinity(std::thread& thread, uint64_t affinity_mask) {
    SetThreadAffinityMask(thread.native_handle(), static_cast<DWORD_PTR>(affinity_mask));
}

void set_thread_name(std::thread& thread, const char* name) {
    WCHAR wide_name[64];
    if (MultiByteToWideChar(CP_UTF8, 0, name, -1, wide_name, std::size(wide_name)) > 0) {
        SetThreadDescription(thread.native_handle(), wide_name);
    }
}

} // namespace kw::ConcurrencyUtils
