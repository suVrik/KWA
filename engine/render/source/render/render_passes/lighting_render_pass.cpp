#include "render/render_passes/lighting_render_pass.h"
#include "render/light/sphere_light_primitive.h"
#include "render/render_passes/shadow_render_pass.h"
#include "render/scene/scene.h"

#include <core/concurrency/task.h>
#include <core/debug/assert.h>
#include <core/math/float4x4.h>
#include <core/memory/memory_resource.h>

namespace kw {

struct LightUniformBuffer {
    float4x4 view_projection;
    float4x4 inverse_view_projection;
    float4 view_position;
    float2 texel_size;
    float2 specular_diffuse;
};

struct PointLightPushConstants {
    float4 position;
    float4 luminance;
    float4 radius_frustum;
    float4 shadow_params;
};

// TODO: Store these in CVars.
static float specular_factor = 0.1f;
static float diffuse_factor = 1.f;

class LightingRenderPass::Task : public kw::Task {
public:
    Task(LightingRenderPass& render_pass)
        : m_render_pass(render_pass)
    {
    }

    void run() override {
        RenderPassContext* context = m_render_pass.begin();
        if (context != nullptr) {
            LightUniformBuffer light_uniform{};
            light_uniform.view_projection = m_render_pass.m_scene.get_camera().get_view_projection_matrix();
            light_uniform.inverse_view_projection = m_render_pass.m_scene.get_camera().get_inverse_view_projection_matrix();
            light_uniform.view_position = float4(m_render_pass.m_scene.get_camera().get_translation(), 1.f);
            light_uniform.texel_size = float2(1.f / context->get_attachment_width(), 1.f / context->get_attachment_height());
            light_uniform.specular_diffuse = float2(specular_factor, diffuse_factor);
            m_transient_uniform_buffer = context->get_render().acquire_transient_uniform_buffer(&light_uniform, sizeof(light_uniform));

            Vector<LightPrimitive*> primitives = m_render_pass.m_scene.query_lights(m_render_pass.m_scene.get_occlusion_camera().get_frustum());
            
            for (LightPrimitive* light_primitive : primitives) {
                if (SphereLightPrimitive* sphere_light_primitive = dynamic_cast<SphereLightPrimitive*>(light_primitive)) {
                    draw_sphere_light(context, sphere_light_primitive);
                } else {
                    KW_ASSERT(false, "Invalid light type.");
                }
            }
        }
    }

    const char* get_name() const override {
        return "Lighting Render Pass";
    }

private:
    void draw_sphere_light(RenderPassContext* context, SphereLightPrimitive* sphere_light_primitive) {
        float3 color = sphere_light_primitive->get_color();
        float power = sphere_light_primitive->get_power();
        float radius = sphere_light_primitive->get_radius();
        float3 luminance = color * power / (4.0 * radius * radius * PI * PI);
        float attenuation_radius = std::sqrt(power * 10.f);

        const SphereLightPrimitive::ShadowParams& shadow_params = sphere_light_primitive->get_shadow_params();

        PointLightPushConstants push_constants{};
        push_constants.position = float4(sphere_light_primitive->get_global_transform().translation, 1.f);
        push_constants.luminance = float4(luminance, 0.f);
        push_constants.radius_frustum = float4(radius, attenuation_radius, 0.1f, 20.f);
        push_constants.shadow_params = float4(shadow_params.normal_bias, shadow_params.perspective_bias, shadow_params.pcss_radius_factor, shadow_params.pcss_filter_factor);

        Camera& camera = m_render_pass.m_scene.get_camera();

        float ico_sphere_radius = 1.08f;
        float z_near = camera.get_z_near() / std::cos(camera.get_fov() / 2.f);

        GraphicsPipeline* graphics_pipeline;
        if (square_distance(sphere_light_primitive->get_global_transform().translation, camera.get_translation()) <= sqr(attenuation_radius * ico_sphere_radius + z_near)) {
            graphics_pipeline = m_render_pass.m_sphere_light_graphics_pipelines[1];
        } else {
            graphics_pipeline = m_render_pass.m_sphere_light_graphics_pipelines[0];
        }

        Texture* uniform_textures[2] = {
            m_render_pass.m_shadow_render_pass.get_dummy_shadow_map(),
            m_render_pass.m_pcf_rotation_texture,
        };

        for (const ShadowRenderPass::ShadowMap& shadow_map : m_render_pass.m_shadow_render_pass.get_shadow_maps()) {
            if (shadow_map.light_primitive == sphere_light_primitive) {
                uniform_textures[0] = shadow_map.texture;
                break;
            }
        }

        DrawCallDescriptor draw_call_descriptor{};
        draw_call_descriptor.graphics_pipeline = graphics_pipeline;
        draw_call_descriptor.vertex_buffers = &m_render_pass.m_sphere_light_vertex_buffer;
        draw_call_descriptor.vertex_buffer_count = 1;
        draw_call_descriptor.index_buffer = m_render_pass.m_sphere_light_index_buffer;
        draw_call_descriptor.index_count = 240;
        draw_call_descriptor.stencil_reference = 0xFF;
        draw_call_descriptor.uniform_textures = uniform_textures;
        draw_call_descriptor.uniform_texture_count = std::size(uniform_textures);
        draw_call_descriptor.uniform_buffers = &m_transient_uniform_buffer;
        draw_call_descriptor.uniform_buffer_count = 1;
        draw_call_descriptor.push_constants = &push_constants;
        draw_call_descriptor.push_constants_size = sizeof(PointLightPushConstants);
        context->draw(draw_call_descriptor);
    }

