#include "system/clipboard_utils.h"

#include <memory/memory_resource.h>

#include <SDL2/SDL_clipboard.h>

#include <cstring>

namespace kw::ClipboardUtils {

const char* get_clipboard_text(MemoryResource& memory_resource) {
    char* clipboard_text = SDL_GetClipboardText();
    size_t clipboard_text_length = strlen(clipboard_text);
    
    char* result = memory_resource.allocate<char>(clipboard_text_length + 1);
    std::memcpy(result, clipboard_text, clipboard_text_length + 1);
    
    SDL_free(clipboard_text);

    return result;
}

void set_clipboard_text(const char* text) {
    SDL_SetClipboardText(text);
}

} // namespace kw::ClipboardUtils
