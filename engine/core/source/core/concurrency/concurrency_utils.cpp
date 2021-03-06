#include "core/concurrency/concurrency_utils.h"

#include <Windows.h>
#include <processthreadsapi.h>

namespace kw::ConcurrencyUtils {

static thread_local char thread_name[32] = "Unnamed Thread";

void set_thread_affinity(std::thread& thread, uint64_t affinity_mask) {
    SetThreadAffinityMask(thread.native_handle(), static_cast<DWORD_PTR>(affinity_mask));
}

const char* get_current_thread_name() {
    return thread_name;
}

void set_current_thread_name(const char* name) {
    WCHAR wide_name[32];
    if (MultiByteToWideChar(CP_UTF8, 0, name, -1, wide_name, std::size(wide_name)) > 0) {
        SetThreadDescription(GetCurrentThread(), wide_name);
    }
    std::strncpy(thread_name, name, sizeof(thread_name) - 1);
}

} // namespace kw::ConcurrencyUtils
