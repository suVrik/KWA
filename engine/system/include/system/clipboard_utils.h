#pragma once

#include <core/string.h>

namespace kw::ClipboardUtils {

String get_clipboard_text(MemoryResource& memory_resource);

void set_clipboard_text(const String& text);

} // namespace kw::ClipboardUtils
