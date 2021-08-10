#pragma once

#include "render/render_passes/full_screen_quad_render_pass.h"

namespace kw {

struct TonemappingRenderPassDescriptor {
    Render* render;
    MemoryResource* transient_memory_resource;
};

class TonemappingRenderPass : public FullScreenQuadRenderPass {
public:
    explicit TonemappingRenderPass(const TonemappingRenderPassDescriptor& descriptor);

    void get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) override;
    void get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) override;
    void get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) override;
    void create_graphics_pipelines(FrameGraph& frame_graph) override;
    void destroy_graphics_pipelines(FrameGraph& frame_graph) override;

    // Must be placed between acquire and present frame graph's tasks.
    Task* create_task();

private:
    class Task;

    MemoryResource& m_transient_memory_resource;

    GraphicsPipeline* m_graphics_pipeline;
};

} // namespace kw
