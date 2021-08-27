#pragma once

#include "render/render_passes/base_render_pass.h"

#include <core/containers/shared_ptr.h>

namespace kw {

class CameraManager;
class Scene;
class TextureManager;

struct ReflectionProbeRenderPassDescriptor {
    Render* render;
    TextureManager* texture_manager;
    Scene* scene;
    CameraManager* camera_manager;
    MemoryResource* transient_memory_resource;
};

class ReflectionProbeRenderPass : public BaseRenderPass {
public:
    explicit ReflectionProbeRenderPass(const ReflectionProbeRenderPassDescriptor& descriptor);
    ~ReflectionProbeRenderPass() override;

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
    Scene& m_scene;
    CameraManager& m_camera_manager;
    MemoryResource& m_transient_memory_resource;

    VertexBuffer* m_vertex_buffer;
    IndexBuffer* m_index_buffer;
    SharedPtr<Texture*> m_texture;
    GraphicsPipeline* m_graphics_pipelines[2];
};

} // namespace kw
