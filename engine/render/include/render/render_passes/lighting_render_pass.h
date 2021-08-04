#pragma once

#include "render/frame_graph.h"

#include <core/containers/vector.h>

namespace kw {

class Scene;
class ShadowRenderPass;

class LightingRenderPass : public RenderPass {
public:
    LightingRenderPass(Render& render, Scene& scene, ShadowRenderPass& shadow_render_pass, MemoryResource& transient_memory_resource);
    ~LightingRenderPass() override;

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

    // Create task that performs lighting. Must be placed between frame first and second graph tasks.
    Task* create_task();

private:
    class Task;

    void create_point_light_buffers();
    void create_point_light_graphics_pipelines(FrameGraph& frame_graph);

    Render& m_render;
    Scene& m_scene;
    ShadowRenderPass& m_shadow_render_pass;
    MemoryResource& m_transient_memory_resource;

    Texture* m_pcf_rotation_texture;

    VertexBuffer* m_point_light_vertex_buffer;
    IndexBuffer* m_point_light_index_buffer;
    GraphicsPipeline* m_point_light_graphics_pipelines[2];
};

} // namespace kw
