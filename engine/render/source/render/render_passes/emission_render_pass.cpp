#include "render/render_passes/emission_render_pass.h"

#include <core/concurrency/task.h>
#include <core/debug/assert.h>

namespace kw {

class EmissionRenderPass::Task : public kw::Task {
public:
    Task(EmissionRenderPass& render_pass)
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

    const char* get_name() const override {
        return "Emission Render Pass";
    }

private:
    EmissionRenderPass& m_render_pass;
};

EmissionRenderPass::EmissionRenderPass(const EmissionRenderPassDescriptor& descriptor)
    : FullScreenQuadRenderPass(*descriptor.render)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
{
    KW_ASSERT(descriptor.render != nullptr);
    KW_ASSERT(descriptor.transient_memory_resource != nullptr);
}

void EmissionRenderPass::get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // None.
}

void EmissionRenderPass::get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // None.
}

void EmissionRenderPass::get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) {
    static const char* const READ_COLOR_ATTACHMENT_NAMES[] = {
        "emission_ao_attachment",
        "reflection_probe_attachment",
    };

    static const char* const WRITE_COLOR_ATTACHMENT_NAME = "lighting_attachment";

    RenderPassDescriptor render_pass_descriptor{};
    render_pass_descriptor.name = "emission_render_pass";
    render_pass_descriptor.render_pass = this;
    render_pass_descriptor.read_attachment_names = READ_COLOR_ATTACHMENT_NAMES;
    render_pass_descriptor.read_attachment_name_count = std::size(READ_COLOR_ATTACHMENT_NAMES);
    render_pass_descriptor.write_color_attachment_names = &WRITE_COLOR_ATTACHMENT_NAME;
    render_pass_descriptor.write_color_attachment_name_count = 1;
    render_pass_descriptors.push_back(render_pass_descriptor);
}

void EmissionRenderPass::create_graphics_pipelines(FrameGraph& frame_graph) {
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

    AttachmentBlendDescriptor attachment_blend_descriptor{};
    attachment_blend_descriptor.attachment_name = "lighting_attachment";
    attachment_blend_descriptor.source_color_blend_factor = BlendFactor::ONE;
    attachment_blend_descriptor.destination_color_blend_factor = BlendFactor::ONE;
    attachment_blend_descriptor.color_blend_op = BlendOp::ADD;
    attachment_blend_descriptor.source_alpha_blend_factor = BlendFactor::ONE;
    attachment_blend_descriptor.destination_alpha_blend_factor = BlendFactor::ONE;
    attachment_blend_descriptor.alpha_blend_op = BlendOp::MAX;

    UniformAttachmentDescriptor uniform_attachment_descriptors[2]{};
    uniform_attachment_descriptors[0].variable_name = "emission_ao_uniform_attachment";
    uniform_attachment_descriptors[0].attachment_name = "emission_ao_attachment";
    uniform_attachment_descriptors[1].variable_name = "reflection_probe_uniform_attachment";
    uniform_attachment_descriptors[1].attachment_name = "reflection_probe_attachment";

    UniformSamplerDescriptor uniform_sampler_descriptor{};
    uniform_sampler_descriptor.variable_name = "sampler_uniform";
    uniform_sampler_descriptor.max_lod = 15.f;

    GraphicsPipelineDescriptor graphics_pipeline_descriptor{};
    graphics_pipeline_descriptor.graphics_pipeline_name = "emission_graphics_pipeline";
    graphics_pipeline_descriptor.render_pass_name = "emission_render_pass";
    graphics_pipeline_descriptor.vertex_shader_filename = "resource/shaders/emission_vertex.hlsl";
    graphics_pipeline_descriptor.fragment_shader_filename = "resource/shaders/emission_fragment.hlsl";
    graphics_pipeline_descriptor.vertex_binding_descriptors = &binding_descriptor;
    graphics_pipeline_descriptor.vertex_binding_descriptor_count = 1;
    graphics_pipeline_descriptor.attachment_blend_descriptors = &attachment_blend_descriptor;
    graphics_pipeline_descriptor.attachment_blend_descriptor_count = 1;
    graphics_pipeline_descriptor.uniform_attachment_descriptors = uniform_attachment_descriptors;
    graphics_pipeline_descriptor.uniform_attachment_descriptor_count = std::size(uniform_attachment_descriptors);
    graphics_pipeline_descriptor.uniform_sampler_descriptors = &uniform_sampler_descriptor;
    graphics_pipeline_descriptor.uniform_sampler_descriptor_count = 1;

    m_graphics_pipeline = frame_graph.create_graphics_pipeline(graphics_pipeline_descriptor);
}

void EmissionRenderPass::destroy_graphics_pipelines(FrameGraph& frame_graph) {
    frame_graph.destroy_graphics_pipeline(m_graphics_pipeline);
}

Task* EmissionRenderPass::create_task() {
    return m_transient_memory_resource.construct<Task>(*this);
}

} // namespace kw
