#pragma once

#include "system/event_loop.h"

#include <core/string.h>

namespace kw {

class Input {
public:
    Input(Window& window);
    ~Input();

    void push_event(const Event& event);

    // Must be called once all events on current frame were pushed.
    void update();

    bool is_key_pressed(Scancode scancode);
    bool is_key_released(Scancode scancode);
    bool is_key_down(Scancode scancode);

    String get_text(MemoryResource& memory_resource) const;

    bool is_button_pressed(uint32_t button);
    bool is_button_released(uint32_t button);
    bool is_button_down(uint32_t button);

    int32_t get_mouse_x() const;
    void set_mouse_x(int32_t value);
    int32_t get_mouse_dx() const;

    int32_t get_mouse_y() const;
    void set_mouse_y(int32_t value);
    int32_t get_mouse_dy() const;

    int32_t get_mouse_wheel() const;

    bool is_mouse_relative() const;
    void set_mouse_relative(bool is_relative);

    // All the currently pressed keyboard controls will be released. On the next update all pressed keys will be in
    // just pressed state. Designed specifically for ImGui's "capture_keyboard" feature.
    void stop_keyboard_propagation();

    // All the currently pressed mouse controls will be released. On the next update all pressed keys will be in just
    // pressed state. Designed specifically for ImGui's "capture_mouse" feature.
    void stop_mouse_propagation();

public:
    Window& m_window;

    bool m_previous_key[SCANCODE_COUNT]{};
    bool m_current_key[SCANCODE_COUNT]{};
    bool m_next_key[SCANCODE_COUNT]{};

    const char* m_current_text;
    const char* m_next_text;

    uint32_t m_button_previous = 0;
    uint32_t m_button_current = 0;
    uint32_t m_button_next = 0;

    int32_t m_mouse_x = 0;

    int32_t m_current_mouse_dx = 0;
    int32_t m_next_mouse_dx = 0;

    int32_t m_mouse_y = 0;

    int32_t m_current_mouse_dy = 0;
    int32_t m_next_mouse_dy = 0;

    int32_t m_current_mouse_wheel = 0;
    int32_t m_next_mouse_wheel = 0;

    bool m_is_mouse_relative = false;
};

} // namespace kw
