#include "render/render_passes/prefilter_render_pass.h"

#include <core/concurrency/task.h>
#include <core/debug/assert.h>
#include <core/math/float4x4.h>

namespace kw {

struct alignas(16) PrefilterPushConstants {
    float4x4 view_projection;
    float roughness;
};

class PrefilterRenderPass::Task : public kw::Task {
public:
    Task(PrefilterRenderPass& render_pass, Texture* texture, const float4x4& view_projection, float roughness, uint32_t scale_factor)
        : m_render_pass(render_pass)
        , m_texture(texture)
        , m_push_constants{ view_projection, roughness }
        , m_size{ render_pass.m_side_dimension / scale_factor }
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
            draw_call_descriptor.override_scissors = true;
            draw_call_descriptor.scissors.width = m_size;
            draw_call_descriptor.scissors.height = m_size;
            draw_call_descriptor.uniform_textures = &m_texture;
            draw_call_descriptor.uniform_texture_count = 1;
            draw_call_descriptor.push_constants = &m_push_constants;
            draw_call_descriptor.push_constants_size = sizeof(PrefilterPushConstants);
            context->draw(draw_call_descriptor);
        }
    }

    const char* get_name() const override {
        return "Prefilter Render Pass";
    }

private:
    PrefilterRenderPass& m_render_pass;
    Texture* m_texture;
    PrefilterPushConstants m_push_constants;
    uint32_t m_size;
};

PrefilterRenderPass::PrefilterRenderPass(const PrefilterRenderPassDescriptor& descriptor)
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

    m_vertex_buffer = m_render.create_vertex_buffer("prefilter_cube", sizeof(VERTEX_DATA));
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

    m_index_buffer = m_render.create_index_buffer("prefilter_cube", sizeof(INDEX_DATA), IndexSize::UINT16);
    m_render.upload_index_buffer(m_index_buffer, INDEX_DATA, sizeof(INDEX_DATA));
}

PrefilterRenderPass::~PrefilterRenderPass() {
    m_render.destroy_index_buffer(m_index_buffer);
    m_render.destroy_vertex_buffer(m_vertex_buffer);
}

void PrefilterRenderPass::get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    AttachmentDescriptor attachment_descriptor{};
    attachment_descriptor.name = "prefilter_attachment";
    attachment_descriptor.format = TextureFormat::RGBA16_FLOAT;
    attachment_descriptor.size_class = SizeClass::ABSOLUTE;
    attachment_descriptor.width = m_side_dimension;
    attachment_descriptor.height = m_side_dimension;
    attachment_descriptor.is_blit_source = true;

    attachment_descriptors.push_back(attachment_descriptor);
}

void PrefilterRenderPass::get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // None.
}

void PrefilterRenderPass::get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) {
    static const char* const WRITE_COLOR_ATTACHMENT_NAME = "prefilter_attachment";

    RenderPassDescriptor render_pass_descriptor{};
    render_pass_descriptor.name = "prefilter_render_pass";
    render_pass_descriptor.render_pass = this;
    render_pass_descriptor.write_color_attachment_names = &WRITE_COLOR_ATTACHMENT_NAME;
    render_pass_descriptor.write_color_attachment_name_count = 1;
    render_pass_descriptors.push_back(render_pass_descriptor);
}

void PrefilterRenderPass::create_graphics_pipelines(FrameGraph& frame_graph) {
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
    graphics_pipeline_descriptor.graphics_pipeline_name = "prefilter_graphics_pipeline";
    graphics_pipeline_descriptor.render_pass_name = "prefilter_render_pass";
    graphics_pipeline_descriptor.vertex_shader_filename = "resource/shaders/prefilter_vertex.hlsl";
    graphics_pipeline_descriptor.fragment_shader_filename = "resource/shaders/prefilter_fragment.hlsl";
    graphics_pipeline_descriptor.vertex_binding_descriptors = &binding_descriptor;
    graphics_pipeline_descriptor.vertex_binding_descriptor_count = 1;
    graphics_pipeline_descriptor.uniform_texture_descriptors = &uniform_texture_descriptor;
    graphics_pipeline_descriptor.uniform_texture_descriptor_count = 1;
    graphics_pipeline_descriptor.uniform_sampler_descriptors = &uniform_sampler_descriptor;
    graphics_pipeline_descriptor.uniform_sampler_descriptor_count = 1;
    graphics_pipeline_descriptor.push_constants_name = "prefilter_push_constants";
    graphics_pipeline_descriptor.push_constants_size = sizeof(PrefilterPushConstants);

    m_graphics_pipeline = frame_graph.create_graphics_pipeline(graphics_pipeline_descriptor);
}

void PrefilterRenderPass::destroy_graphics_pipelines(FrameGraph& frame_graph) {
    frame_graph.destroy_graphics_pipeline(m_graphics_pipeline);
}

Task* PrefilterRenderPass::create_task(Texture* texture, const float4x4& view_projection, float roughness, uint32_t scale_factor) {
    return m_transient_memory_resource.construct<Task>(*this, texture, view_projection, roughness, scale_factor);
}

} // namespace kw
