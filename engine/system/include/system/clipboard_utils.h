#pragma once

#include <core/string.h>

namespace kw::ClipboardUtils {

StringLinear get_clipboard_text(MemoryResourceLinear* memory_resource);

void set_clipboard_text(const StringLinear& text);

} // namespace kw::ClipboardUtils
