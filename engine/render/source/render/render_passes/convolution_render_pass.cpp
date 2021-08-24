#include "render/render_passes/convolution_render_pass.h"

#include <core/concurrency/task.h>
#include <core/debug/assert.h>
#include <core/math/float4x4.h>

namespace kw {

struct ConvolutionPushConstants {
    float4x4 view_projection;
};

class ConvolutionRenderPass::Task : public kw::Task {
public:
    Task(ConvolutionRenderPass& render_pass, Texture* texture, const float4x4& view_projection)
        : m_render_pass(render_pass)
        , m_texture(texture)
        , m_push_constants{view_projection}
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
            draw_call_descriptor.index_count = 36;
            draw_call_descriptor.uniform_textures = &m_texture;
            draw_call_descriptor.uniform_texture_count = 1;
            draw_call_descriptor.push_constants = &m_push_constants;
            draw_call_descriptor.push_constants_size = sizeof(ConvolutionPushConstants);
            context->draw(draw_call_descriptor);
        }
    }

    const char* get_name() const override {
        return "Convolution Render Pass";
    }

private:
    ConvolutionRenderPass& m_render_pass;
    Texture* m_texture;
    ConvolutionPushConstants m_push_constants;
};

ConvolutionRenderPass::ConvolutionRenderPass(const ConvolutionRenderPassDescriptor& descriptor)
    : m_render(*descriptor.render)
    , m_side_dimension(descriptor.side_dimension)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
{
    KW_ASSERT(descriptor.render != nullptr);
    KW_ASSERT(descriptor.transient_memory_resource != nullptr);
    
    static const float3 VERTEX_DATA[] = {
        float3( 1.0f,  1.0f, -1.0f),
        float3( 1.0f, -1.0f, -1.0f),
        float3( 1.0f,  1.0f,  1.0f),
        float3( 1.0f, -1.0f,  1.0f),
        float3(-1.0f,  1.0f, -1.0f),
        float3(-1.0f, -1.0f, -1.0f),
        float3(-1.0f,  1.0f,  1.0f),
        float3(-1.0f, -1.0f,  1.0f)
    };

    m_vertex_buffer = m_render.create_vertex_buffer("convolution_cube", sizeof(VERTEX_DATA));
    m_render.upload_vertex_buffer(m_vertex_buffer, VERTEX_DATA, sizeof(VERTEX_DATA));
    
    static const uint16_t INDEX_DATA[] = {
        0, 2, 4,
        3, 7, 2,
        7, 5, 6,
        5, 7, 1,
        1, 3, 0,
        5, 1, 4,
        2, 6, 4,
        7, 6, 2,
        5, 4, 6,
        7, 3, 1,
        3, 2, 0,
        1, 0, 4,
    };

    m_index_buffer = m_render.create_index_buffer("convolution_cube", sizeof(INDEX_DATA), IndexSize::UINT16);
    m_render.upload_index_buffer(m_index_buffer, INDEX_DATA, sizeof(INDEX_DATA));
}

ConvolutionRenderPass::~ConvolutionRenderPass() {
    m_render.destroy_index_buffer(m_index_buffer);
    m_render.destroy_vertex_buffer(m_vertex_buffer);
}

void ConvolutionRenderPass::get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    AttachmentDescriptor attachment_descriptor{};
    attachment_descriptor.name = "convolution_attachment";
    attachment_descriptor.format = TextureFormat::RGBA16_FLOAT;
    attachment_descriptor.size_class = SizeClass::ABSOLUTE;
    attachment_descriptor.width = m_side_dimension;
    attachment_descriptor.height = m_side_dimension;
    attachment_descriptor.is_blit_source = true;

    attachment_descriptors.push_back(attachment_descriptor);
}

void ConvolutionRenderPass::get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // None.
}

void ConvolutionRenderPass::get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) {
    static const char* const WRITE_COLOR_ATTACHMENT_NAME = "convolution_attachment";

    RenderPassDescriptor render_pass_descriptor{};
    render_pass_descriptor.name = "convolution_render_pass";
    render_pass_descriptor.render_pass = this;
    render_pass_descriptor.write_color_attachment_names = &WRITE_COLOR_ATTACHMENT_NAME;
    render_pass_descriptor.write_color_attachment_name_count = 1;
    render_pass_descriptors.push_back(render_pass_descriptor);
}

void ConvolutionRenderPass::create_graphics_pipelines(FrameGraph& frame_graph) {
    AttributeDescriptor attribute_descriptor{};
    attribute_descriptor.semantic = Semantic::POSITION;
    attribute_descriptor.format = TextureFormat::RGB32_FLOAT;
    attribute_descriptor.offset = 0;

    BindingDescriptor binding_descriptor{};
    binding_descriptor.attribute_descriptors = &attribute_descriptor;
    binding_descriptor.attribute_descriptor_count = 1;
    binding_descriptor.stride = sizeof(float3);

    UniformTextureDescriptor uniform_texture_descriptor{};
    uniform_texture_descriptor.variable_name = "cubemap_uniform_texture";
    uniform_texture_descriptor.texture_type = TextureType::TEXTURE_CUBE;
    
    UniformSamplerDescriptor uniform_sampler_descriptor{};
    uniform_sampler_descriptor.variable_name = "sampler_uniform";
    uniform_sampler_descriptor.max_lod = 15.f;

    GraphicsPipelineDescriptor graphics_pipeline_descriptor{};
    graphics_pipeline_descriptor.graphics_pipeline_name = "convolution_graphics_pipeline";
    graphics_pipeline_descriptor.render_pass_name = "convolution_render_pass";
    graphics_pipeline_descriptor.vertex_shader_filename = "resource/shaders/convolution_vertex.hlsl";
    graphics_pipeline_descriptor.fragment_shader_filename = "resource/shaders/convolution_fragment.hlsl";
    graphics_pipeline_descriptor.vertex_binding_descriptors = &binding_descriptor;
    graphics_pipeline_descriptor.vertex_binding_descriptor_count = 1;
    graphics_pipeline_descriptor.uniform_texture_descriptors = &uniform_texture_descriptor;
    graphics_pipeline_descriptor.uniform_texture_descriptor_count = 1;
    graphics_pipeline_descriptor.uniform_sampler_descriptors = &uniform_sampler_descriptor;
    graphics_pipeline_descriptor.uniform_sampler_descriptor_count = 1;
    graphics_pipeline_descriptor.push_constants_name = "convolution_push_constants";
    graphics_pipeline_descriptor.push_constants_size = sizeof(ConvolutionPushConstants);

    m_graphics_pipeline = frame_graph.create_graphics_pipeline(graphics_pipeline_descriptor);
}

void ConvolutionRenderPass::destroy_graphics_pipelines(FrameGraph& frame_graph) {
    frame_graph.destroy_graphics_pipeline(m_graphics_pipeline);
}

Task* ConvolutionRenderPass::create_task(Texture* texture, const float4x4& view_projection) {
    return m_transient_memory_resource.construct<Task>(*this, texture, view_projection);
}

} // namespace kw
