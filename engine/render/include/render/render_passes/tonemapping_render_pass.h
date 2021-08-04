#pragma once

#include "render/render_passes/full_screen_quad_render_pass.h"

#include <core/containers/vector.h>

namespace kw {

class TonemappingRenderPass : public FullScreenQuadRenderPass {
public:
    TonemappingRenderPass(Render& render, MemoryResource& transient_memory_resource);

    // Fill color attachments created by this render pass.
    void get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors);

    // Fill depth stencil attachment created by this render pass.
    void get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors);

    // Fill render pass descriptors created by this render pass.
    void get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors);

    // Create graphics pipelines for this render pass
    void create_graphics_pipelines(FrameGraph& frame_graph);

    // Destroy graphics pipelines for this render pass.
    void destroy_graphics_pipelines(FrameGraph& frame_graph);

    // Create task that performs tonemapping. Must be placed between frame first and second graph tasks.
    Task* create_task();

private:
    class Task;

    MemoryResource& m_transient_memory_resource;

    GraphicsPipeline* m_graphics_pipeline;
};

} // namespace kw
