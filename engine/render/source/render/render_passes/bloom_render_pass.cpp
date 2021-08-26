#include "render/render_passes/bloom_render_pass.h"
#include "render/render_passes/downsampling_render_pass.h"
#include "render/render_passes/upsampling_render_pass.h"

#include <core/concurrency/task.h>
#include <core/debug/assert.h>
#include <core/math/float4.h>

namespace kw {

struct BloomPushConstants {
    float4 transparency;
};

class BloomRenderPass::Task : public kw::Task {
public:
    Task(BloomRenderPass& render_pass)
        : m_render_pass(render_pass)
    {
    }

    void run() override {
        RenderPassContext* context = m_render_pass.begin();
        if (context != nullptr) {
            BloomPushConstants push_constants{};
            push_constants.transparency = float4(m_render_pass.m_transparency);

            DrawCallDescriptor draw_call_descriptor{};
            draw_call_descriptor.graphics_pipeline = m_render_pass.m_graphics_pipeline;
            draw_call_descriptor.vertex_buffers = &m_render_pass.m_vertex_buffer;
            draw_call_descriptor.vertex_buffer_count = 1;
            draw_call_descriptor.index_buffer = m_render_pass.m_index_buffer;
            draw_call_descriptor.index_count = 6;
            draw_call_descriptor.push_constants = &push_constants;
            draw_call_descriptor.push_constants_size = sizeof(BloomPushConstants);
            context->draw(draw_call_descriptor);
        }
    }

    const char* get_name() const override {
        return "Bloom Render Pass";
    }

private:
    BloomRenderPass& m_render_pass;
};

BloomRenderPass::BloomRenderPass(const BloomRenderPassDescriptor& descriptor)
    : FullScreenQuadRenderPass(*descriptor.render)
    , m_transparency(descriptor.transparency)
    , m_downsampling_render_passes(*descriptor.persistent_memory_resource)
    , m_upsampling_render_passes(*descriptor.persistent_memory_resource)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
{
    KW_ASSERT(descriptor.render != nullptr);
    KW_ASSERT(descriptor.mip_count > 0);
    KW_ASSERT(descriptor.persistent_memory_resource != nullptr);
    KW_ASSERT(descriptor.transient_memory_resource != nullptr);

    String input_attachment_name("lighting_attachment", m_transient_memory_resource);

    for (uint32_t i = 0; i < descriptor.mip_count; i++) {
        uint32_t inverse_scale = 2u << i;
        float scale = 1.f / inverse_scale;

        char render_pass_name[32];
        std::snprintf(render_pass_name, sizeof(render_pass_name), "downsampling_%u_render_pass", inverse_scale);

        char graphics_pipeline_name[40];
        std::snprintf(graphics_pipeline_name, sizeof(graphics_pipeline_name), "downsampling_%u_graphics_pipeline", inverse_scale);

        char output_attachment_name[32];
        std::snprintf(output_attachment_name, sizeof(output_attachment_name), "downsampling_%u_attachment", inverse_scale);

        DownsamplingRenderPassDescriptor downsampling_render_pass_descriptor{};
        downsampling_render_pass_descriptor.render = descriptor.render;
        downsampling_render_pass_descriptor.render_pass_name = render_pass_name;
        downsampling_render_pass_descriptor.graphics_pipeline_name = graphics_pipeline_name;
        downsampling_render_pass_descriptor.input_attachment_name = input_attachment_name.c_str();
        downsampling_render_pass_descriptor.output_attachment_name = output_attachment_name;
        downsampling_render_pass_descriptor.output_attachment_scale = scale;
        downsampling_render_pass_descriptor.persistent_memory_resource = descriptor.persistent_memory_resource;
        downsampling_render_pass_descriptor.transient_memory_resource = descriptor.transient_memory_resource;

        m_downsampling_render_passes.push_back(allocate_unique<DownsamplingRenderPass>(*descriptor.persistent_memory_resource, downsampling_render_pass_descriptor));

        input_attachment_name = String(output_attachment_name, m_transient_memory_resource);
    }

    for (uint32_t i = descriptor.mip_count - 1; i > 0; i--) {
        uint32_t inverse_scale = 2u << (i - 1);
        float scale = 1.f / inverse_scale;

        char render_pass_name[32];
        std::snprintf(render_pass_name, sizeof(render_pass_name), "upsampling_%u_render_pass", inverse_scale);

        char graphics_pipeline_name[40];
        std::snprintf(graphics_pipeline_name, sizeof(graphics_pipeline_name), "upsampling_%u_graphics_pipeline", inverse_scale);

        char output_attachment_name[32];
        std::snprintf(output_attachment_name, sizeof(output_attachment_name), "downsampling_%u_attachment", inverse_scale);

        UpsamplingRenderPassDescriptor upsampling_render_pass_descriptor{};
        upsampling_render_pass_descriptor.render = descriptor.render;
        upsampling_render_pass_descriptor.blur_radius = descriptor.blur_radius;
        upsampling_render_pass_descriptor.render_pass_name = render_pass_name;
        upsampling_render_pass_descriptor.graphics_pipeline_name = graphics_pipeline_name;
        upsampling_render_pass_descriptor.input_attachment_name = input_attachment_name.c_str();
        upsampling_render_pass_descriptor.output_attachment_name = output_attachment_name;
        upsampling_render_pass_descriptor.output_attachment_scale = scale;
        upsampling_render_pass_descriptor.persistent_memory_resource = descriptor.persistent_memory_resource;
        upsampling_render_pass_descriptor.transient_memory_resource = descriptor.transient_memory_resource;

        m_upsampling_render_passes.push_back(allocate_unique<UpsamplingRenderPass>(*descriptor.persistent_memory_resource, upsampling_render_pass_descriptor));

        input_attachment_name = String(output_attachment_name, m_transient_memory_resource);
    }
}

BloomRenderPass::~BloomRenderPass() = default;

void BloomRenderPass::get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    attachment_descriptors.reserve(attachment_descriptors.size() + m_downsampling_render_passes.size() + m_upsampling_render_passes.size());

