#include "render/render_passes/lighting_render_pass.h"
#include "render/camera/camera_manager.h"
#include "render/light/point_light_primitive.h"
#include "render/scene/render_scene.h"
#include "render/shadow/shadow_manager.h"

#include <core/concurrency/task.h>
#include <core/debug/assert.h>
#include <core/math/float4x4.h>
#include <core/memory/memory_resource.h>

namespace kw {

struct LightUniformBuffer {
    float4x4 view_projection;
    float4x4 inverse_view_projection;
    float4 view_position;
    float4 texel_size;
};

struct PointLightPushConstants {
    float4 position;
    float4 luminance;
    float4 radius_frustum;
    float4 shadow_params;
};

class LightingRenderPass::Task : public kw::Task {
public:
    Task(LightingRenderPass& render_pass)
        : m_render_pass(render_pass)
    {
    }

    void run() override {
        RenderPassContext* context = m_render_pass.begin();
        if (context != nullptr) {
            Camera& camera = m_render_pass.m_camera_manager.get_camera();

            LightUniformBuffer light_uniform{};
            light_uniform.view_projection = camera.get_view_projection_matrix();
            light_uniform.inverse_view_projection = camera.get_inverse_view_projection_matrix();
            light_uniform.view_position = float4(camera.get_translation(), 0.f);
            light_uniform.texel_size = float4(1.f / context->get_attachment_width(), 1.f / context->get_attachment_height(), 0.f, 0.f);

            m_transient_uniform_buffer = context->get_render().acquire_transient_uniform_buffer(&light_uniform, sizeof(light_uniform));

            Vector<LightPrimitive*> primitives = m_render_pass.m_scene.query_lights(m_render_pass.m_camera_manager.get_occlusion_camera().get_frustum());
            
            for (LightPrimitive* light_primitive : primitives) {
                if (PointLightPrimitive* point_light_primitive = dynamic_cast<PointLightPrimitive*>(light_primitive)) {
                    draw_point_light(context, point_light_primitive);
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
    void draw_point_light(RenderPassContext* context, PointLightPrimitive* point_light_primitive) {
        float3 color = point_light_primitive->get_color();
        float power = point_light_primitive->get_power();
        float3 luminance = color * power;
        float attenuation_radius = std::sqrt(power * 50.f);

        const PointLightPrimitive::ShadowParams& shadow_params = point_light_primitive->get_shadow_params();

        PointLightPushConstants push_constants{};
        push_constants.position = float4(point_light_primitive->get_global_transform().translation, 0.f);
        push_constants.luminance = float4(luminance, 0.f);
        push_constants.radius_frustum = float4(attenuation_radius, 0.1f, 20.f, 0.f);
        push_constants.shadow_params = float4(shadow_params.normal_bias, shadow_params.perspective_bias, shadow_params.pcss_radius, shadow_params.pcss_filter_factor);

        Camera& camera = m_render_pass.m_camera_manager.get_camera();

        float ico_sphere_radius = 1.08f;
        float z_near = camera.get_z_near() / std::cos(camera.get_fov() / 2.f);

        GraphicsPipeline* graphics_pipeline;
        if (square_distance(point_light_primitive->get_global_transform().translation, camera.get_translation()) <= sqr(attenuation_radius * ico_sphere_radius + z_near)) {
            graphics_pipeline = m_render_pass.m_graphics_pipelines[1];
        } else {
            graphics_pipeline = m_render_pass.m_graphics_pipelines[0];
        }

        Texture* uniform_textures[3] = {
            m_render_pass.m_shadow_manager.get_depth_texture(point_light_primitive),
            m_render_pass.m_shadow_manager.get_color_texture(point_light_primitive),
            m_render_pass.m_pcf_rotation_texture,
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
    , m_camera_manager(*descriptor.camera_manager)
    , m_shadow_manager(*descriptor.shadow_manager)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
    , m_pcf_rotation_texture(nullptr)
    , m_vertex_buffer(nullptr)
    , m_index_buffer(nullptr)
    , m_graphics_pipelines{ nullptr, nullptr }
{
    KW_ASSERT(descriptor.render != nullptr);
    KW_ASSERT(descriptor.scene != nullptr);
    KW_ASSERT(descriptor.camera_manager != nullptr);
    KW_ASSERT(descriptor.shadow_manager != nullptr);
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

    m_vertex_buffer = m_render.create_vertex_buffer("point_light", sizeof(VERTEX_DATA));
    m_render.upload_vertex_buffer(m_vertex_buffer, VERTEX_DATA, sizeof(VERTEX_DATA));

    static const uint16_t INDEX_DATA[] = {
        12, 13, 0,  15, 13, 1,  17, 12, 0,  19, 17, 0,
        16, 19, 0,  22, 15, 1,  24, 14, 2,  26, 18, 3,
        28, 20, 4,  30, 21, 5,  25, 22, 1,  27, 24, 2,
        29, 26, 3,  31, 28, 4,  23, 30, 5,  37, 32, 6,
        39, 33, 7,  40, 34, 8,  41, 35, 9,  38, 36, 10,
        11, 41, 38, 41, 36, 38, 41, 9,  36, 11, 40, 41,
        40, 35, 41, 40, 8,  35, 11, 39, 40, 39, 34, 40,
        39, 7,  34, 11, 37, 39, 37, 33, 39, 37, 6,  33,
        11, 38, 37, 38, 32, 37, 38, 10, 32, 10, 36, 23,
        36, 30, 23, 36, 9,  30, 9,  35, 31, 35, 28, 31,
        35, 8,  28, 8,  34, 29, 34, 26, 29, 34, 7,  26,
        7,  33, 27, 33, 24, 27, 33, 6,  24, 6,  32, 25,
        32, 22, 25, 32, 10, 22, 9,  31, 30, 31, 21, 30,
        31, 4,  21, 8,  29, 28, 29, 20, 28, 29, 3,  20,
        7,  27, 26, 27, 18, 26, 27, 2,  18, 6,  25, 24,
        25, 14, 24, 25, 1,  14, 10, 23, 22, 23, 15, 22,
        23, 5,  15, 5,  21, 16, 21, 19, 16, 21, 4,  19,
        4,  20, 19, 20, 17, 19, 20, 3,  17, 3,  18, 17,
        18, 12, 17, 18, 2,  12, 5,  16, 15, 16, 13, 15,
        16, 0,  13, 2,  14, 12, 14, 13, 12, 14, 1,  13,
    };

    m_index_buffer = m_render.create_index_buffer("point_light", sizeof(INDEX_DATA), IndexSize::UINT16);
    m_render.upload_index_buffer(m_index_buffer, INDEX_DATA, sizeof(INDEX_DATA));
}

LightingRenderPass::~LightingRenderPass() {
    m_render.destroy_index_buffer(m_index_buffer);
    m_render.destroy_vertex_buffer(m_vertex_buffer);
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
        "albedo_metalness_attachment",
        "normal_roughness_attachment",
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

    UniformAttachmentDescriptor uniform_attachment_descriptors[3]{};
    uniform_attachment_descriptors[0].variable_name = "albedo_metalness_uniform_attachment";
    uniform_attachment_descriptors[0].attachment_name = "albedo_metalness_attachment";
    uniform_attachment_descriptors[1].variable_name = "normal_roughness_uniform_attachment";
    uniform_attachment_descriptors[1].attachment_name = "normal_roughness_attachment";
    uniform_attachment_descriptors[2].variable_name = "depth_uniform_attachment";
    uniform_attachment_descriptors[2].attachment_name = "depth_attachment";

    UniformTextureDescriptor uniform_texture_descriptors[3]{};
    uniform_texture_descriptors[0].texture_type = TextureType::TEXTURE_CUBE;
    uniform_texture_descriptors[0].variable_name = "opaque_shadow_uniform_texture";
    uniform_texture_descriptors[1].texture_type = TextureType::TEXTURE_CUBE;
    uniform_texture_descriptors[1].variable_name = "translucent_shadow_uniform_texture";
    uniform_texture_descriptors[2].texture_type = TextureType::TEXTURE_3D;
    uniform_texture_descriptors[2].variable_name = "pcf_rotation_uniform_texture";

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
    outside_graphics_pipeline_descriptor.graphics_pipeline_name = "outside_point_light_graphics_pipeline";
    outside_graphics_pipeline_descriptor.render_pass_name = "lighting_render_pass";
    outside_graphics_pipeline_descriptor.vertex_shader_filename = "resource/shaders/point_light_vertex.hlsl";
    outside_graphics_pipeline_descriptor.fragment_shader_filename = "resource/shaders/point_light_fragment.hlsl";
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
    outside_graphics_pipeline_descriptor.push_constants_name = "point_light_push_constants";
    outside_graphics_pipeline_descriptor.push_constants_size = sizeof(PointLightPushConstants);

    m_graphics_pipelines[0] = frame_graph.create_graphics_pipeline(outside_graphics_pipeline_descriptor);

    GraphicsPipelineDescriptor inside_graphics_pipeline_descriptor{};
    inside_graphics_pipeline_descriptor.graphics_pipeline_name = "inside_point_light_graphics_pipeline";
    inside_graphics_pipeline_descriptor.render_pass_name = "lighting_render_pass";
    inside_graphics_pipeline_descriptor.vertex_shader_filename = "resource/shaders/point_light_vertex.hlsl";
    inside_graphics_pipeline_descriptor.fragment_shader_filename = "resource/shaders/point_light_fragment.hlsl";
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
    inside_graphics_pipeline_descriptor.push_constants_name = "point_light_push_constants";
    inside_graphics_pipeline_descriptor.push_constants_size = sizeof(PointLightPushConstants);

    m_graphics_pipelines[1] = frame_graph.create_graphics_pipeline(inside_graphics_pipeline_descriptor);
}

void LightingRenderPass::destroy_graphics_pipelines(FrameGraph& frame_graph) {
    frame_graph.destroy_graphics_pipeline(m_graphics_pipelines[1]);
    frame_graph.destroy_graphics_pipeline(m_graphics_pipelines[0]);
}

Task* LightingRenderPass::create_task() {
    return m_transient_memory_resource.construct<Task>(*this);
}

} // namespace kw
