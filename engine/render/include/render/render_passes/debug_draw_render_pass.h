#pragma once

#include "render/frame_graph.h"

#include <core/containers/vector.h>
#include <core/math/float4x4.h>

namespace kw {

class DebugDrawManager;
class Scene;

class DebugDrawRenderPass : public RenderPass {
public:
    DebugDrawRenderPass(Render& render, Scene& scene, DebugDrawManager& debug_draw_manager, MemoryResource& transient_memory_resource);
    
    // Fill color attachments created by this render pass.
    void get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors);

    // Fill depth stencil attachment created by this render pass.
    void get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors);

    // Fill render pass descriptors created by this render pass.
    void get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors);

    // Create graphics pipelines for this render pass.
    void create_graphics_pipelines(FrameGraph& frame_graph);

    // Create task that renders debug draw primitives. Must be placed between frame first and second graph tasks.
    Task* create_task();

private:
    class Task;

    Render& m_render;
    Scene& m_scene;
    DebugDrawManager& m_debug_draw_manager;
    MemoryResource& m_transient_memory_resource;

    GraphicsPipeline* m_graphics_pipeline;
};

} // namespace kw
