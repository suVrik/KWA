#pragma once

#include "render/render_passes/base_render_pass.h"

namespace kw {

class Scene;
class ShadowRenderPass;

class LightingRenderPass : public BaseRenderPass {
public:
    LightingRenderPass(Render& render, Scene& scene, ShadowRenderPass& shadow_render_pass, MemoryResource& transient_memory_resource);
    ~LightingRenderPass() override;

    void get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) override;
    void get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) override;
    void get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) override;
    void create_graphics_pipelines(FrameGraph& frame_graph) override;
    void destroy_graphics_pipelines(FrameGraph& frame_graph) override;

    // Must be placed between shadow render pass's task and present frame graph's task.
    Task* create_task();

private:
    class Task;

    void create_sphere_light_buffers();
    void create_sphere_light_graphics_pipelines(FrameGraph& frame_graph);

    Render& m_render;
    Scene& m_scene;
    ShadowRenderPass& m_shadow_render_pass;
    MemoryResource& m_transient_memory_resource;

    Texture* m_pcf_rotation_texture;

    VertexBuffer* m_sphere_light_vertex_buffer;
    IndexBuffer* m_sphere_light_index_buffer;
    GraphicsPipeline* m_sphere_light_graphics_pipelines[2];
};

} // namespace kw
