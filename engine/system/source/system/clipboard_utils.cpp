#include "system/clipboard_utils.h"

#include <SDL2/SDL_clipboard.h>
#include <string>

namespace kw::ClipboardUtils {

// Allocate 1024 bytes upfront and scale when needed.
static std::string clipboard_data(1024, ' ');

const char* get_clipboard_text() {
    char* clipboard_text = SDL_GetClipboardText();
    clipboard_data = clipboard_text;
    SDL_free(clipboard_text);
    return clipboard_data.c_str();
}

void set_clipboard_text(const char* text) {
    SDL_SetClipboardText(text);
}

} // namespace kw::ClipboardUtils
