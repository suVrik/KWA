#pragma once

#include "render/render_passes/full_screen_quad_render_pass.h"

#include <core/containers/unique_ptr.h>

namespace kw {

class DownsamplingRenderPass;
class UpsamplingRenderPass;

struct BloomRenderPassDescriptor {
    Render* render;
    
    uint32_t mip_count;
    float blur_radius;
    float transparency;

    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

class BloomRenderPass : public FullScreenQuadRenderPass {
public:
    explicit BloomRenderPass(const BloomRenderPassDescriptor& descriptor);
    ~BloomRenderPass() override;

    void get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) override;
    void get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) override;
    void get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) override;
    void create_graphics_pipelines(FrameGraph& frame_graph) override;
    void destroy_graphics_pipelines(FrameGraph& frame_graph) override;

    // All these tasks be placed between acquire and present frame graph's tasks.
    Vector<Task*> create_tasks();

private:
    class Task;

    float m_transparency;
    Vector<UniquePtr<DownsamplingRenderPass>> m_downsampling_render_passes;
    Vector<UniquePtr<UpsamplingRenderPass>> m_upsampling_render_passes;
    MemoryResource& m_transient_memory_resource;

    GraphicsPipeline* m_graphics_pipeline;
};

} // namespace kw
