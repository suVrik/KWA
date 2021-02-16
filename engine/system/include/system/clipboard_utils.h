#pragma once

namespace kw::ClipboardUtils {

/** Result is valid until the next call. */
const char* get_clipboard_text();

void set_clipboard_text(const char* text);

} // namespace kw::ClipboardUtils
