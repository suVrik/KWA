#pragma once

#include "render/render_passes/base_render_pass.h"

namespace kw {

class ImguiManager;

class ImguiRenderPass : public BaseRenderPass {
public:
    ImguiRenderPass(Render& render, ImguiManager& imgui_manager, MemoryResource& transient_memory_resource);
    ~ImguiRenderPass();

    void get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) override;
    void get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) override;
    void get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) override;
    void create_graphics_pipelines(FrameGraph& frame_graph) override;
    void destroy_graphics_pipelines(FrameGraph& frame_graph) override;

    // Must be placed between acquire and present frame graph's tasks.
    Task* create_task();

private:
    class Task;

    Render& m_render;
    ImguiManager& m_imgui_manager;
    MemoryResource& m_transient_memory_resource;

    Texture* m_font_texture;
    GraphicsPipeline* m_graphics_pipeline;
};

} // namespace kw