    for (UniquePtr<DownsamplingRenderPass>& downsampling_render_pass : m_downsampling_render_passes) {
        downsampling_render_pass->get_color_attachment_descriptors(attachment_descriptors);
    }

    for (UniquePtr<UpsamplingRenderPass>& upsampling_render_pass : m_upsampling_render_passes) {
        upsampling_render_pass->get_color_attachment_descriptors(attachment_descriptors);
    }
}

void BloomRenderPass::get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    attachment_descriptors.reserve(attachment_descriptors.size() + m_downsampling_render_passes.size() + m_upsampling_render_passes.size());

    for (UniquePtr<DownsamplingRenderPass>& downsampling_render_pass : m_downsampling_render_passes) {
        downsampling_render_pass->get_depth_stencil_attachment_descriptors(attachment_descriptors);
    }

    for (UniquePtr<UpsamplingRenderPass>& upsampling_render_pass : m_upsampling_render_passes) {
        upsampling_render_pass->get_depth_stencil_attachment_descriptors(attachment_descriptors);
    }
}

void BloomRenderPass::get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) {
    render_pass_descriptors.reserve(render_pass_descriptors.size() + m_downsampling_render_passes.size() + m_upsampling_render_passes.size());

    for (UniquePtr<DownsamplingRenderPass>& downsampling_render_pass : m_downsampling_render_passes) {
        downsampling_render_pass->get_render_pass_descriptors(render_pass_descriptors);
    }

    for (UniquePtr<UpsamplingRenderPass>& upsampling_render_pass : m_upsampling_render_passes) {
        upsampling_render_pass->get_render_pass_descriptors(render_pass_descriptors);
    }

    static const char* const READ_COLOR_ATTACHMENT_NAME = "downsampling_2_attachment";
    static const char* const WRITE_COLOR_ATTACHMENT_NAME = "lighting_attachment";

    RenderPassDescriptor render_pass_descriptor{};
    render_pass_descriptor.name = "bloom_render_pass";
    render_pass_descriptor.render_pass = this;
    render_pass_descriptor.read_attachment_names = &READ_COLOR_ATTACHMENT_NAME;
    render_pass_descriptor.read_attachment_name_count = 1;
    render_pass_descriptor.write_color_attachment_names = &WRITE_COLOR_ATTACHMENT_NAME;
    render_pass_descriptor.write_color_attachment_name_count = 1;
    render_pass_descriptors.push_back(render_pass_descriptor);
}

