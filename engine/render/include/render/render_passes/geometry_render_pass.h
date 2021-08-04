#pragma once

#include "render/frame_graph.h"

#include <core/containers/vector.h>

namespace kw {

class Scene;

class GeometryRenderPass : public RenderPass {
public:
    GeometryRenderPass(Render& render, Scene& scene, MemoryResource& transient_memory_resource);

    // Fill color attachments created by this render pass.
    void get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors);

    // Fill depth stencil attachment created by this render pass.
    void get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors);

    // Fill render pass descriptors created by this render pass.
    void get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors);

    // Create graphics pipelines for this render pass.
    void create_graphics_pipelines(FrameGraph& frame_graph);

    // Destroy graphics pipelines for this render pass.
    void destroy_graphics_pipelines(FrameGraph& frame_graph);

    // Create task that renders the scene to G-buffer. Must be placed between frame first and second graph tasks.
    Task* create_task();

private:
    class Task;

    Render& m_render;
    Scene& m_scene;
    MemoryResource& m_transient_memory_resource;
};

} // namespace kw
