#include "render/debug/imgui_manager.h"

#include <system/clipboard_utils.h>
#include <system/input.h>
#include <system/window.h>

#include <core/debug/assert.h>

namespace kw {

static const Cursor CURSOR_MAPPING[] = {
    Cursor::ARROW,       // ImGuiMouseCursor_Arrow
    Cursor::TEXT_INPUT,  // ImGuiMouseCursor_TextInput
    Cursor::RESIZE_ALL,  // ImGuiMouseCursor_ResizeAll
    Cursor::RESIZE_NS,   // ImGuiMouseCursor_ResizeNS
    Cursor::RESIZE_EW,   // ImGuiMouseCursor_ResizeEW
    Cursor::RESIZE_NESW, // ImGuiMouseCursor_ResizeNESW
    Cursor::RESIZE_NWSE, // ImGuiMouseCursor_ResizeNWSE
    Cursor::HAND,        // ImGuiMouseCursor_Hand
    Cursor::NOT_ALLOWED, // ImGuiMouseCursor_NotAllowed
};

static void* imgui_alloc(size_t size, void* userdata) {
    MemoryResource* memory_resource = static_cast<MemoryResource*>(userdata);
    KW_ASSERT(memory_resource != nullptr);

    return memory_resource->allocate(size, 1);
}

static void imgui_free(void* memory, void* userdata) {
    MemoryResource* memory_resource = static_cast<MemoryResource*>(userdata);
    KW_ASSERT(memory_resource != nullptr);

    memory_resource->deallocate(memory);
}

static const char* get_clipboard_text(void* user_data) {
    return ClipboardUtils::get_clipboard_text(*static_cast<MemoryResource*>(user_data));
}

static void set_clipboard_text(void* user_data, const char* text) {
    ClipboardUtils::set_clipboard_text(text);
}

ImguiManager::ImguiManager(Input& input, Window& window, MemoryResource& persistent_memory_resource, MemoryResource& transient_memory_resource)
    : m_input(input)
    , m_window(window)
    , m_persistent_memory_resource(persistent_memory_resource)
    , m_transient_memory_resource(transient_memory_resource)
    , m_imgui({ imgui_alloc, imgui_free, &persistent_memory_resource })
{
    IMGUI_CHECKVERSION(m_imgui);

    ImGuiIO& io = m_imgui.GetIO();

    io.BackendFlags = ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos;

    io.KeyMap[ImGuiKey_Tab]         = static_cast<int>(Scancode::TAB);
    io.KeyMap[ImGuiKey_LeftArrow]   = static_cast<int>(Scancode::LEFT);
    io.KeyMap[ImGuiKey_RightArrow]  = static_cast<int>(Scancode::RIGHT);
    io.KeyMap[ImGuiKey_UpArrow]     = static_cast<int>(Scancode::UP);
    io.KeyMap[ImGuiKey_DownArrow]   = static_cast<int>(Scancode::DOWN);
    io.KeyMap[ImGuiKey_PageUp]      = static_cast<int>(Scancode::PAGEUP);
    io.KeyMap[ImGuiKey_PageDown]    = static_cast<int>(Scancode::PAGEDOWN);
    io.KeyMap[ImGuiKey_Home]        = static_cast<int>(Scancode::HOME);
    io.KeyMap[ImGuiKey_End]         = static_cast<int>(Scancode::END);
    io.KeyMap[ImGuiKey_Insert]      = static_cast<int>(Scancode::INSERT);
    io.KeyMap[ImGuiKey_Delete]      = static_cast<int>(Scancode::DELETE);
    io.KeyMap[ImGuiKey_Backspace]   = static_cast<int>(Scancode::BACKSPACE);
    io.KeyMap[ImGuiKey_Space]       = static_cast<int>(Scancode::SPACE);
    io.KeyMap[ImGuiKey_Enter]       = static_cast<int>(Scancode::RETURN);
    io.KeyMap[ImGuiKey_Escape]      = static_cast<int>(Scancode::ESCAPE);
    io.KeyMap[ImGuiKey_KeyPadEnter] = static_cast<int>(Scancode::KP_ENTER);
    io.KeyMap[ImGuiKey_A]           = static_cast<int>(Scancode::A);
    io.KeyMap[ImGuiKey_C]           = static_cast<int>(Scancode::C);
    io.KeyMap[ImGuiKey_V]           = static_cast<int>(Scancode::V);
    io.KeyMap[ImGuiKey_X]           = static_cast<int>(Scancode::X);
    io.KeyMap[ImGuiKey_Y]           = static_cast<int>(Scancode::Y);
    io.KeyMap[ImGuiKey_Z]           = static_cast<int>(Scancode::Z);

    io.GetClipboardTextFn = get_clipboard_text;
    io.SetClipboardTextFn = set_clipboard_text;
    io.ClipboardUserData = &m_transient_memory_resource;
}

void ImguiManager::update() {
    ImGuiIO& io = m_imgui.GetIO();

    io.MousePos.x   = static_cast<float>(m_input.get_mouse_x());
    io.MousePos.y   = static_cast<float>(m_input.get_mouse_y());
    io.MouseWheel   = static_cast<float>(m_input.get_mouse_wheel());
    io.MouseDown[0] = m_input.is_button_down(BUTTON_LEFT);
    io.MouseDown[1] = m_input.is_button_down(BUTTON_RIGHT);
    io.MouseDown[2] = m_input.is_button_down(BUTTON_MIDDLE);

    for (size_t i = 0; i < std::size(io.KeysDown) && i < SCANCODE_COUNT; i++) {
        io.KeysDown[i] = m_input.is_key_down(static_cast<Scancode>(i));
    }

    io.KeyShift = m_input.is_key_down(Scancode::LSHIFT) || m_input.is_key_down(Scancode::RSHIFT);
    io.KeyCtrl  = m_input.is_key_down(Scancode::LCTRL)  || m_input.is_key_down(Scancode::RCTRL);
    io.KeyAlt   = m_input.is_key_down(Scancode::LALT)   || m_input.is_key_down(Scancode::RALT);
    io.KeySuper = m_input.is_key_down(Scancode::LGUI)   || m_input.is_key_down(Scancode::RGUI);

    String text = m_input.get_text(m_transient_memory_resource);
    io.AddInputCharactersUTF8(text.c_str());

    if (io.WantCaptureKeyboard) {
        m_input.stop_keyboard_propagation();
    }

    if (io.WantCaptureMouse) {
        m_input.stop_mouse_propagation();
    }

    // TODO: Elapsed time.
    // ImGui doesn't accept zero elapsed_time value.
    io.DeltaTime = std::max(1.f / 60.f, 1e-4f);

    io.DisplaySize.x = static_cast<float>(m_window.get_render_width());
    io.DisplaySize.y = static_cast<float>(m_window.get_render_height());
    io.DisplayFramebufferScale = ImVec2(1.f, 1.f);

    if (io.WantSetMousePos) {
        m_input.set_mouse_x(static_cast<int32_t>(io.MousePos.x));
        m_input.set_mouse_y(static_cast<int32_t>(io.MousePos.y));
    }

    ImGuiMouseCursor cursor = m_imgui.GetMouseCursor();
    if (io.MouseDrawCursor || cursor == ImGuiMouseCursor_None) {
        m_window.toggle_cursor(false);
    } else {
        if (cursor >= 0 && cursor < std::size(CURSOR_MAPPING)) {
            m_window.set_cursor(CURSOR_MAPPING[cursor]);
        } else {
            m_window.set_cursor(Cursor::ARROW);
        }
        m_window.toggle_cursor(true);
    }

    m_input.toggle_mouse_capture(io.WantCaptureMouse);

    m_imgui.NewFrame();
}

ImGui& ImguiManager::get_imgui() {
    return m_imgui;
}

} // namespace kw
