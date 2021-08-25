#include "render/render_passes/upsampling_render_pass.h"

#include <core/concurrency/task.h>
#include <core/debug/assert.h>
#include <core/math/float4.h>

namespace kw {

struct UpsamplingPushConstants {
    float4 texel_size;
};

class UpsamplingRenderPass::Task : public kw::Task {
public:
    Task(UpsamplingRenderPass& render_pass)
        : m_render_pass(render_pass)
    {
    }

    void run() override {
        RenderPassContext* context = m_render_pass.begin();
        if (context != nullptr) {
            UpsamplingPushConstants push_constants{};
            push_constants.texel_size = float4(1.f / context->get_attachment_width(), 1.f / context->get_attachment_height(), 0.f, 0.f) * m_render_pass.m_blur_radius;

            DrawCallDescriptor draw_call_descriptor{};
            draw_call_descriptor.graphics_pipeline = m_render_pass.m_graphics_pipeline;
            draw_call_descriptor.vertex_buffers = &m_render_pass.m_vertex_buffer;
            draw_call_descriptor.vertex_buffer_count = 1;
            draw_call_descriptor.index_buffer = m_render_pass.m_index_buffer;
            draw_call_descriptor.index_count = 6;
            draw_call_descriptor.push_constants = &push_constants;
            draw_call_descriptor.push_constants_size = sizeof(UpsamplingPushConstants);
            context->draw(draw_call_descriptor);
        }
    }

    const char* get_name() const override {
        return "Upsampling Render Pass";
    }

private:
    UpsamplingRenderPass& m_render_pass;
};

UpsamplingRenderPass::UpsamplingRenderPass(const UpsamplingRenderPassDescriptor& descriptor)
    : FullScreenQuadRenderPass(*descriptor.render)
    , m_blur_radius(descriptor.blur_radius)
    , m_render_pass_name(descriptor.render_pass_name, *descriptor.persistent_memory_resource)
    , m_graphics_pipeline_name(descriptor.graphics_pipeline_name, *descriptor.persistent_memory_resource)
    , m_input_attachment_name(descriptor.input_attachment_name, *descriptor.persistent_memory_resource)
    , m_input_attachment_name_c_str(m_input_attachment_name.c_str())
    , m_output_attachment_name(descriptor.output_attachment_name, *descriptor.persistent_memory_resource)
    , m_output_attachment_name_c_str(m_output_attachment_name.c_str())
    , m_output_attachment_scale(descriptor.output_attachment_scale)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
{
    KW_ASSERT(descriptor.render != nullptr);
    KW_ASSERT(descriptor.blur_radius > 0.f);
    KW_ASSERT(descriptor.render_pass_name != nullptr);
    KW_ASSERT(descriptor.graphics_pipeline_name != nullptr);
    KW_ASSERT(descriptor.input_attachment_name != nullptr);
    KW_ASSERT(descriptor.output_attachment_name != nullptr);
    KW_ASSERT(descriptor.output_attachment_scale > 0.f && descriptor.output_attachment_scale < 1.f);
    KW_ASSERT(descriptor.persistent_memory_resource != nullptr);
    KW_ASSERT(descriptor.transient_memory_resource != nullptr);
}

void UpsamplingRenderPass::get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // None.
}

void UpsamplingRenderPass::get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // None.
}

void UpsamplingRenderPass::get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) {
    RenderPassDescriptor render_pass_descriptor{};
    render_pass_descriptor.name = m_render_pass_name.c_str();
    render_pass_descriptor.render_pass = this;
    render_pass_descriptor.read_attachment_names = &m_input_attachment_name_c_str;
    render_pass_descriptor.read_attachment_name_count = 1;
    render_pass_descriptor.write_color_attachment_names = &m_output_attachment_name_c_str;
    render_pass_descriptor.write_color_attachment_name_count = 1;
    render_pass_descriptors.push_back(render_pass_descriptor);
}

void UpsamplingRenderPass::create_graphics_pipelines(FrameGraph& frame_graph) {
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
    attachment_blend_descriptor.attachment_name = m_output_attachment_name_c_str;
    attachment_blend_descriptor.source_color_blend_factor = BlendFactor::ONE;
    attachment_blend_descriptor.destination_color_blend_factor = BlendFactor::ONE;
    attachment_blend_descriptor.color_blend_op = BlendOp::ADD;
    attachment_blend_descriptor.source_alpha_blend_factor = BlendFactor::ONE;
    attachment_blend_descriptor.destination_alpha_blend_factor = BlendFactor::ONE;
    attachment_blend_descriptor.alpha_blend_op = BlendOp::MAX;

    UniformAttachmentDescriptor uniform_attachment_descriptor{};
    uniform_attachment_descriptor.variable_name = "input_uniform_attachment";
    uniform_attachment_descriptor.attachment_name = m_input_attachment_name_c_str;

    UniformSamplerDescriptor uniform_sampler_descriptor{};
    uniform_sampler_descriptor.variable_name = "sampler_uniform";
    uniform_sampler_descriptor.address_mode_u = AddressMode::CLAMP;
    uniform_sampler_descriptor.address_mode_v = AddressMode::CLAMP;
    uniform_sampler_descriptor.address_mode_w = AddressMode::CLAMP;
    uniform_sampler_descriptor.max_lod = 15.f;

    GraphicsPipelineDescriptor graphics_pipeline_descriptor{};
    graphics_pipeline_descriptor.graphics_pipeline_name = m_graphics_pipeline_name.c_str();
    graphics_pipeline_descriptor.render_pass_name = m_render_pass_name.c_str();
    graphics_pipeline_descriptor.vertex_shader_filename = "resource/shaders/full_screen_quad_vertex.hlsl";
    graphics_pipeline_descriptor.fragment_shader_filename = "resource/shaders/upsampling_fragment.hlsl";
    graphics_pipeline_descriptor.vertex_binding_descriptors = &binding_descriptor;
    graphics_pipeline_descriptor.vertex_binding_descriptor_count = 1;
    graphics_pipeline_descriptor.attachment_blend_descriptors = &attachment_blend_descriptor;
    graphics_pipeline_descriptor.attachment_blend_descriptor_count = 1;
    graphics_pipeline_descriptor.uniform_attachment_descriptors = &uniform_attachment_descriptor;
    graphics_pipeline_descriptor.uniform_attachment_descriptor_count = 1;
    graphics_pipeline_descriptor.uniform_sampler_descriptors = &uniform_sampler_descriptor;
    graphics_pipeline_descriptor.uniform_sampler_descriptor_count = 1;
    graphics_pipeline_descriptor.push_constants_name = "upsampling_push_constants";
    graphics_pipeline_descriptor.push_constants_size = sizeof(UpsamplingPushConstants);

    m_graphics_pipeline = frame_graph.create_graphics_pipeline(graphics_pipeline_descriptor);
}

void UpsamplingRenderPass::destroy_graphics_pipelines(FrameGraph& frame_graph) {
    frame_graph.destroy_graphics_pipeline(m_graphics_pipeline);
}

Task* UpsamplingRenderPass::create_task() {
    return m_transient_memory_resource.construct<Task>(*this);
}

} // namespace kw
