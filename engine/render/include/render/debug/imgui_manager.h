#pragma once

#include <core/math/float4x4.h>

#include <imgui.h>

namespace kw {

class Input;
class MemoryResource;
class Window;

class ImguiManager {
public:
    ImguiManager(Input& input, Window& window, MemoryResource& transient_memory_resource, MemoryResource& persistent_memory_resource);

    // Must be called every frame before any ImGui call.
    void update();

    // This object uses persistent memory allocator and, unlike `ImGui` namespace, different `ImGui` instances can be
    // accessed from different threads simultaneously.
    ImGui& get_imgui();

private:
    Input& m_input;
    Window& m_window;
    MemoryResource& m_transient_memory_resource;
    MemoryResource& m_persistent_memory_resource;

    ImGui m_imgui;
};

} // namespace kw