    LightingRenderPass& m_render_pass;
    UniformBuffer* m_transient_uniform_buffer;
};

LightingRenderPass::LightingRenderPass(const LightingRenderPassDescriptor& descriptor)
    : m_render(*descriptor.render)
    , m_scene(*descriptor.scene)
    , m_shadow_render_pass(*descriptor.shadow_render_pass)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
    , m_pcf_rotation_texture(nullptr)
    , m_sphere_light_vertex_buffer(nullptr)
    , m_sphere_light_index_buffer(nullptr)
    , m_sphere_light_graphics_pipelines{ nullptr, nullptr }
{
    KW_ASSERT(descriptor.render != nullptr);
    KW_ASSERT(descriptor.scene != nullptr);
    KW_ASSERT(descriptor.shadow_render_pass != nullptr);
    KW_ASSERT(descriptor.transient_memory_resource != nullptr);

    CreateTextureDescriptor create_texture_descriptor{};
    create_texture_descriptor.name = "pcf_rotation_texture";
    create_texture_descriptor.type = TextureType::TEXTURE_3D;
    create_texture_descriptor.format = TextureFormat::RG32_FLOAT;
    create_texture_descriptor.width = 32;
    create_texture_descriptor.height = 32;
    create_texture_descriptor.depth = 32;

    m_pcf_rotation_texture = m_render.create_texture(create_texture_descriptor);

    float2 data[32][32][32]{};
    for (size_t i = 0; i < 32; i++) {
        for (size_t j = 0; j < 32; j++) {
            for (size_t k = 0; k < 32; k++) {
                float angle = std::rand() * 2.f * PI / RAND_MAX;
                data[i][j][k].x = std::cos(angle);
                data[i][j][k].y = std::sin(angle);
            }
        }
    }

    UploadTextureDescriptor upload_texture_descriptor{};
    upload_texture_descriptor.texture = m_pcf_rotation_texture;
    upload_texture_descriptor.data = data;
    upload_texture_descriptor.size = sizeof(data);
    upload_texture_descriptor.width = 32;
    upload_texture_descriptor.height = 32;
    upload_texture_descriptor.depth = 32;

    m_render.upload_texture(upload_texture_descriptor);

    create_sphere_light_buffers();
}

LightingRenderPass::~LightingRenderPass() {
    m_render.destroy_index_buffer(m_sphere_light_index_buffer);
    m_render.destroy_vertex_buffer(m_sphere_light_vertex_buffer);
    m_render.destroy_texture(m_pcf_rotation_texture);
}

void LightingRenderPass::get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    attachment_descriptors.push_back(AttachmentDescriptor{ "lighting_attachment", TextureFormat::RGBA16_FLOAT });
}

void LightingRenderPass::get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // None.
}

