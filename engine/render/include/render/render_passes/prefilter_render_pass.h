#pragma once

#include "render/render_passes/base_render_pass.h"

namespace kw {

class float4x4;

struct PrefilterRenderPassDescriptor {
    Render* render;
    uint32_t side_dimension;
    MemoryResource* transient_memory_resource;
};

class PrefilterRenderPass : public BaseRenderPass {
public:
    explicit PrefilterRenderPass(const PrefilterRenderPassDescriptor& descriptor);
    ~PrefilterRenderPass();

    void get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) override;
    void get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) override;
    void get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) override;
    void create_graphics_pipelines(FrameGraph& frame_graph) override;
    void destroy_graphics_pipelines(FrameGraph& frame_graph) override;

    // Must be placed between acquire and present frame graph's tasks.
    Task* create_task(Texture* texture, const float4x4& view_projection, float roughness, uint32_t scale_factor);

private:
    class Task;

    Render& m_render;
    uint32_t m_side_dimension;
    MemoryResource& m_transient_memory_resource;

    VertexBuffer* m_vertex_buffer;
    IndexBuffer* m_index_buffer;

    GraphicsPipeline* m_graphics_pipeline;
};

} // namespace kw
