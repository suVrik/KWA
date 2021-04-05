#include "system/clipboard_utils.h"

#include <SDL2/SDL_clipboard.h>

namespace kw::ClipboardUtils {

String get_clipboard_text(MemoryResource& memory_resource) {
    char* clipboard_text = SDL_GetClipboardText();
    String result(clipboard_text, memory_resource);
    SDL_free(clipboard_text);
    return result;
}

void set_clipboard_text(const String& text) {
    SDL_SetClipboardText(text.c_str());
}

} // namespace kw::ClipboardUtils
