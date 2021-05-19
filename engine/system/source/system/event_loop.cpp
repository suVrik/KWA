#include <system/event_loop.h>
#include <system/window.h>

#include <core/error.h>
#include <core/memory/memory_resource.h>

#include <debug/assert.h>

#include <SDL2/SDL.h>

namespace kw {

static Window* get_window_from_window_id(Uint32 window_id) {
    SDL_Window* sdl_window = SDL_GetWindowFromID(window_id);
    if (sdl_window != nullptr) {
        Window* window = static_cast<Window*>(SDL_GetWindowData(sdl_window, "Window"));
        KW_ASSERT(window != nullptr);
        return window;
    }
    return nullptr;
}

static Scancode SCANCODE_MAPPING[] = {
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::A,
    Scancode::B,
    Scancode::C,
    Scancode::D,
    Scancode::E,
    Scancode::F,
    Scancode::G,
    Scancode::H,
    Scancode::I,
    Scancode::J,
    Scancode::K,
    Scancode::L,
    Scancode::M,
    Scancode::N,
    Scancode::O,
    Scancode::P,
    Scancode::Q,
    Scancode::R,
    Scancode::S,
    Scancode::T,
    Scancode::U,
    Scancode::V,
    Scancode::W,
    Scancode::X,
    Scancode::Y,
    Scancode::Z,
    Scancode::DIGIT_1,
    Scancode::DIGIT_2,
    Scancode::DIGIT_3,
    Scancode::DIGIT_4,
    Scancode::DIGIT_5,
    Scancode::DIGIT_6,
    Scancode::DIGIT_7,
    Scancode::DIGIT_8,
    Scancode::DIGIT_9,
    Scancode::DIGIT_0,
    Scancode::RETURN,
    Scancode::ESCAPE,
    Scancode::BACKSPACE,
    Scancode::TAB,
    Scancode::SPACE,
    Scancode::MINUS,
    Scancode::EQUALS,
    Scancode::LEFTBRACKET,
    Scancode::RIGHTBRACKET,
    Scancode::BACKSLASH,
    Scancode::NONUSHASH,
    Scancode::SEMICOLON,
    Scancode::APOSTROPHE,
    Scancode::GRAVE,
    Scancode::COMMA,
    Scancode::PERIOD,
    Scancode::SLASH,
    Scancode::CAPSLOCK,
    Scancode::F1,
    Scancode::F2,
    Scancode::F3,
    Scancode::F4,
    Scancode::F5,
    Scancode::F6,
    Scancode::F7,
    Scancode::F8,
    Scancode::F9,
    Scancode::F10,
    Scancode::F11,
    Scancode::F12,
    Scancode::PRINTSCREEN,
    Scancode::SCROLLLOCK,
    Scancode::PAUSE,
    Scancode::INSERT,
    Scancode::HOME,
    Scancode::PAGEUP,
    Scancode::DELETE,
    Scancode::END,
    Scancode::PAGEDOWN,
    Scancode::RIGHT,
    Scancode::LEFT,
    Scancode::DOWN,
    Scancode::UP,
    Scancode::NUMLOCKCLEAR,
    Scancode::KP_DIVIDE,
    Scancode::KP_MULTIPLY,
    Scancode::KP_MINUS,
    Scancode::KP_PLUS,
    Scancode::KP_ENTER,
    Scancode::KP_1,
    Scancode::KP_2,
    Scancode::KP_3,
    Scancode::KP_4,
    Scancode::KP_5,
    Scancode::KP_6,
    Scancode::KP_7,
    Scancode::KP_8,
    Scancode::KP_9,
    Scancode::KP_0,
    Scancode::KP_PERIOD,
    Scancode::NONUSBACKSLASH,
    Scancode::APPLICATION,
    Scancode::POWER,
    Scancode::KP_EQUALS,
    Scancode::F13,
    Scancode::F14,
    Scancode::F15,
    Scancode::F16,
    Scancode::F17,
    Scancode::F18,
    Scancode::F19,
    Scancode::F20,
    Scancode::F21,
    Scancode::F22,
    Scancode::F23,
    Scancode::F24,
    Scancode::EXECUTE,
    Scancode::HELP,
    Scancode::MENU,
    Scancode::SELECT,
    Scancode::STOP,
    Scancode::AGAIN,
    Scancode::UNDO,
    Scancode::CUT,
    Scancode::COPY,
    Scancode::PASTE,
    Scancode::FIND,
    Scancode::MUTE,
    Scancode::VOLUMEUP,
    Scancode::VOLUMEDOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::KP_COMMA,
    Scancode::KP_EQUALSAS400,
    Scancode::INTERNATIONAL1,
    Scancode::INTERNATIONAL2,
    Scancode::INTERNATIONAL3,
    Scancode::INTERNATIONAL4,
    Scancode::INTERNATIONAL5,
    Scancode::INTERNATIONAL6,
    Scancode::INTERNATIONAL7,
    Scancode::INTERNATIONAL8,
    Scancode::INTERNATIONAL9,
    Scancode::LANG1,
    Scancode::LANG2,
    Scancode::LANG3,
    Scancode::LANG4,
    Scancode::LANG5,
    Scancode::LANG6,
    Scancode::LANG7,
    Scancode::LANG8,
    Scancode::LANG9,
    Scancode::ALTERASE,
    Scancode::SYSREQ,
    Scancode::CANCEL,
    Scancode::CLEAR,
    Scancode::PRIOR,
    Scancode::RETURN2,
    Scancode::SEPARATOR,
    Scancode::OUT,
    Scancode::OPER,
    Scancode::CLEARAGAIN,
    Scancode::CRSEL,
    Scancode::EXSEL,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::KP_00,
    Scancode::KP_000,
    Scancode::THOUSANDSSEPARATOR,
    Scancode::DECIMALSEPARATOR,
    Scancode::CURRENCYUNIT,
    Scancode::CURRENCYSUBUNIT,
    Scancode::KP_LEFTPAREN,
    Scancode::KP_RIGHTPAREN,
    Scancode::KP_LEFTBRACE,
    Scancode::KP_RIGHTBRACE,
    Scancode::KP_TAB,
    Scancode::KP_BACKSPACE,
    Scancode::KP_A,
    Scancode::KP_B,
    Scancode::KP_C,
    Scancode::KP_D,
    Scancode::KP_E,
    Scancode::KP_F,
    Scancode::KP_XOR,
    Scancode::KP_POWER,
    Scancode::KP_PERCENT,
    Scancode::KP_LESS,
    Scancode::KP_GREATER,
    Scancode::KP_AMPERSAND,
    Scancode::KP_DBLAMPERSAND,
    Scancode::KP_VERTICALBAR,
    Scancode::KP_DBLVERTICALBAR,
    Scancode::KP_COLON,
    Scancode::KP_HASH,
    Scancode::KP_SPACE,
    Scancode::KP_AT,
    Scancode::KP_EXCLAM,
    Scancode::KP_MEMSTORE,
    Scancode::KP_MEMRECALL,
    Scancode::KP_MEMCLEAR,
    Scancode::KP_MEMADD,
    Scancode::KP_MEMSUBTRACT,
    Scancode::KP_MEMMULTIPLY,
    Scancode::KP_MEMDIVIDE,
    Scancode::KP_PLUSMINUS,
    Scancode::KP_CLEAR,
    Scancode::KP_CLEARENTRY,
    Scancode::KP_BINARY,
    Scancode::KP_OCTAL,
    Scancode::KP_DECIMAL,
    Scancode::KP_HEXADECIMAL,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::LCTRL,
    Scancode::LSHIFT,
    Scancode::LALT,
    Scancode::LGUI,
    Scancode::RCTRL,
    Scancode::RSHIFT,
    Scancode::RALT,
    Scancode::RGUI,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::UNKNOWN,
    Scancode::MODE,
    Scancode::AUDIONEXT,
    Scancode::AUDIOPREV,
    Scancode::AUDIOSTOP,
    Scancode::AUDIOPLAY,
    Scancode::AUDIOMUTE,
    Scancode::MEDIASELECT,
    Scancode::WWW,
    Scancode::MAIL,
    Scancode::CALCULATOR,
    Scancode::COMPUTER,
    Scancode::AC_SEARCH,
    Scancode::AC_HOME,
    Scancode::AC_BACK,
    Scancode::AC_FORWARD,
    Scancode::AC_STOP,
    Scancode::AC_REFRESH,
    Scancode::AC_BOOKMARKS,
    Scancode::BRIGHTNESSDOWN,
    Scancode::BRIGHTNESSUP,
    Scancode::DISPLAYSWITCH,
    Scancode::KBDILLUMTOGGLE,
    Scancode::KBDILLUMDOWN,
    Scancode::KBDILLUMUP,
    Scancode::EJECT,
    Scancode::SLEEP,
    Scancode::APP1,
    Scancode::APP2,
    Scancode::AUDIOREWIND,
    Scancode::AUDIOFASTFORWARD,
};

EventLoop::EventLoop() {
    KW_ASSERT(!SDL_WasInit(SDL_INIT_VIDEO), "Only one event loop must exist at a time.");
    KW_ERROR(SDL_Init(SDL_INIT_VIDEO) == 0, "Failed to initialize SDL.");
}

EventLoop::~EventLoop() {
    SDL_Quit();
}

bool EventLoop::poll_event(MemoryResource& memory_resource, Event& event) {
    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event) != 0) {
        switch (sdl_event.type) {
        case SDL_QUIT:
            event.type = EventType::QUIT;
            return true;
        case SDL_WINDOWEVENT: {
            Window* window = get_window_from_window_id(sdl_event.window.windowID);
            if (window != nullptr) {
                switch (sdl_event.window.event) {
                case SDL_WINDOWEVENT_MINIMIZED:
                    event.size_changed.type = EventType::SIZE_CHANGED;
                    event.size_changed.window = window;
                    event.size_changed.width = 0;
                    event.size_changed.height = 0;
                    return true;
                case SDL_WINDOWEVENT_MAXIMIZED:
                case SDL_WINDOWEVENT_RESTORED:
                    event.size_changed.type = EventType::SIZE_CHANGED;
                    event.size_changed.window = window;
                    event.size_changed.width = window->get_width();
                    event.size_changed.height = window->get_height();
                    return true;
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    event.size_changed.type = EventType::SIZE_CHANGED;
                    event.size_changed.window = window;
                    event.size_changed.width = static_cast<uint32_t>(sdl_event.window.data1);
                    event.size_changed.height = static_cast<uint32_t>(sdl_event.window.data2);
                    return true;
                }
            }
            break;
        }
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            Window* window = get_window_from_window_id(sdl_event.key.windowID);
            if (window != nullptr) {
                event.key_event.type = sdl_event.type == SDL_KEYDOWN ? EventType::KEY_DOWN : EventType::KEY_UP;
                event.key_event.window = window;
                if (sdl_event.key.keysym.scancode < std::size(SCANCODE_MAPPING)) {
                    event.key_event.scancode = SCANCODE_MAPPING[static_cast<size_t>(sdl_event.key.keysym.scancode)];
                } else {
                    event.key_event.scancode = Scancode::UNKNOWN;
                }
                return true;
            }
            break;
        }
        case SDL_TEXTINPUT: {
            Window* window = get_window_from_window_id(sdl_event.text.windowID);
            if (window != nullptr) {
                size_t length = std::strlen(sdl_event.text.text);
                char* text = memory_resource.allocate<char>(length + 1);
                std::memcpy(text, sdl_event.text.text, length + 1);

                event.text.type = EventType::TEXT;
                event.text.window = window;
                event.text.text = text;

                return true;
            }
            break;
        }
        case SDL_MOUSEMOTION: {
            Window* window = get_window_from_window_id(sdl_event.motion.windowID);
            if (window != nullptr) {
                event.mouse_move.type = EventType::MOUSE_MOVE;
                event.mouse_move.window = window;
                event.mouse_move.x = sdl_event.motion.x;
                event.mouse_move.y = sdl_event.motion.y;
                event.mouse_move.dx = sdl_event.motion.xrel;
                event.mouse_move.dy = sdl_event.motion.yrel;
                return true;
            }
            break;
        }
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            Window* window = get_window_from_window_id(sdl_event.button.windowID);
            if (window != nullptr) {
                event.button_event.type = sdl_event.type == SDL_MOUSEBUTTONDOWN ? EventType::BUTTON_DOWN : EventType::BUTTON_UP;
                event.button_event.window = window;
                event.button_event.button = sdl_event.button.button;
                return true;
            }
            break;
        }
        case SDL_MOUSEWHEEL: {
            Window* window = get_window_from_window_id(sdl_event.button.windowID);
            if (window != nullptr) {
                event.mouse_wheel.type = EventType::MOUSE_WHEEL;
                event.mouse_wheel.window = window;
                event.mouse_wheel.delta = sdl_event.wheel.y;
                return true;
            }
            break;
        }
        }
    }
    return false;
}

} // namespace kw