void LightingRenderPass::get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) {
    static const char* const READ_COLOR_ATTACHMENT_NAMES[] = {
        "albedo_ao_attachment",
        "normal_roughness_attachment",
        "emission_metalness_attachment",
        "depth_attachment",
    };

    static const char* const WRITE_COLOR_ATTACHMENT_NAMES[] = {
        "lighting_attachment",
    };

    RenderPassDescriptor render_pass_descriptor{};
    render_pass_descriptor.name = "lighting_render_pass";
    render_pass_descriptor.render_pass = this;
    render_pass_descriptor.read_attachment_names = READ_COLOR_ATTACHMENT_NAMES;
    render_pass_descriptor.read_attachment_name_count = std::size(READ_COLOR_ATTACHMENT_NAMES);
    render_pass_descriptor.write_color_attachment_names = WRITE_COLOR_ATTACHMENT_NAMES;
    render_pass_descriptor.write_color_attachment_name_count = std::size(WRITE_COLOR_ATTACHMENT_NAMES);
    render_pass_descriptor.read_depth_stencil_attachment_name = "depth_attachment";
    render_pass_descriptors.push_back(render_pass_descriptor);
}

void LightingRenderPass::create_graphics_pipelines(FrameGraph& frame_graph) {
    create_sphere_light_graphics_pipelines(frame_graph);
}

void LightingRenderPass::destroy_graphics_pipelines(FrameGraph& frame_graph) {
    frame_graph.destroy_graphics_pipeline(m_sphere_light_graphics_pipelines[1]);
    frame_graph.destroy_graphics_pipeline(m_sphere_light_graphics_pipelines[0]);
}

Task* LightingRenderPass::create_task() {
    return m_transient_memory_resource.construct<Task>(*this);
}

void LightingRenderPass::create_sphere_light_buffers() {
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

    m_sphere_light_vertex_buffer = m_render.create_vertex_buffer("sphere_light", sizeof(VERTEX_DATA));
    m_render.upload_vertex_buffer(m_sphere_light_vertex_buffer, VERTEX_DATA, sizeof(VERTEX_DATA));

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

    m_sphere_light_index_buffer = m_render.create_index_buffer("sphere_light", sizeof(INDEX_DATA), IndexSize::UINT16);
    m_render.upload_index_buffer(m_sphere_light_index_buffer, INDEX_DATA, sizeof(INDEX_DATA));
}

