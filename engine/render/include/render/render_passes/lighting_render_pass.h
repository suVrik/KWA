#pragma once

#include "render/render_passes/base_render_pass.h"

namespace kw {

class CameraManager;
class RenderScene;
class ShadowManager;

struct LightingRenderPassDescriptor {
    Render* render;
    RenderScene* scene;
    CameraManager* camera_manager;
    ShadowManager* shadow_manager;
    MemoryResource* transient_memory_resource;
};

class LightingRenderPass : public BaseRenderPass {
public:
    explicit LightingRenderPass(const LightingRenderPassDescriptor& descriptor);
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

    Render& m_render;
    RenderScene& m_scene;
    CameraManager& m_camera_manager;
    ShadowManager& m_shadow_manager;
    MemoryResource& m_transient_memory_resource;

    Texture* m_pcf_rotation_texture;

    VertexBuffer* m_vertex_buffer;
    IndexBuffer* m_index_buffer;
    GraphicsPipeline* m_graphics_pipelines[2];
};

} // namespace kw
