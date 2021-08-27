#include "system/input.h"
#include "system/window.h"

#include <SDL_mouse.h>

namespace kw {

Input::Input(Window& window)
    : m_window(window)
    , m_current_text("")
    , m_next_text("")
{
}

void Input::push_event(const Event& event) {
    switch (event.type) {
    case EventType::KEY_DOWN:
        if (static_cast<size_t>(event.key_event.scancode) < SCANCODE_COUNT) {
            m_next_key[static_cast<size_t>(event.key_event.scancode)] = true;
        }
        break;
    case EventType::KEY_UP:
        if (static_cast<size_t>(event.key_event.scancode) < SCANCODE_COUNT) {
            m_next_key[static_cast<size_t>(event.key_event.scancode)] = false;
        }
        break;
    case EventType::TEXT:
        // Valid until transient resource clear.
        m_next_text = event.text.text;
        break;
    case EventType::BUTTON_DOWN:
        if (event.button_event.button < 32) {
            m_button_next |= 1 << event.button_event.button;
        }
        break;
    case EventType::BUTTON_UP:
        if (event.button_event.button < 32) {
            m_button_next &= ~(1 << event.button_event.button);
        }
        break;
    case EventType::MOUSE_MOVE:
        m_mouse_x = event.mouse_move.x;
        m_mouse_y = event.mouse_move.y;
        m_next_mouse_dx = event.mouse_move.dx;
        m_next_mouse_dy = event.mouse_move.dy;
        break;
    case EventType::MOUSE_WHEEL:
        m_next_mouse_wheel = event.mouse_wheel.delta;
        break;
    }
}

void Input::update() {
    std::copy(std::begin(m_current_key), std::end(m_current_key), std::begin(m_previous_key));
    std::copy(std::begin(m_next_key), std::end(m_next_key), std::begin(m_current_key));

    m_current_key[static_cast<size_t>(Scancode::CTRL)] =
        m_current_key[static_cast<size_t>(Scancode::LCTRL)] ||
        m_current_key[static_cast<size_t>(Scancode::RCTRL)];
    
    m_current_key[static_cast<size_t>(Scancode::SHIFT)] =
        m_current_key[static_cast<size_t>(Scancode::LSHIFT)] ||
        m_current_key[static_cast<size_t>(Scancode::RSHIFT)];
    
    m_current_key[static_cast<size_t>(Scancode::ALT)] =
        m_current_key[static_cast<size_t>(Scancode::LALT)] ||
        m_current_key[static_cast<size_t>(Scancode::RALT)];
    
    m_current_key[static_cast<size_t>(Scancode::GUI)] =
        m_current_key[static_cast<size_t>(Scancode::LGUI)] ||
        m_current_key[static_cast<size_t>(Scancode::RGUI)];

    std::swap(m_current_text, m_next_text);
    m_next_text = "";

    m_button_previous = m_button_current;
    m_button_current = m_button_next;

    m_current_mouse_dx = m_next_mouse_dx;
    m_next_mouse_dx = 0;

    m_current_mouse_dy = m_next_mouse_dy;
    m_next_mouse_dy = 0;

    m_current_mouse_wheel = m_next_mouse_wheel;
    m_next_mouse_wheel = 0;
}

bool Input::is_key_pressed(Scancode scancode) {
    if (static_cast<size_t>(scancode) < SCANCODE_COUNT) {
        return !m_previous_key[static_cast<size_t>(scancode)] && m_current_key[static_cast<size_t>(scancode)];
    }
    return false;
}

bool Input::is_key_down(Scancode scancode) {
    if (static_cast<size_t>(scancode) < SCANCODE_COUNT) {
        return m_current_key[static_cast<size_t>(scancode)];
    }
    return false;
}

bool Input::is_key_released(Scancode scancode) {
    if (static_cast<size_t>(scancode) < SCANCODE_COUNT) {
        return m_previous_key[static_cast<size_t>(scancode)] && !m_current_key[static_cast<size_t>(scancode)];
    }
    return false;
}

String Input::get_text(MemoryResource& memory_resource) const {
    return String(m_current_text, memory_resource);
}

bool Input::is_button_pressed(uint32_t button) {
    if (button < 32) {
        return (m_button_previous & (1 << button)) == 0 && (m_button_current & (1 << button)) != 0;
    }
    return false;
}

bool Input::is_button_down(uint32_t button) {
    if (button < 32) {
        return (m_button_current & (1 << button)) != 0;
    }
    return false;
}

bool Input::is_button_released(uint32_t button) {
    if (button < 32) {
        return (m_button_previous & (1 << button)) != 0 && (m_button_current & (1 << button)) == 0;
    }
    return false;
}

int32_t Input::get_mouse_x() const {
    return m_mouse_x;
}

void Input::set_mouse_x(int32_t value) {
    SDL_WarpMouseInWindow(m_window.get_sdl_window(), value, m_mouse_y);
}

int32_t Input::get_mouse_dx() const {
    return m_current_mouse_dx;
}

int32_t Input::get_mouse_y() const {
    return m_mouse_y;
}

void Input::set_mouse_y(int32_t value) {
    SDL_WarpMouseInWindow(m_window.get_sdl_window(), m_mouse_x, value);
}

int32_t Input::get_mouse_dy() const {
    return m_current_mouse_dy;
}

int32_t Input::get_mouse_wheel() const {
    return m_current_mouse_wheel;
}

bool Input::is_mouse_relative() const {
    return m_is_mouse_relative;
}

void Input::toggle_mouse_relative(bool is_relative) {
    if (is_relative != m_is_mouse_relative) {
        SDL_SetRelativeMouseMode(is_relative ? SDL_TRUE : SDL_FALSE);
        m_is_mouse_relative = is_relative;
    }
}

bool Input::is_mouse_capture() const {
    return m_is_mouse_capture;
}

void Input::toggle_mouse_capture(bool is_capture) {
    if (m_is_mouse_capture != is_capture) {
        SDL_CaptureMouse(is_capture ? SDL_TRUE : SDL_FALSE);
        m_is_mouse_capture = is_capture;
    }
}

void Input::stop_keyboard_propagation() {
    std::fill(std::begin(m_current_key), std::end(m_current_key), false);
    m_current_text = "";
}

void Input::stop_mouse_propagation() {
    m_button_current = 0;
    m_current_mouse_wheel = 0;
}

} // namespace kw
