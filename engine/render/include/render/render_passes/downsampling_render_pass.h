#pragma once

#include "render/render_passes/full_screen_quad_render_pass.h"

#include <core/containers/string.h>

namespace kw {

struct DownsamplingRenderPassDescriptor {
    Render* render;
    
    const char* render_pass_name;
    const char* graphics_pipeline_name;
    const char* input_attachment_name;
    const char* output_attachment_name;
    
    // Input attachment scale is meant to be twice as large.
    float output_attachment_scale;

    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

class DownsamplingRenderPass : public FullScreenQuadRenderPass {
public:
    explicit DownsamplingRenderPass(const DownsamplingRenderPassDescriptor& descriptor);

    void get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) override;
    void get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) override;
    void get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) override;
    void create_graphics_pipelines(FrameGraph& frame_graph) override;
    void destroy_graphics_pipelines(FrameGraph& frame_graph) override;

    // Must be placed between acquire and present frame graph's tasks.
    Task* create_task();

private:
    class Task;

    String m_render_pass_name;
    String m_graphics_pipeline_name;
    String m_input_attachment_name;
    const char* m_input_attachment_name_c_str;
    String m_output_attachment_name;
    const char* m_output_attachment_name_c_str;
    float m_output_attachment_scale;
    MemoryResource& m_transient_memory_resource;

    GraphicsPipeline* m_graphics_pipeline;
};

} // namespace kw
