#include "render/render_passes/reflection_probe_render_pass.h"
#include "render/camera/camera_manager.h"
#include "render/reflection_probe/reflection_probe_primitive.h"
#include "render/scene/render_scene.h"
#include "render/texture/texture_manager.h"

#include <core/concurrency/task.h>
#include <core/debug/assert.h>
#include <core/math/float4x4.h>
#include <core/memory/memory_resource.h>

namespace kw {

struct ReflectionProbeUniformBuffer {
    float4x4 view_projection;
    float4x4 inverse_view_projection;
    float4 view_position;
    float4 texel_size;
};

struct ReflectionProbePushConstants {
    float4 position;

    float4 aabbox_min;
    float4 aabbox_max;

    float4 radius_lod;
};

class ReflectionProbeRenderPass::Task : public kw::Task {
public:
    Task(ReflectionProbeRenderPass& render_pass)
        : m_render_pass(render_pass)
    {
    }

    void run() override {
        RenderPassContext* context = m_render_pass.begin();
        if (context != nullptr) {
            Camera& camera = m_render_pass.m_camera_manager.get_camera();

            ReflectionProbeUniformBuffer light_uniform{};
            light_uniform.view_projection = camera.get_view_projection_matrix();
            light_uniform.inverse_view_projection = camera.get_inverse_view_projection_matrix();
            light_uniform.view_position = float4(camera.get_translation(), 0.f);
            light_uniform.texel_size = float4(1.f / context->get_attachment_width(), 1.f / context->get_attachment_height(), 0.f, 0.f);

            UniformBuffer* transient_uniform_buffer = context->get_render().acquire_transient_uniform_buffer(&light_uniform, sizeof(light_uniform));

            Vector<ReflectionProbePrimitive*> primitives = m_render_pass.m_scene.query_reflection_probes(m_render_pass.m_camera_manager.get_occlusion_camera().get_frustum());
            
            for (ReflectionProbePrimitive* reflection_probe_primitive : primitives) {
                SharedPtr<Texture*> irradiance_map = reflection_probe_primitive->get_irradiance_map();
                SharedPtr<Texture*> prefiltered_environment_map = reflection_probe_primitive->get_prefiltered_environment_map();
                if (irradiance_map && prefiltered_environment_map ) {
                    KW_ASSERT(*irradiance_map != nullptr, "Irradiance map is not loaded.");
                    KW_ASSERT(*prefiltered_environment_map != nullptr, "Irradiance map is not loaded.");

                    float falloff_radius = reflection_probe_primitive->get_falloff_radius();
                    const aabbox& parallax_box = reflection_probe_primitive->get_parallax_box();
                    float lod_count = (*prefiltered_environment_map)->get_mip_level_count();

                    ReflectionProbePushConstants push_constants{};
                    push_constants.position = float4(reflection_probe_primitive->get_global_translation(), 0.f);
                    push_constants.aabbox_min = float4(parallax_box.center - parallax_box.extent, 0.f);
                    push_constants.aabbox_max = float4(parallax_box.center + parallax_box.extent, 0.f);
                    push_constants.radius_lod = float4(falloff_radius, lod_count, 0.f, 0.f);

                    Camera& camera = m_render_pass.m_camera_manager.get_camera();

                    float ico_sphere_radius = 1.08f;
                    float z_near = camera.get_z_near() / std::cos(camera.get_fov() / 2.f);

                    GraphicsPipeline* graphics_pipeline;
                    if (square_distance(reflection_probe_primitive->get_global_translation(), camera.get_translation()) <= sqr(falloff_radius * ico_sphere_radius + z_near)) {
                        graphics_pipeline = m_render_pass.m_graphics_pipelines[1];
                    } else {
                        graphics_pipeline = m_render_pass.m_graphics_pipelines[0];
                    }

                    Texture* uniform_textures[3] = {
                        *irradiance_map,
                        * prefiltered_environment_map,
                        *m_render_pass.m_texture,
                    };

                    DrawCallDescriptor draw_call_descriptor{};
                    draw_call_descriptor.graphics_pipeline = graphics_pipeline;
                    draw_call_descriptor.vertex_buffers = &m_render_pass.m_vertex_buffer;
                    draw_call_descriptor.vertex_buffer_count = 1;
                    draw_call_descriptor.index_buffer = m_render_pass.m_index_buffer;
                    draw_call_descriptor.index_count = 240;
                    draw_call_descriptor.stencil_reference = 0xFF;
                    draw_call_descriptor.uniform_textures = uniform_textures;
                    draw_call_descriptor.uniform_texture_count = std::size(uniform_textures);
                    draw_call_descriptor.uniform_buffers = &transient_uniform_buffer;
                    draw_call_descriptor.uniform_buffer_count = 1;
                    draw_call_descriptor.push_constants = &push_constants;
                    draw_call_descriptor.push_constants_size = sizeof(ReflectionProbePushConstants);
                    context->draw(draw_call_descriptor);
                }
            }
        }
    }