void BloomRenderPass::create_graphics_pipelines(FrameGraph& frame_graph) {
    for (UniquePtr<DownsamplingRenderPass>& downsampling_render_pass : m_downsampling_render_passes) {
        downsampling_render_pass->create_graphics_pipelines(frame_graph);
    }

    for (UniquePtr<UpsamplingRenderPass>& upsampling_render_pass : m_upsampling_render_passes) {
        upsampling_render_pass->create_graphics_pipelines(frame_graph);
    }

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

    UniformAttachmentDescriptor uniform_attachment_descriptor{};
    uniform_attachment_descriptor.variable_name = "downsampling_2_uniform_attachment";
    uniform_attachment_descriptor.attachment_name = "downsampling_2_attachment";

    UniformSamplerDescriptor uniform_sampler_descriptor{};
    uniform_sampler_descriptor.variable_name = "sampler_uniform";
    uniform_sampler_descriptor.address_mode_u = AddressMode::CLAMP;
    uniform_sampler_descriptor.address_mode_v = AddressMode::CLAMP;
    uniform_sampler_descriptor.address_mode_w = AddressMode::CLAMP;
    uniform_sampler_descriptor.max_lod = 15.f;

    GraphicsPipelineDescriptor graphics_pipeline_descriptor{};
    graphics_pipeline_descriptor.graphics_pipeline_name = "bloom_graphics_pipeline";
    graphics_pipeline_descriptor.render_pass_name = "bloom_render_pass";
    graphics_pipeline_descriptor.vertex_shader_filename = "resource/shaders/full_screen_quad_vertex.hlsl";
    graphics_pipeline_descriptor.fragment_shader_filename = "resource/shaders/bloom_fragment.hlsl";
    graphics_pipeline_descriptor.vertex_binding_descriptors = &binding_descriptor;
    graphics_pipeline_descriptor.vertex_binding_descriptor_count = 1;
    graphics_pipeline_descriptor.attachment_blend_descriptors = &attachment_blend_descriptor;
    graphics_pipeline_descriptor.attachment_blend_descriptor_count = 1;
    graphics_pipeline_descriptor.uniform_attachment_descriptors = &uniform_attachment_descriptor;
    graphics_pipeline_descriptor.uniform_attachment_descriptor_count = 1;
    graphics_pipeline_descriptor.uniform_sampler_descriptors = &uniform_sampler_descriptor;
    graphics_pipeline_descriptor.uniform_sampler_descriptor_count = 1;
    graphics_pipeline_descriptor.push_constants_name = "bloom_push_constants";
    graphics_pipeline_descriptor.push_constants_size = sizeof(BloomPushConstants);

    m_graphics_pipeline = frame_graph.create_graphics_pipeline(graphics_pipeline_descriptor);
}

void BloomRenderPass::destroy_graphics_pipelines(FrameGraph& frame_graph) {
    frame_graph.destroy_graphics_pipeline(m_graphics_pipeline);

    for (UniquePtr<UpsamplingRenderPass>& upsampling_render_pass : m_upsampling_render_passes) {
        upsampling_render_pass->destroy_graphics_pipelines(frame_graph);
    }

    for (UniquePtr<DownsamplingRenderPass>& downsampling_render_pass : m_downsampling_render_passes) {
        downsampling_render_pass->destroy_graphics_pipelines(frame_graph);
    }
}

Vector<Task*> BloomRenderPass::create_tasks() {
    Vector<kw::Task*> result(m_transient_memory_resource);
    result.reserve(m_downsampling_render_passes.size() + m_upsampling_render_passes.size() + 1ull);

    for (UniquePtr<DownsamplingRenderPass>& downsampling_render_pass : m_downsampling_render_passes) {
        result.push_back(downsampling_render_pass->create_task());
    }

    for (UniquePtr<UpsamplingRenderPass>& upsampling_render_pass : m_upsampling_render_passes) {
        result.push_back(upsampling_render_pass->create_task());
    }

    result.push_back(m_transient_memory_resource.construct<Task>(*this));

    return result;
}

} // namespace kw
