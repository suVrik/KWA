#pragma once

#include "render/frame_graph.h"

#include <core/containers/vector.h>

namespace kw {

class BaseRenderPass : public RenderPass {
public:
    // Fill color attachments created by this render pass.
    virtual void get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) = 0;

    // Fill depth stencil attachment created by this render pass.
    virtual void get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) = 0;

    // Fill render pass descriptors created by this render pass.
    virtual void get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) = 0;

    // Create graphics pipelines for this render pass.
    virtual void create_graphics_pipelines(FrameGraph& frame_graph) = 0;

    // Destroy graphics pipelines for this render pass.
    virtual void destroy_graphics_pipelines(FrameGraph& frame_graph) = 0;
};

} // namespace kw
