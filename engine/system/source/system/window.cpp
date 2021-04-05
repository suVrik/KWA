#include <system/window.h>

#include <core/error.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

namespace kw {

Window::Window(const WindowDescriptor& descriptor) {
    m_window = SDL_CreateWindow(descriptor.title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, static_cast<int>(descriptor.width), static_cast<int>(descriptor.height), SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_VULKAN);
    KW_ERROR(m_window != nullptr, "Failed to create window");

    SDL_SetWindowData(m_window, "Window", this);

    m_cursors[0] = SDL_GetDefaultCursor();
    m_cursors[1] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    m_cursors[2] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
    m_cursors[3] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    m_cursors[4] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    m_cursors[5] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
    m_cursors[6] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
    m_cursors[7] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    m_cursors[8] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);
    m_cursors[9] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
}

Window::~Window() {
    // m_cursors[i] is acquired from `SDL_GetDefaultCursor` and must not be freed.
    for (size_t i = 1; i < CURSOR_COUNT; i++) {
        SDL_FreeCursor(m_cursors[i]);
    }

    SDL_DestroyWindow(m_window);
}

String Window::get_title(MemoryResource& memory_resource) const {
    return String(SDL_GetWindowTitle(m_window), memory_resource);
}

void Window::set_title(const String& title) {
    SDL_SetWindowTitle(m_window, title.c_str());
}

Cursor Window::get_cursor() const {
    SDL_Cursor* cursor = SDL_GetCursor();
    for (size_t i = 0; i < CURSOR_COUNT; i++) {
        if (cursor == m_cursors[i]) {
            return static_cast<Cursor>(i);
        }
    }
    return Cursor::ARROW;
}

void Window::set_cursor(Cursor cursor) {
    SDL_SetCursor(m_cursors[static_cast<size_t>(cursor)]);
}

bool Window::is_cursor_shown() const {
    return SDL_ShowCursor(-1) == 1;
}

void Window::toggle_cursor(bool is_shown) {
    SDL_ShowCursor(is_shown ? 1 : 0);
}

uint32_t Window::get_width() const {
    uint32_t result;
    SDL_GetWindowSize(m_window, reinterpret_cast<int*>(&result), nullptr);
    return result;
}

void Window::set_width(uint32_t value) {
    SDL_SetWindowSize(m_window, static_cast<int>(value), get_height());
}

uint32_t Window::get_height() const {
    uint32_t result;
    SDL_GetWindowSize(m_window, nullptr, reinterpret_cast<int*>(&result));
    return result;
}

void Window::set_height(uint32_t value) {
    SDL_SetWindowSize(m_window, get_width(), static_cast<int>(value));
}

uint32_t Window::get_render_width() const {
    uint32_t result;
    SDL_Vulkan_GetDrawableSize(m_window, reinterpret_cast<int*>(&result), nullptr);
    return result;
}

uint32_t Window::get_render_height() const {
    uint32_t result;
    SDL_Vulkan_GetDrawableSize(m_window, nullptr, reinterpret_cast<int*>(&result));
    return result;
}

SDL_Window* Window::get_sdl_window() const {
    return m_window;
}

} // namespace kw
