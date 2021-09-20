#pragma once

#include "render/render_passes/base_render_pass.h"

namespace kw {

class CameraManager;
class RenderScene;

struct ParticleSystemRenderPassDescriptor {
    RenderScene* scene;
    CameraManager* camera_manager;
    MemoryResource* transient_memory_resource;
};

class ParticleSystemRenderPass : public BaseRenderPass {
public:
    explicit ParticleSystemRenderPass(const ParticleSystemRenderPassDescriptor& descriptor);

    void get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) override;
    void get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) override;
    void get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) override;
    void create_graphics_pipelines(FrameGraph& frame_graph) override;
    void destroy_graphics_pipelines(FrameGraph& frame_graph) override;

    // Must be placed between acquire and present frame graph's tasks.
    Task* create_task();

private:
    class Task;

    RenderScene& m_scene;
    CameraManager& m_camera_manager;
    MemoryResource& m_transient_memory_resource;
};

} // namespace kw
