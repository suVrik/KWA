#include <system/event_loop.h>

#include <core/error.h>

#include <debug/assert.h>

#include <SDL2/SDL.h>

namespace kw {

EventLoop::EventLoop() {
    KW_ASSERT(!SDL_WasInit(SDL_INIT_VIDEO), "Only one event loop must exist at a time.");
    KW_ERROR(SDL_Init(SDL_INIT_VIDEO) == 0, "Failed to initialize SDL.");
}

EventLoop::~EventLoop() {
    SDL_Quit();
}

bool EventLoop::poll_event(Event& event) {
    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event) != 0) {
        switch (sdl_event.type) {
        case SDL_QUIT:
            event.type = EventType::QUIT;
            return true;
        case SDL_WINDOWEVENT: {
            SDL_Window* sdl_window = SDL_GetWindowFromID(sdl_event.window.windowID);
            if (sdl_window != nullptr) {
                auto* window = static_cast<Window*>(SDL_GetWindowData(sdl_window, "Window"));
                KW_ASSERT(window != nullptr);

                switch (sdl_event.window.event) {
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    event.size_changed.type = EventType::SIZE_CHANGED;
                    event.size_changed.window = window;
                    event.size_changed.width = static_cast<uint32_t>(sdl_event.window.data1);
                    event.size_changed.height = static_cast<uint32_t>(sdl_event.window.data2);
                    return true;
                }
            }
        }
        }
    }
    return false;
}

} // namespace kw
