#pragma once

#include <core/string.h>

struct SDL_Cursor;
struct SDL_Window;

namespace kw {

struct WindowDescriptor {
    const char* title;
    uint32_t width;
    uint32_t height;
};

enum class Cursor {
    ARROW,
    TEXT_INPUT,
    RESIZE_ALL,
    RESIZE_NS,
    RESIZE_EW,
    RESIZE_NESW,
    RESIZE_NWSE,
    HAND,
    NOT_ALLOWED,
    WAIT,
};

constexpr size_t CURSOR_COUNT = 10;

class Window {
public:
    /** To create `Window` you must have an `EventLoop`. */
    Window(const WindowDescriptor& descriptor);
    ~Window();

    String get_title(MemoryResource& memory_resource) const;
    void set_title(const String& title);

    Cursor get_cursor() const;
    void set_cursor(Cursor cursor);

    bool is_cursor_shown() const;
    void toggle_cursor(bool is_shown);

    uint32_t get_width() const;
    void set_width(uint32_t value);

    uint32_t get_height() const;
    void set_height(uint32_t value); 

    uint32_t get_render_width() const;
    uint32_t get_render_height() const;

    SDL_Window* get_sdl_window() const;

private:
    SDL_Window* m_window;
    SDL_Cursor* m_cursors[CURSOR_COUNT];
};

} // namespace kw