    const char* get_name() const override {
        return "Reflection Probe Render Pass";
    }

private:
    ReflectionProbeRenderPass& m_render_pass;
};

ReflectionProbeRenderPass::ReflectionProbeRenderPass(const ReflectionProbeRenderPassDescriptor& descriptor)
    : m_render(*descriptor.render)
    , m_scene(*descriptor.scene)
    , m_camera_manager(*descriptor.camera_manager)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
    , m_vertex_buffer(nullptr)
    , m_index_buffer(nullptr)
    , m_graphics_pipelines{ nullptr, nullptr }
{
    KW_ASSERT(descriptor.render != nullptr);
    KW_ASSERT(descriptor.texture_manager != nullptr);
    KW_ASSERT(descriptor.scene != nullptr);
    KW_ASSERT(descriptor.camera_manager != nullptr);
    KW_ASSERT(descriptor.transient_memory_resource != nullptr);

    // TODO: Put this in some shared class with LightingRenderPass?
    static const float3 VERTEX_DATA[] = {
        float3( 0.000000f, -1.080000f,  0.000000f), float3( 0.781496f, -0.482997f,  0.567783f),
        float3(-0.298499f, -0.482997f,  0.918701f), float3(-0.965980f, -0.482993f,  0.000000f),
        float3(-0.298499f, -0.482997f, -0.918701f), float3( 0.781496f, -0.482997f, -0.567783f),
        float3( 0.298499f,  0.482997f,  0.918701f), float3(-0.781496f,  0.482997f,  0.567783f),
        float3(-0.781496f,  0.482997f, -0.567783f), float3( 0.298499f,  0.482997f, -0.918701f),
        float3( 0.965980f,  0.482993f,  0.000000f), float3( 0.000000f,  1.080000f,  0.000000f),
        float3(-0.175452f, -0.918707f,  0.539995f), float3( 0.459348f, -0.918707f,  0.333732f),
        float3( 0.283898f, -0.567797f,  0.873733f), float3( 0.918700f, -0.567795f,  0.000000f),
        float3( 0.459348f, -0.918707f, -0.333732f), float3(-0.567788f, -0.918704f,  0.000000f),
        float3(-0.743245f, -0.567795f,  0.539997f), float3(-0.175452f, -0.918707f, -0.539995f),
        float3(-0.743245f, -0.567795f, -0.539997f), float3( 0.283898f, -0.567797f, -0.873733f),
        float3( 1.027143f,  0.000000f,  0.333734f), float3( 1.027143f,  0.000000f, -0.333734f),
        float3( 0.000000f,  0.000000f,  1.080000f), float3( 0.634808f,  0.000000f,  0.873738f),
        float3(-1.027143f,  0.000000f,  0.333734f), float3(-0.634808f,  0.000000f,  0.873738f),
        float3(-0.634808f,  0.000000f, -0.873738f), float3(-1.027143f,  0.000000f, -0.333734f),
        float3( 0.634808f,  0.000000f, -0.873738f), float3( 0.000000f,  0.000000f, -1.080000f),
        float3( 0.743245f,  0.567795f,  0.539997f), float3(-0.283898f,  0.567797f,  0.873733f),
        float3(-0.918700f,  0.567795f,  0.000000f), float3(-0.283898f,  0.567797f, -0.873733f),
        float3( 0.743245f,  0.567795f, -0.539997f), float3( 0.175452f,  0.918707f,  0.539995f),
        float3( 0.567788f,  0.918704f,  0.000000f), float3(-0.459348f,  0.918707f,  0.333732f),
        float3(-0.459348f,  0.918707f, -0.333732f), float3( 0.175452f,  0.918707f, -0.539995f),
    };

    m_vertex_buffer = m_render.create_vertex_buffer("reflection_probe", sizeof(VERTEX_DATA));
    m_render.upload_vertex_buffer(m_vertex_buffer, VERTEX_DATA, sizeof(VERTEX_DATA));

    static const uint16_t INDEX_DATA[] = {
        0,  13, 12, 1,  13, 15, 0,  12, 17, 0,  17, 19,
        0,  19, 16, 1,  15, 22, 2,  14, 24, 3,  18, 26,
        4,  20, 28, 5,  21, 30, 1,  22, 25, 2,  24, 27,
        3,  26, 29, 4,  28, 31, 5,  30, 23, 6,  32, 37,
        7,  33, 39, 8,  34, 40, 9,  35, 41, 10, 36, 38,
        38, 41, 11, 38, 36, 41, 36, 9,  41, 41, 40, 11,
        41, 35, 40, 35, 8,  40, 40, 39, 11, 40, 34, 39,
        34, 7,  39, 39, 37, 11, 39, 33, 37, 33, 6,  37,
        37, 38, 11, 37, 32, 38, 32, 10, 38, 23, 36, 10,
        23, 30, 36, 30, 9,  36, 31, 35, 9,  31, 28, 35,
        28, 8,  35, 29, 34, 8,  29, 26, 34, 26, 7,  34,
        27, 33, 7,  27, 24, 33, 24, 6,  33, 25, 32, 6,
        25, 22, 32, 22, 10, 32, 30, 31, 9,  30, 21, 31,
        21, 4,  31, 28, 29, 8,  28, 20, 29, 20, 3,  29,
        26, 27, 7,  26, 18, 27, 18, 2,  27, 24, 25, 6,
        24, 14, 25, 14, 1,  25, 22, 23, 10, 22, 15, 23,
        15, 5,  23, 16, 21, 5,  16, 19, 21, 19, 4,  21,
        19, 20, 4,  19, 17, 20, 17, 3,  20, 17, 18, 3,
        17, 12, 18, 12, 2,  18, 15, 16, 5,  15, 13, 16,
        13, 0,  16, 12, 14, 2,  12, 13, 14, 13, 1,  14,
    };

    m_index_buffer = m_render.create_index_buffer("reflection_probe", sizeof(INDEX_DATA), IndexSize::UINT16);
    m_render.upload_index_buffer(m_index_buffer, INDEX_DATA, sizeof(INDEX_DATA));

    m_texture = descriptor.texture_manager->load("resource/textures/brdf_lut.kwt");
}

ReflectionProbeRenderPass::~ReflectionProbeRenderPass() {
    m_render.destroy_index_buffer(m_index_buffer);
    m_render.destroy_vertex_buffer(m_vertex_buffer);
}

void ReflectionProbeRenderPass::get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    attachment_descriptors.push_back(AttachmentDescriptor{ "reflection_probe_attachment", TextureFormat::RGBA16_FLOAT });
}

