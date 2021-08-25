#include "render/render_passes/antialiasing_render_pass.h"

#include <core/concurrency/task.h>
#include <core/debug/assert.h>
#include <core/math/float4.h>

namespace kw {

struct FxaaPushConstants {
    float4 texel_size;
};

class AntialiasingRenderPass::Task : public kw::Task {
public:
    Task(AntialiasingRenderPass& render_pass)
        : m_render_pass(render_pass)
    {
    }

    void run() override {
        RenderPassContext* context = m_render_pass.begin();
        if (context != nullptr) {
            FxaaPushConstants push_constants{};
            push_constants.texel_size = float4(1.f / context->get_attachment_width(), 1.f / context->get_attachment_height(), 0.f, 0.f);

            DrawCallDescriptor draw_call_descriptor{};
            draw_call_descriptor.graphics_pipeline = m_render_pass.m_graphics_pipeline;
            draw_call_descriptor.vertex_buffers = &m_render_pass.m_vertex_buffer;
            draw_call_descriptor.vertex_buffer_count = 1;
            draw_call_descriptor.index_buffer = m_render_pass.m_index_buffer;
            draw_call_descriptor.index_count = 6;
            draw_call_descriptor.push_constants = &push_constants;
            draw_call_descriptor.push_constants_size = sizeof(FxaaPushConstants);
            context->draw(draw_call_descriptor);
        }
    }

    const char* get_name() const override {
        return "Antialiasing Render Pass";
    }

private:
    AntialiasingRenderPass& m_render_pass;
};

AntialiasingRenderPass::AntialiasingRenderPass(const AntialiasingRenderPassDescriptor& descriptor)
    : FullScreenQuadRenderPass(*descriptor.render)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
{
    KW_ASSERT(descriptor.render != nullptr);
    KW_ASSERT(descriptor.transient_memory_resource != nullptr);
}

void AntialiasingRenderPass::get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // None.
}

void AntialiasingRenderPass::get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // None.
}

void AntialiasingRenderPass::get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) {
    static const char* const READ_COLOR_ATTACHMENT_NAME = "tonemapping_attachment";
    static const char* const WRITE_COLOR_ATTACHMENT_NAME = "swapchain_attachment";

    RenderPassDescriptor render_pass_descriptor{};
    render_pass_descriptor.name = "antialiasing_render_pass";
    render_pass_descriptor.render_pass = this;
    render_pass_descriptor.read_attachment_names = &READ_COLOR_ATTACHMENT_NAME;
    render_pass_descriptor.read_attachment_name_count = 1;
    render_pass_descriptor.write_color_attachment_names = &WRITE_COLOR_ATTACHMENT_NAME;
    render_pass_descriptor.write_color_attachment_name_count = 1;
    render_pass_descriptors.push_back(render_pass_descriptor);
}

void AntialiasingRenderPass::create_graphics_pipelines(FrameGraph& frame_graph) {
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
    uniform_attachment_descriptor.variable_name = "tonemapping_uniform_attachment";
    uniform_attachment_descriptor.attachment_name = "tonemapping_attachment";

    UniformSamplerDescriptor uniform_sampler_descriptor{};
    uniform_sampler_descriptor.variable_name = "sampler_uniform";
    uniform_sampler_descriptor.address_mode_u = AddressMode::CLAMP;
    uniform_sampler_descriptor.address_mode_v = AddressMode::CLAMP;
    uniform_sampler_descriptor.address_mode_w = AddressMode::CLAMP;
    uniform_sampler_descriptor.max_lod = 15.f;

    GraphicsPipelineDescriptor graphics_pipeline_descriptor{};
    graphics_pipeline_descriptor.graphics_pipeline_name = "antialiasing_graphics_pipeline";
    graphics_pipeline_descriptor.render_pass_name = "antialiasing_render_pass";
    graphics_pipeline_descriptor.vertex_shader_filename = "resource/shaders/full_screen_quad_vertex.hlsl";
    graphics_pipeline_descriptor.fragment_shader_filename = "resource/shaders/fxaa_fragment.hlsl";
    graphics_pipeline_descriptor.vertex_binding_descriptors = &binding_descriptor;
    graphics_pipeline_descriptor.vertex_binding_descriptor_count = 1;
    graphics_pipeline_descriptor.uniform_attachment_descriptors = &uniform_attachment_descriptor;
    graphics_pipeline_descriptor.uniform_attachment_descriptor_count = 1;
    graphics_pipeline_descriptor.uniform_sampler_descriptors = &uniform_sampler_descriptor;
    graphics_pipeline_descriptor.uniform_sampler_descriptor_count = 1;
    graphics_pipeline_descriptor.push_constants_name = "fxaa_push_constants";
    graphics_pipeline_descriptor.push_constants_size = sizeof(FxaaPushConstants);

    m_graphics_pipeline = frame_graph.create_graphics_pipeline(graphics_pipeline_descriptor);
}

void AntialiasingRenderPass::destroy_graphics_pipelines(FrameGraph& frame_graph) {
    frame_graph.destroy_graphics_pipeline(m_graphics_pipeline);
}

Task* AntialiasingRenderPass::create_task() {
    return m_transient_memory_resource.construct<Task>(*this);
}

} // namespace kw
