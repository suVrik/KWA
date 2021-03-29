#include "system/clipboard_utils.h"

#include <SDL2/SDL_clipboard.h>

namespace kw::ClipboardUtils {

StringLinear get_clipboard_text(MemoryResourceLinear* memory_resource) {
    char* clipboard_text = SDL_GetClipboardText();
    StringLinear result(clipboard_text, memory_resource);
    SDL_free(clipboard_text);
    return result;
}

void set_clipboard_text(const StringLinear& text) {
    SDL_SetClipboardText(text.c_str());
}

} // namespace kw::ClipboardUtils