void LightingRenderPass::create_sphere_light_graphics_pipelines(FrameGraph& frame_graph) {
    AttributeDescriptor vertex_attribute_descriptor{};
    vertex_attribute_descriptor.semantic = Semantic::POSITION;
    vertex_attribute_descriptor.format = TextureFormat::RGB32_FLOAT;
    vertex_attribute_descriptor.offset = 0;

    BindingDescriptor vertex_binding_descriptor{};
    vertex_binding_descriptor.attribute_descriptors = &vertex_attribute_descriptor;
    vertex_binding_descriptor.attribute_descriptor_count = 1;
    vertex_binding_descriptor.stride = sizeof(float3);
    
    AttachmentBlendDescriptor attachment_blend_descriptor{};
    attachment_blend_descriptor.attachment_name = "lighting_attachment";
    attachment_blend_descriptor.source_color_blend_factor = BlendFactor::ONE;
    attachment_blend_descriptor.destination_color_blend_factor = BlendFactor::ONE;
    attachment_blend_descriptor.color_blend_op = BlendOp::ADD;
    attachment_blend_descriptor.source_alpha_blend_factor = BlendFactor::ONE;
    attachment_blend_descriptor.destination_alpha_blend_factor = BlendFactor::ONE;
    attachment_blend_descriptor.alpha_blend_op = BlendOp::MAX;

    UniformAttachmentDescriptor uniform_attachment_descriptors[4]{};
    uniform_attachment_descriptors[0].variable_name = "albedo_ao_uniform_attachment";
    uniform_attachment_descriptors[0].attachment_name = "albedo_ao_attachment";
    uniform_attachment_descriptors[1].variable_name = "normal_roughness_uniform_attachment";
    uniform_attachment_descriptors[1].attachment_name = "normal_roughness_attachment";
    uniform_attachment_descriptors[2].variable_name = "emission_metalness_uniform_attachment";
    uniform_attachment_descriptors[2].attachment_name = "emission_metalness_attachment";
    uniform_attachment_descriptors[3].variable_name = "depth_uniform_attachment";
    uniform_attachment_descriptors[3].attachment_name = "depth_attachment";

    UniformTextureDescriptor uniform_texture_descriptors[2]{};
    uniform_texture_descriptors[0].texture_type = TextureType::TEXTURE_CUBE;
    uniform_texture_descriptors[0].variable_name = "shadow_uniform_texture";
    uniform_texture_descriptors[1].texture_type = TextureType::TEXTURE_3D;
    uniform_texture_descriptors[1].variable_name = "pcf_rotation_uniform_texture";

    UniformSamplerDescriptor uniform_sampler_descriptors[2]{};
    uniform_sampler_descriptors[0].variable_name = "sampler_uniform";
    uniform_sampler_descriptors[0].max_lod = 15.f;
    uniform_sampler_descriptors[1].variable_name = "shadow_sampler_uniform";
    uniform_sampler_descriptors[1].compare_enable = true;
    uniform_sampler_descriptors[1].compare_op = CompareOp::LESS;
    uniform_sampler_descriptors[1].max_lod = 15.f;

    UniformBufferDescriptor uniform_buffer_descriptor{};
    uniform_buffer_descriptor.variable_name = "LightUniformBuffer";
    uniform_buffer_descriptor.size = sizeof(LightUniformBuffer);

    GraphicsPipelineDescriptor outside_graphics_pipeline_descriptor{};
    outside_graphics_pipeline_descriptor.graphics_pipeline_name = "outside_sphere_light_graphics_pipeline";
    outside_graphics_pipeline_descriptor.render_pass_name = "lighting_render_pass";
    outside_graphics_pipeline_descriptor.vertex_shader_filename = "resource/shaders/sphere_light_vertex.hlsl";
    outside_graphics_pipeline_descriptor.fragment_shader_filename = "resource/shaders/sphere_light_fragment.hlsl";
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
    outside_graphics_pipeline_descriptor.uniform_sampler_descriptors = uniform_sampler_descriptors;
    outside_graphics_pipeline_descriptor.uniform_sampler_descriptor_count = std::size(uniform_sampler_descriptors);
    outside_graphics_pipeline_descriptor.uniform_buffer_descriptors = &uniform_buffer_descriptor;
    outside_graphics_pipeline_descriptor.uniform_buffer_descriptor_count = 1;
    outside_graphics_pipeline_descriptor.push_constants_name = "sphere_light_push_constants";
    outside_graphics_pipeline_descriptor.push_constants_size = sizeof(PointLightPushConstants);

    m_sphere_light_graphics_pipelines[0] = frame_graph.create_graphics_pipeline(outside_graphics_pipeline_descriptor);

    GraphicsPipelineDescriptor inside_graphics_pipeline_descriptor{};
    inside_graphics_pipeline_descriptor.graphics_pipeline_name = "inside_sphere_light_graphics_pipeline";
    inside_graphics_pipeline_descriptor.render_pass_name = "lighting_render_pass";
    inside_graphics_pipeline_descriptor.vertex_shader_filename = "resource/shaders/sphere_light_vertex.hlsl";
    inside_graphics_pipeline_descriptor.fragment_shader_filename = "resource/shaders/sphere_light_fragment.hlsl";
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
    inside_graphics_pipeline_descriptor.uniform_sampler_descriptors = uniform_sampler_descriptors;
    inside_graphics_pipeline_descriptor.uniform_sampler_descriptor_count = std::size(uniform_sampler_descriptors);
    inside_graphics_pipeline_descriptor.uniform_buffer_descriptors = &uniform_buffer_descriptor;
    inside_graphics_pipeline_descriptor.uniform_buffer_descriptor_count = 1;
    inside_graphics_pipeline_descriptor.push_constants_name = "sphere_light_push_constants";
    inside_graphics_pipeline_descriptor.push_constants_size = sizeof(PointLightPushConstants);

    m_sphere_light_graphics_pipelines[1] = frame_graph.create_graphics_pipeline(inside_graphics_pipeline_descriptor);
}

} // namespace kw
