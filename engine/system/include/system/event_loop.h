#pragma once

#include <cstdint>

namespace kw {

class Window;

enum class EventType {
    QUIT,
    SIZE_CHANGED,
};

constexpr size_t EVENT_TYPE_COUNT = 2;

union Event {
    EventType type;

    struct {
        EventType type;
        Window* window;
        uint32_t width;
        uint32_t height;
    } size_changed;
};

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    bool poll_event(Event& event);
};

} // namespace kw
