#pragma once

#include <core/math/float4x4.h>

#include <imgui.h>

namespace kw {

class Input;
class MemoryResource;
class Window;

struct ImguiManagerDescriptor {
    Input* input;
    Window* window;
    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

class ImguiManager {
public:
    explicit ImguiManager(const ImguiManagerDescriptor& descriptor);

    // Must be called every frame before any ImGui call.
    void update();

    // This object uses persistent memory allocator and, unlike `ImGui` namespace, different `ImGui` instances can be
    // accessed from different threads simultaneously. The same ImGui instance can be accessed only from one thread!
    ImGui& get_imgui();

private:
    Input& m_input;
    Window& m_window;
    MemoryResource& m_persistent_memory_resource;
    MemoryResource& m_transient_memory_resource;

    ImGui m_imgui;
};

} // namespace kw
