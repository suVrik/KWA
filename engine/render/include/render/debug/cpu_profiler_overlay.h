#pragma once

namespace kw {

class ImguiManager;
class MemoryResource;

struct CpuProfilerOverlayDescriptor {
    ImguiManager* imgui_manager;
    MemoryResource* transient_memory_resource;
};

class CpuProfilerOverlay {
public:
    explicit CpuProfilerOverlay(const CpuProfilerOverlayDescriptor& descriptor);

    // Not an asynchronous task because the same ImGui instance can't be accessed from different threads.
    void update();

private:
    ImguiManager& m_imgui_manager;
    MemoryResource& m_transient_memory_resource;

    int m_offset;
};

} // namespace kw
