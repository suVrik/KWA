#include "render/render_passes/tonemapping_render_pass.h"

#include <core/concurrency/task.h>

namespace kw {

class TonemappingRenderPass::Task : public kw::Task {
public:
    Task(TonemappingRenderPass& render_pass)
        : m_render_pass(render_pass)
    {
    }

    void run() override {
        RenderPassContext* context = m_render_pass.begin();
        if (context != nullptr) {
            DrawCallDescriptor draw_call_descriptor{};
            draw_call_descriptor.graphics_pipeline = m_render_pass.m_graphics_pipeline;
            draw_call_descriptor.vertex_buffers = &m_render_pass.m_vertex_buffer;
            draw_call_descriptor.vertex_buffer_count = 1;
            draw_call_descriptor.index_buffer = m_render_pass.m_index_buffer;
            draw_call_descriptor.index_count = 6;
            context->draw(draw_call_descriptor);
        }
    }

    const char* get_name() const {
        return "HDR Render Pass";
    }

private:
    TonemappingRenderPass& m_render_pass;
};

TonemappingRenderPass::TonemappingRenderPass(Render& render, MemoryResource& transient_memory_resource)
    : FullScreenQuadRenderPass(render)
    , m_transient_memory_resource(transient_memory_resource)
{
}

void TonemappingRenderPass::get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // None.
}

void TonemappingRenderPass::get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // None.
}

void TonemappingRenderPass::get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) {
    static const char* const READ_COLOR_ATTACHMENT_NAME = "lighting_attachment";
    static const char* const WRITE_COLOR_ATTACHMENT_NAME = "swapchain_attachment";

    RenderPassDescriptor render_pass_descriptor{};
    render_pass_descriptor.name = "tonemapping_render_pass";
    render_pass_descriptor.render_pass = this;
    render_pass_descriptor.read_attachment_names = &READ_COLOR_ATTACHMENT_NAME;
    render_pass_descriptor.read_attachment_name_count = 1;
    render_pass_descriptor.write_color_attachment_names = &WRITE_COLOR_ATTACHMENT_NAME;
    render_pass_descriptor.write_color_attachment_name_count = 1;
    render_pass_descriptors.push_back(render_pass_descriptor);
}

void TonemappingRenderPass::create_graphics_pipelines(FrameGraph& frame_graph) {
    AttributeDescriptor attribute_descriptors[2]{};
    attribute_descriptors[0].semantic = Semantic::POSITION;
    attribute_descriptors[0].format = TextureFormat::RG32_FLOAT;
    attribute_descriptors[0].offset = offsetof(Vertex, position);
    attribute_descriptors[1].semantic = Semantic::TEXCOORD;
    attribute_descriptors[1].format = TextureFormat::RG32_FLOAT;
    attribute_descriptors[1].offset = offsetof(Vertex, texcoord);

    BindingDescriptor binding_descriptor{};
    binding_descriptor.attribute_descriptors = attribute_descriptors;
    binding_descriptor.attribute_descriptor_count = std::size(attribute_descriptors);
    binding_descriptor.stride = sizeof(Vertex);

    UniformAttachmentDescriptor uniform_attachment_descriptor{};
    uniform_attachment_descriptor.variable_name = "lighting_uniform_attachment";
    uniform_attachment_descriptor.attachment_name = "lighting_attachment";

    UniformSamplerDescriptor uniform_sampler_descriptor{};
    uniform_sampler_descriptor.variable_name = "sampler_uniform";
    uniform_sampler_descriptor.max_lod = 15.f;

    GraphicsPipelineDescriptor graphics_pipeline_descriptor{};
    graphics_pipeline_descriptor.graphics_pipeline_name = "tonemapping_graphics_pipeline";
    graphics_pipeline_descriptor.render_pass_name = "tonemapping_render_pass";
    graphics_pipeline_descriptor.vertex_shader_filename = "resource/shaders/tonemapping_vertex.hlsl";
    graphics_pipeline_descriptor.fragment_shader_filename = "resource/shaders/tonemapping_fragment.hlsl";
    graphics_pipeline_descriptor.vertex_binding_descriptors = &binding_descriptor;
    graphics_pipeline_descriptor.vertex_binding_descriptor_count = 1;
    graphics_pipeline_descriptor.uniform_attachment_descriptors = &uniform_attachment_descriptor;
    graphics_pipeline_descriptor.uniform_attachment_descriptor_count = 1;
    graphics_pipeline_descriptor.uniform_sampler_descriptors = &uniform_sampler_descriptor;
    graphics_pipeline_descriptor.uniform_sampler_descriptor_count = 1;

    m_graphics_pipeline = frame_graph.create_graphics_pipeline(graphics_pipeline_descriptor);
}

void TonemappingRenderPass::destroy_graphics_pipelines(FrameGraph& frame_graph) {
    frame_graph.destroy_graphics_pipeline(m_graphics_pipeline);
}

Task* TonemappingRenderPass::create_task() {
    return new (m_transient_memory_resource.allocate<Task>()) Task(*this);
}

} // namespace kw
