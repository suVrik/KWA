#pragma once

namespace kw {

class MemoryResource;

namespace ClipboardUtils {

char* get_clipboard_text(MemoryResource& memory_resource);

void set_clipboard_text(const char* text);

} // namespace ClipboardUtils

} // namespace kw