void ReflectionProbeRenderPass::get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // None.
}

void ReflectionProbeRenderPass::get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) {
    static const char* const READ_COLOR_ATTACHMENT_NAMES[] = {
        "albedo_metalness_attachment",
        "normal_roughness_attachment",
        "depth_attachment",
    };

    static const char* WRITE_COLOR_ATTACHMENT_NAME = "reflection_probe_attachment";

    RenderPassDescriptor render_pass_descriptor{};
    render_pass_descriptor.name = "reflection_probe_render_pass";
    render_pass_descriptor.render_pass = this;
    render_pass_descriptor.read_attachment_names = READ_COLOR_ATTACHMENT_NAMES;
    render_pass_descriptor.read_attachment_name_count = std::size(READ_COLOR_ATTACHMENT_NAMES);
    render_pass_descriptor.write_color_attachment_names = &WRITE_COLOR_ATTACHMENT_NAME;
    render_pass_descriptor.write_color_attachment_name_count = 1;
    render_pass_descriptor.read_depth_stencil_attachment_name = "depth_attachment";
    render_pass_descriptors.push_back(render_pass_descriptor);
}

void ReflectionProbeRenderPass::create_graphics_pipelines(FrameGraph& frame_graph) {
    AttributeDescriptor vertex_attribute_descriptor{};
    vertex_attribute_descriptor.semantic = Semantic::POSITION;
    vertex_attribute_descriptor.format = TextureFormat::RGB32_FLOAT;
    vertex_attribute_descriptor.offset = 0;

    BindingDescriptor vertex_binding_descriptor{};
    vertex_binding_descriptor.attribute_descriptors = &vertex_attribute_descriptor;
    vertex_binding_descriptor.attribute_descriptor_count = 1;
    vertex_binding_descriptor.stride = sizeof(float3);
    
    AttachmentBlendDescriptor attachment_blend_descriptor{};
    attachment_blend_descriptor.attachment_name = "reflection_probe_attachment";
    attachment_blend_descriptor.source_color_blend_factor = BlendFactor::ONE;
    attachment_blend_descriptor.destination_color_blend_factor = BlendFactor::ONE;
    attachment_blend_descriptor.color_blend_op = BlendOp::ADD;
    attachment_blend_descriptor.source_alpha_blend_factor = BlendFactor::ONE;
    attachment_blend_descriptor.destination_alpha_blend_factor = BlendFactor::ONE;
    attachment_blend_descriptor.alpha_blend_op = BlendOp::ADD;

    UniformAttachmentDescriptor uniform_attachment_descriptors[3]{};
    uniform_attachment_descriptors[0].variable_name = "albedo_metalness_uniform_attachment";
    uniform_attachment_descriptors[0].attachment_name = "albedo_metalness_attachment";
    uniform_attachment_descriptors[1].variable_name = "normal_roughness_uniform_attachment";
    uniform_attachment_descriptors[1].attachment_name = "normal_roughness_attachment";
    uniform_attachment_descriptors[2].variable_name = "depth_uniform_attachment";
    uniform_attachment_descriptors[2].attachment_name = "depth_attachment";

    UniformTextureDescriptor uniform_texture_descriptors[3]{};
    uniform_texture_descriptors[0].texture_type = TextureType::TEXTURE_CUBE;
    uniform_texture_descriptors[0].variable_name = "irradiance_uniform_texture";
    uniform_texture_descriptors[1].texture_type = TextureType::TEXTURE_CUBE;
    uniform_texture_descriptors[1].variable_name = "prefilter_uniform_texture";
    uniform_texture_descriptors[2].texture_type = TextureType::TEXTURE_2D;
    uniform_texture_descriptors[2].variable_name = "brdf_lookup_uniform_texture";

    UniformSamplerDescriptor uniform_sampler_descriptor{};
    uniform_sampler_descriptor.variable_name = "sampler_uniform";
    uniform_sampler_descriptor.max_lod = 15.f;

    UniformBufferDescriptor uniform_buffer_descriptor{};
    uniform_buffer_descriptor.variable_name = "ReflectionProbeUniformBuffer";
    uniform_buffer_descriptor.size = sizeof(ReflectionProbeUniformBuffer);

    GraphicsPipelineDescriptor outside_graphics_pipeline_descriptor{};
    outside_graphics_pipeline_descriptor.graphics_pipeline_name = "outside_reflection_probe_graphics_pipeline";
    outside_graphics_pipeline_descriptor.render_pass_name = "reflection_probe_render_pass";
    outside_graphics_pipeline_descriptor.vertex_shader_filename = "resource/shaders/reflection_probe_vertex.hlsl";
    outside_graphics_pipeline_descriptor.fragment_shader_filename = "resource/shaders/reflection_probe_fragment.hlsl";
    outside_graphics_pipeline_descriptor.vertex_binding_descriptors = &vertex_binding_descriptor;
    outside_graphics_pipeline_descriptor.vertex_binding_descriptor_count = 1;
    outside_graphics_pipeline_descriptor.cull_mode = CullMode::BACK;
    outside_graphics_pipeline_descriptor.is_depth_test_enabled = true;
    outside_graphics_pipeline_descriptor.depth_compare_op = CompareOp::LESS;
    outside_graphics_pipeline_descriptor.is_stencil_test_enabled = true;
    outside_graphics_pipeline_descriptor.stencil_compare_mask = 0xFF;
    outside_graphics_pipeline_descriptor.front_stencil_op_state.compare_op = CompareOp::EQUAL;
    outside_graphics_pipeline_descriptor.attachment_blend_descriptors = &attachment_blend_descriptor;
    outside_graphics_pipeline_descriptor.attachment_blend_descriptor_count = 1;
    outside_graphics_pipeline_descriptor.uniform_attachment_descriptors = uniform_attachment_descriptors;
    outside_graphics_pipeline_descriptor.uniform_attachment_descriptor_count = std::size(uniform_attachment_descriptors);
    outside_graphics_pipeline_descriptor.uniform_texture_descriptors = uniform_texture_descriptors;
    outside_graphics_pipeline_descriptor.uniform_texture_descriptor_count = std::size(uniform_texture_descriptors);
    outside_graphics_pipeline_descriptor.uniform_sampler_descriptors = &uniform_sampler_descriptor;
    outside_graphics_pipeline_descriptor.uniform_sampler_descriptor_count = 1;
    outside_graphics_pipeline_descriptor.uniform_buffer_descriptors = &uniform_buffer_descriptor;
    outside_graphics_pipeline_descriptor.uniform_buffer_descriptor_count = 1;
    outside_graphics_pipeline_descriptor.push_constants_name = "reflection_probe_push_constants";
    outside_graphics_pipeline_descriptor.push_constants_size = sizeof(ReflectionProbePushConstants);

    m_graphics_pipelines[0] = frame_graph.create_graphics_pipeline(outside_graphics_pipeline_descriptor);

    GraphicsPipelineDescriptor inside_graphics_pipeline_descriptor{};
    inside_graphics_pipeline_descriptor.graphics_pipeline_name = "inside_reflection_probe_graphics_pipeline";
    inside_graphics_pipeline_descriptor.render_pass_name = "reflection_probe_render_pass";
    inside_graphics_pipeline_descriptor.vertex_shader_filename = "resource/shaders/reflection_probe_vertex.hlsl";
    inside_graphics_pipeline_descriptor.fragment_shader_filename = "resource/shaders/reflection_probe_fragment.hlsl";
    inside_graphics_pipeline_descriptor.vertex_binding_descriptors = &vertex_binding_descriptor;
    inside_graphics_pipeline_descriptor.vertex_binding_descriptor_count = 1;
    inside_graphics_pipeline_descriptor.cull_mode = CullMode::FRONT;
    inside_graphics_pipeline_descriptor.is_depth_test_enabled = true;
    inside_graphics_pipeline_descriptor.depth_compare_op = CompareOp::GREATER;
    inside_graphics_pipeline_descriptor.is_stencil_test_enabled = true;
    inside_graphics_pipeline_descriptor.stencil_compare_mask = 0xFF;
    inside_graphics_pipeline_descriptor.back_stencil_op_state.compare_op = CompareOp::EQUAL;
    inside_graphics_pipeline_descriptor.attachment_blend_descriptors = &attachment_blend_descriptor;
    inside_graphics_pipeline_descriptor.attachment_blend_descriptor_count = 1;
    inside_graphics_pipeline_descriptor.uniform_attachment_descriptors = uniform_attachment_descriptors;
    inside_graphics_pipeline_descriptor.uniform_attachment_descriptor_count = std::size(uniform_attachment_descriptors);
    inside_graphics_pipeline_descriptor.uniform_texture_descriptors = uniform_texture_descriptors;
    inside_graphics_pipeline_descriptor.uniform_texture_descriptor_count = std::size(uniform_texture_descriptors);
    inside_graphics_pipeline_descriptor.uniform_sampler_descriptors = &uniform_sampler_descriptor;
    inside_graphics_pipeline_descriptor.uniform_sampler_descriptor_count = 1;
    inside_graphics_pipeline_descriptor.uniform_buffer_descriptors = &uniform_buffer_descriptor;
    inside_graphics_pipeline_descriptor.uniform_buffer_descriptor_count = 1;
    inside_graphics_pipeline_descriptor.push_constants_name = "reflection_probe_push_constants";
    inside_graphics_pipeline_descriptor.push_constants_size = sizeof(ReflectionProbePushConstants);

    m_graphics_pipelines[1] = frame_graph.create_graphics_pipeline(inside_graphics_pipeline_descriptor);
}

void ReflectionProbeRenderPass::destroy_graphics_pipelines(FrameGraph& frame_graph) {
    frame_graph.destroy_graphics_pipeline(m_graphics_pipelines[1]);
    frame_graph.destroy_graphics_pipeline(m_graphics_pipelines[0]);
}

Task* ReflectionProbeRenderPass::create_task() {
    return m_transient_memory_resource.construct<Task>(*this);
}

} // namespace kw
