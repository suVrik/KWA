#include <Windows.h>

#if defined(ABSOLUTE)
#undef ABSOLUTE
#endif

#if defined(RELATIVE)
#undef RELATIVE
#endif

#include <render/frame_graph.h>
#include <render/render.h>
#include <render/render_utils.h>

#include <system/event_loop.h>
#include <system/window.h>

#include <core/math.h>

#include <concurrency/thread_pool.h>

#include <memory/linear_memory_resource.h>
#include <memory/malloc_memory_resource.h>

using namespace kw;

struct VertexData {
    float4 position;
    float4 normal;
    float4 tangent;
    float2 texcoord;
};

struct JointData {
    uint8_t joints[4];
    uint8_t weights[4];
};

struct InstanceData {
    float4x4 model;
};

struct GeometryData {
    float4x4 model_view_projection;
};

struct PointLightData {
    float4x4 model_view_projection;
    float4 intensity;
};

struct TonemappingData {
    float4x4 view_projection;
};

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/) {
    //
    // System
    //

    EventLoop event_loop;

    WindowDescriptor window_descriptor{};
    window_descriptor.title = "Render Example";
    window_descriptor.width = 1280;
    window_descriptor.height = 1024;

    Window window(window_descriptor);

    MallocMemoryResource& persistent_memory_resource = MallocMemoryResource::instance();
    LinearMemoryResource transient_memory_resource(persistent_memory_resource, 0x4000000);
    
    ThreadPool thread_pool(6);

    //
    // Render
    //

    RenderDescriptor render_descriptor{};
    render_descriptor.api = RenderApi::VULKAN;
    render_descriptor.persistent_memory_resource = &persistent_memory_resource;
    render_descriptor.transient_memory_resource = &transient_memory_resource;
    render_descriptor.is_validation_enabled = true;
    render_descriptor.is_debug_names_enabled = true;
    render_descriptor.staging_buffer_size = 128 * 1024 * 1024;
    render_descriptor.transient_buffer_size = 8 * 1024 * 1024;
    render_descriptor.buffer_allocation_size = 16 * 1024 * 1024;
    render_descriptor.buffer_block_size = 4 * 1024;
    render_descriptor.texture_allocation_size = 256 * 1024 * 1024;
    render_descriptor.texture_block_size = 64 * 1024;

    std::unique_ptr<Render> render(Render::create_instance(render_descriptor));

    //
    // Attachments
    //

    AttachmentDescriptor color_attachment_descriptors[4]{};
    color_attachment_descriptors[0].name = "albedo_ao_attachment";
    color_attachment_descriptors[0].format = TextureFormat::RGBA8_UNORM;
    color_attachment_descriptors[0].load_op = LoadOp::DONT_CARE;
    color_attachment_descriptors[1].name = "normal_roughness_attachment";
    color_attachment_descriptors[1].format = TextureFormat::RGBA16_SNORM;
    color_attachment_descriptors[1].load_op = LoadOp::DONT_CARE;
    color_attachment_descriptors[2].name = "emission_metalness_attachment";
    color_attachment_descriptors[2].format = TextureFormat::RGBA8_UNORM;
    color_attachment_descriptors[2].load_op = LoadOp::DONT_CARE;
    color_attachment_descriptors[3].name = "lighting_attachment";
    color_attachment_descriptors[3].format = TextureFormat::RGBA16_FLOAT;

    AttachmentDescriptor depth_stencil_attachment_descriptors[2]{};
    depth_stencil_attachment_descriptors[0].name = "shadow_attachment";
    depth_stencil_attachment_descriptors[0].format = TextureFormat::D32_FLOAT;
    depth_stencil_attachment_descriptors[0].size_class = SizeClass::ABSOLUTE;
    depth_stencil_attachment_descriptors[0].width = 1024.f;
    depth_stencil_attachment_descriptors[0].height = 1024.f;
    depth_stencil_attachment_descriptors[0].count = 3;
    depth_stencil_attachment_descriptors[0].clear_depth = 1.f;
    depth_stencil_attachment_descriptors[1].name = "depth_attachment";
    depth_stencil_attachment_descriptors[1].format = TextureFormat::D24_UNORM_S8_UINT;
    depth_stencil_attachment_descriptors[1].clear_depth = 1.f;

    RenderPassDescriptor render_passes[4]{};

    //
    // Shared descriptors
    //

    AttributeDescriptor vertex_attribute_descriptors[4]{};
    vertex_attribute_descriptors[0].semantic = Semantic::POSITION;
    vertex_attribute_descriptors[0].semantic_index = 0;
    vertex_attribute_descriptors[0].format = TextureFormat::RGBA32_FLOAT;
    vertex_attribute_descriptors[0].offset = offsetof(VertexData, position);
    vertex_attribute_descriptors[1].semantic = Semantic::NORMAL;
    vertex_attribute_descriptors[1].semantic_index = 0;
    vertex_attribute_descriptors[1].format = TextureFormat::RGBA16_SNORM;
    vertex_attribute_descriptors[1].offset = offsetof(VertexData, normal);
    vertex_attribute_descriptors[2].semantic = Semantic::TANGENT;
    vertex_attribute_descriptors[2].semantic_index = 0;
    vertex_attribute_descriptors[2].format = TextureFormat::RGBA16_SNORM;
    vertex_attribute_descriptors[2].offset = offsetof(VertexData, tangent);
    vertex_attribute_descriptors[3].semantic = Semantic::TEXCOORD;
    vertex_attribute_descriptors[3].semantic_index = 0;
    vertex_attribute_descriptors[3].format = TextureFormat::RG32_FLOAT;
    vertex_attribute_descriptors[3].offset = offsetof(VertexData, texcoord);

    BindingDescriptor vertex_binding_descriptor{};
    vertex_binding_descriptor.attribute_descriptors = vertex_attribute_descriptors;
    vertex_binding_descriptor.attribute_descriptor_count = std::size(vertex_attribute_descriptors);
    vertex_binding_descriptor.stride = sizeof(VertexData);

    AttributeDescriptor joint_attribute_descriptors[2]{};
    joint_attribute_descriptors[0].semantic = Semantic::JOINTS;
    joint_attribute_descriptors[0].semantic_index = 0;
    joint_attribute_descriptors[0].format = TextureFormat::RGBA8_UINT;
    joint_attribute_descriptors[0].offset = offsetof(JointData, joints);
    joint_attribute_descriptors[1].semantic = Semantic::WEIGHTS;
    joint_attribute_descriptors[1].semantic_index = 0;
    joint_attribute_descriptors[1].format = TextureFormat::RGBA8_UNORM;
    joint_attribute_descriptors[1].offset = offsetof(JointData, weights);

    BindingDescriptor skinned_vertex_binding_descriptors[2]{};
    skinned_vertex_binding_descriptors[0].attribute_descriptors = vertex_attribute_descriptors;
    skinned_vertex_binding_descriptors[0].attribute_descriptor_count = std::size(vertex_attribute_descriptors);
    skinned_vertex_binding_descriptors[0].stride = sizeof(VertexData);
    skinned_vertex_binding_descriptors[1].attribute_descriptors = joint_attribute_descriptors;
    skinned_vertex_binding_descriptors[1].attribute_descriptor_count = std::size(joint_attribute_descriptors);
    skinned_vertex_binding_descriptors[1].stride = sizeof(JointData);

    AttributeDescriptor instance_attribute_descriptors[4]{};
    instance_attribute_descriptors[0].semantic = Semantic::POSITION;
    instance_attribute_descriptors[0].semantic_index = 1;
    instance_attribute_descriptors[0].format = TextureFormat::RGBA32_FLOAT;
    instance_attribute_descriptors[0].offset = offsetof(InstanceData, model) + offsetof(float4x4, _r0);
    instance_attribute_descriptors[1].semantic = Semantic::POSITION;
    instance_attribute_descriptors[1].semantic_index = 2;
    instance_attribute_descriptors[1].format = TextureFormat::RGBA32_FLOAT;
    instance_attribute_descriptors[1].offset = offsetof(InstanceData, model) + offsetof(float4x4, _r1);
    instance_attribute_descriptors[2].semantic = Semantic::POSITION;
    instance_attribute_descriptors[2].semantic_index = 3;
    instance_attribute_descriptors[2].format = TextureFormat::RGBA32_FLOAT;
    instance_attribute_descriptors[2].offset = offsetof(InstanceData, model) + offsetof(float4x4, _r2);
    instance_attribute_descriptors[3].semantic = Semantic::POSITION;
    instance_attribute_descriptors[3].semantic_index = 4;
    instance_attribute_descriptors[3].format = TextureFormat::RGBA32_FLOAT;
    instance_attribute_descriptors[3].offset = offsetof(InstanceData, model) + offsetof(float4x4, _r3);

    BindingDescriptor instance_binding_descriptor{};
    instance_binding_descriptor.attribute_descriptors = instance_attribute_descriptors;
    instance_binding_descriptor.attribute_descriptor_count = std::size(instance_attribute_descriptors);
    instance_binding_descriptor.stride = sizeof(InstanceData);

    UniformDescriptor joint_data_uniform_buffer_descriptor{};
    joint_data_uniform_buffer_descriptor.variable_name = "GeometryData";
    joint_data_uniform_buffer_descriptor.visibility = ShaderVisibility::VERTEX;

    //
    // Shadow pass
    //

    GraphicsPipelineDescriptor shadow_pass_pipeline_states[2]{};

    shadow_pass_pipeline_states[0].name = "shadow_pipeline";
    shadow_pass_pipeline_states[0].vertex_shader_filename = "resource/shaders/geometry_vertex.hlsl";
    shadow_pass_pipeline_states[0].vertex_binding_descriptors = &vertex_binding_descriptor;
    shadow_pass_pipeline_states[0].vertex_binding_descriptor_count = 1;
    shadow_pass_pipeline_states[0].instance_binding_descriptors = &instance_binding_descriptor;
    shadow_pass_pipeline_states[0].instance_binding_descriptor_count = 1;
    shadow_pass_pipeline_states[0].depth_bias_constant_factor = 0.f;
    shadow_pass_pipeline_states[0].depth_bias_clamp = 0.f;
    shadow_pass_pipeline_states[0].depth_bias_slope_factor = 0.5f;
    shadow_pass_pipeline_states[0].is_depth_test_enabled = true;
    shadow_pass_pipeline_states[0].is_depth_write_enabled = true;
    shadow_pass_pipeline_states[0].depth_compare_op = CompareOp::LESS;
    shadow_pass_pipeline_states[0].push_constants_name = "geometry_data";
    shadow_pass_pipeline_states[0].push_constants_visibility = ShaderVisibility::VERTEX;
    shadow_pass_pipeline_states[0].push_constants_size = sizeof(GeometryData);

    shadow_pass_pipeline_states[1].name = "shadow_skinned_pipeline";
    shadow_pass_pipeline_states[1].vertex_shader_filename = "resource/shaders/geometry_skinned_vertex.hlsl";
    shadow_pass_pipeline_states[1].vertex_binding_descriptors = skinned_vertex_binding_descriptors;
    shadow_pass_pipeline_states[1].vertex_binding_descriptor_count = std::size(skinned_vertex_binding_descriptors);
    shadow_pass_pipeline_states[1].depth_bias_constant_factor = 0.f;
    shadow_pass_pipeline_states[1].depth_bias_clamp = 0.f;
    shadow_pass_pipeline_states[1].depth_bias_slope_factor = 0.5f;
    shadow_pass_pipeline_states[1].is_depth_test_enabled = true;
    shadow_pass_pipeline_states[1].is_depth_write_enabled = true;
    shadow_pass_pipeline_states[1].depth_compare_op = CompareOp::LESS;
    shadow_pass_pipeline_states[1].uniform_buffer_descriptors = &joint_data_uniform_buffer_descriptor;
    shadow_pass_pipeline_states[1].uniform_buffer_descriptor_count = 1;

    render_passes[0].name = "shadow_pass";
    render_passes[0].render_pass = nullptr;
    render_passes[0].graphics_pipeline_descriptors = shadow_pass_pipeline_states;
    render_passes[0].graphics_pipeline_descriptor_count = std::size(shadow_pass_pipeline_states);
    render_passes[0].depth_stencil_attachment_name = "shadow_attachment";

    //
    // Geometry pass
    //

    UniformDescriptor geometry_texture_descriptors[3]{};
    geometry_texture_descriptors[0].variable_name = "albedo_ao_map";
    geometry_texture_descriptors[0].visibility = ShaderVisibility::FRAGMENT;
    geometry_texture_descriptors[1].variable_name = "normal_roughness_map";
    geometry_texture_descriptors[1].visibility = ShaderVisibility::FRAGMENT;
    geometry_texture_descriptors[2].variable_name = "emission_metalness_map";
    geometry_texture_descriptors[2].visibility = ShaderVisibility::FRAGMENT;

    SamplerDescriptor basic_sampler_descriptor{};
    basic_sampler_descriptor.variable_name = "basic_sampler";
    basic_sampler_descriptor.visibility = ShaderVisibility::FRAGMENT;
    basic_sampler_descriptor.max_lod = 15.f;

    GraphicsPipelineDescriptor geometry_pass_pipeline_state[2]{};

    geometry_pass_pipeline_state[0].name = "geometry_pipeline";
    geometry_pass_pipeline_state[0].vertex_shader_filename = "resource/shaders/geometry_vertex.hlsl";
    geometry_pass_pipeline_state[0].fragment_shader_filename = "resource/shaders/geometry_fragment.hlsl";
    geometry_pass_pipeline_state[0].vertex_binding_descriptors = &vertex_binding_descriptor;
    geometry_pass_pipeline_state[0].vertex_binding_descriptor_count = 1;
    geometry_pass_pipeline_state[0].instance_binding_descriptors = &instance_binding_descriptor;
    geometry_pass_pipeline_state[0].instance_binding_descriptor_count = 1;
    geometry_pass_pipeline_state[0].is_depth_test_enabled = true;
    geometry_pass_pipeline_state[0].is_depth_write_enabled = true;
    geometry_pass_pipeline_state[0].depth_compare_op = CompareOp::LESS;
    geometry_pass_pipeline_state[0].is_stencil_test_enabled = true;
    geometry_pass_pipeline_state[0].stencil_write_mask = 0xFF;
    geometry_pass_pipeline_state[0].front_stencil_op_state.pass_op = StencilOp::REPLACE;
    geometry_pass_pipeline_state[0].front_stencil_op_state.compare_op = CompareOp::ALWAYS;
    geometry_pass_pipeline_state[0].texture_descriptors = geometry_texture_descriptors;
    geometry_pass_pipeline_state[0].texture_descriptor_count = std::size(geometry_texture_descriptors);
    geometry_pass_pipeline_state[0].sampler_descriptors = &basic_sampler_descriptor;
    geometry_pass_pipeline_state[0].sampler_descriptor_count = 1;
    geometry_pass_pipeline_state[0].push_constants_name = "geometry_data";
    geometry_pass_pipeline_state[0].push_constants_visibility = ShaderVisibility::VERTEX;
    geometry_pass_pipeline_state[0].push_constants_size = sizeof(GeometryData);

    geometry_pass_pipeline_state[1].name = "geometry_skinned_pipeline";
    geometry_pass_pipeline_state[1].vertex_shader_filename = "resource/shaders/geometry_skinned_vertex.hlsl";
    geometry_pass_pipeline_state[1].fragment_shader_filename = "resource/shaders/geometry_fragment.hlsl";
    geometry_pass_pipeline_state[1].vertex_binding_descriptors = skinned_vertex_binding_descriptors;
    geometry_pass_pipeline_state[1].vertex_binding_descriptor_count = std::size(skinned_vertex_binding_descriptors);
    geometry_pass_pipeline_state[1].is_depth_test_enabled = true;
    geometry_pass_pipeline_state[1].is_depth_write_enabled = true;
    geometry_pass_pipeline_state[1].depth_compare_op = CompareOp::LESS;
    geometry_pass_pipeline_state[1].is_stencil_test_enabled = true;
    geometry_pass_pipeline_state[1].stencil_write_mask = 0xFF;
    geometry_pass_pipeline_state[1].front_stencil_op_state.pass_op = StencilOp::REPLACE;
    geometry_pass_pipeline_state[1].front_stencil_op_state.compare_op = CompareOp::ALWAYS;
    geometry_pass_pipeline_state[1].uniform_buffer_descriptors = &joint_data_uniform_buffer_descriptor;
    geometry_pass_pipeline_state[1].uniform_buffer_descriptor_count = 1;
    geometry_pass_pipeline_state[1].texture_descriptors = geometry_texture_descriptors;
    geometry_pass_pipeline_state[1].texture_descriptor_count = std::size(geometry_texture_descriptors);
    geometry_pass_pipeline_state[1].sampler_descriptors = &basic_sampler_descriptor;
    geometry_pass_pipeline_state[1].sampler_descriptor_count = 1;

    const char* const geometry_pass_color_attachments[] = {
        "albedo_ao_attachment",
        "normal_roughness_attachment",
        "emission_metalness_attachment",
    };

    render_passes[1].name = "geometry_pass";
    render_passes[1].render_pass = nullptr;
    render_passes[1].graphics_pipeline_descriptors = geometry_pass_pipeline_state;
    render_passes[1].graphics_pipeline_descriptor_count = std::size(geometry_pass_pipeline_state);
    render_passes[1].color_attachment_names = geometry_pass_color_attachments;
    render_passes[1].color_attachment_name_count = std::size(geometry_pass_color_attachments);
    render_passes[1].depth_stencil_attachment_name = "depth_attachment";

    //
    // Lighting pass
    //

    AttributeDescriptor float4_attribute_descriptor{};
    float4_attribute_descriptor.semantic = Semantic::POSITION;
    float4_attribute_descriptor.semantic_index = 0;
    float4_attribute_descriptor.format = TextureFormat::RGBA32_FLOAT;
    float4_attribute_descriptor.offset = 0;

    BindingDescriptor float4_binding_descriptor{};
    float4_binding_descriptor.attribute_descriptors = &float4_attribute_descriptor;
    float4_binding_descriptor.attribute_descriptor_count = 1;
    float4_binding_descriptor.stride = sizeof(float4);

    AttachmentBlendDescriptor lightint_pass_blend_descriptor{};
    lightint_pass_blend_descriptor.attachment_name = "lighting_attachment";
    lightint_pass_blend_descriptor.source_color_blend_factor = BlendFactor::ONE;
    lightint_pass_blend_descriptor.destination_color_blend_factor = BlendFactor::ONE;
    lightint_pass_blend_descriptor.color_blend_op = BlendOp::ADD;
    lightint_pass_blend_descriptor.source_alpha_blend_factor = BlendFactor::ONE;
    lightint_pass_blend_descriptor.destination_alpha_blend_factor = BlendFactor::ONE;
    lightint_pass_blend_descriptor.alpha_blend_op = BlendOp::MAX;

    UniformAttachmentDescriptor lighting_uniform_attachment_descriptors[5]{};
    lighting_uniform_attachment_descriptors[0].variable_name = "shadow_map";
    lighting_uniform_attachment_descriptors[0].attachment_name = "shadow_attachment";
    lighting_uniform_attachment_descriptors[0].visibility = ShaderVisibility::FRAGMENT;
    lighting_uniform_attachment_descriptors[1].variable_name = "albedo_ao_map";
    lighting_uniform_attachment_descriptors[1].attachment_name = "albedo_ao_attachment";
    lighting_uniform_attachment_descriptors[1].visibility = ShaderVisibility::FRAGMENT;
    lighting_uniform_attachment_descriptors[2].variable_name = "normal_roughness_map";
    lighting_uniform_attachment_descriptors[2].attachment_name = "normal_roughness_attachment";
    lighting_uniform_attachment_descriptors[2].visibility = ShaderVisibility::FRAGMENT;
    lighting_uniform_attachment_descriptors[3].variable_name = "emission_metalness_map";
    lighting_uniform_attachment_descriptors[3].attachment_name = "emission_metalness_attachment";
    lighting_uniform_attachment_descriptors[3].visibility = ShaderVisibility::FRAGMENT;
    lighting_uniform_attachment_descriptors[4].variable_name = "depth_map";
    lighting_uniform_attachment_descriptors[4].attachment_name = "depth_attachment";
    lighting_uniform_attachment_descriptors[4].visibility = ShaderVisibility::FRAGMENT;

    SamplerDescriptor lighting_sampler_descriptors[2]{};
    lighting_sampler_descriptors[0].variable_name = "basic_sampler";
    lighting_sampler_descriptors[0].visibility = ShaderVisibility::FRAGMENT;
    lighting_sampler_descriptors[0].max_lod = 15.f;
    lighting_sampler_descriptors[1].variable_name = "shadow_sampler";
    lighting_sampler_descriptors[1].visibility = ShaderVisibility::FRAGMENT;
    lighting_sampler_descriptors[1].compare_enable = true;
    lighting_sampler_descriptors[1].compare_op = CompareOp::LESS;
    lighting_sampler_descriptors[1].max_lod = 15.f;

    GraphicsPipelineDescriptor lighting_pass_pipeline_states[2]{};

    lighting_pass_pipeline_states[0].name = "point_light_pipeline";
    lighting_pass_pipeline_states[0].vertex_shader_filename = "resource/shaders/point_light_vertex.hlsl";
    lighting_pass_pipeline_states[0].fragment_shader_filename = "resource/shaders/point_light_fragment.hlsl";
    lighting_pass_pipeline_states[0].vertex_binding_descriptors = &float4_binding_descriptor;
    lighting_pass_pipeline_states[0].vertex_binding_descriptor_count = 1;
    lighting_pass_pipeline_states[0].is_depth_test_enabled = true;
    lighting_pass_pipeline_states[0].depth_compare_op = CompareOp::LESS;
    lighting_pass_pipeline_states[0].is_stencil_test_enabled = true;
    lighting_pass_pipeline_states[0].stencil_compare_mask = 0xFF;
    lighting_pass_pipeline_states[0].front_stencil_op_state.compare_op = CompareOp::EQUAL;
    lighting_pass_pipeline_states[0].attachment_blend_descriptors = &lightint_pass_blend_descriptor;
    lighting_pass_pipeline_states[0].attachment_blend_descriptor_count = 1;
    lighting_pass_pipeline_states[0].uniform_attachment_descriptors = lighting_uniform_attachment_descriptors;
    lighting_pass_pipeline_states[0].uniform_attachment_descriptor_count = std::size(lighting_uniform_attachment_descriptors);
    lighting_pass_pipeline_states[0].sampler_descriptors = lighting_sampler_descriptors;
    lighting_pass_pipeline_states[0].sampler_descriptor_count = std::size(lighting_sampler_descriptors);
    lighting_pass_pipeline_states[0].push_constants_name = "point_light_data";
    lighting_pass_pipeline_states[0].push_constants_size = sizeof(PointLightData);

    lighting_pass_pipeline_states[1].name = "point_light_inside_pipeline";
    lighting_pass_pipeline_states[1].vertex_shader_filename = "resource/shaders/point_light_vertex.hlsl";
    lighting_pass_pipeline_states[1].fragment_shader_filename = "resource/shaders/point_light_fragment.hlsl";
    lighting_pass_pipeline_states[1].vertex_binding_descriptors = &float4_binding_descriptor;
    lighting_pass_pipeline_states[1].vertex_binding_descriptor_count = 1;
    lighting_pass_pipeline_states[1].cull_mode = CullMode::FRONT;
    lighting_pass_pipeline_states[1].is_depth_test_enabled = true;
    lighting_pass_pipeline_states[1].depth_compare_op = CompareOp::GREATER;
    lighting_pass_pipeline_states[1].is_stencil_test_enabled = true;
    lighting_pass_pipeline_states[1].stencil_compare_mask = 0xFF;
    lighting_pass_pipeline_states[1].back_stencil_op_state.compare_op = CompareOp::EQUAL;
    lighting_pass_pipeline_states[1].attachment_blend_descriptors = &lightint_pass_blend_descriptor;
    lighting_pass_pipeline_states[1].attachment_blend_descriptor_count = 1;
    lighting_pass_pipeline_states[1].uniform_attachment_descriptors = lighting_uniform_attachment_descriptors;
    lighting_pass_pipeline_states[1].uniform_attachment_descriptor_count = std::size(lighting_uniform_attachment_descriptors);
    lighting_pass_pipeline_states[1].sampler_descriptors = lighting_sampler_descriptors;
    lighting_pass_pipeline_states[1].sampler_descriptor_count = std::size(lighting_sampler_descriptors);
    lighting_pass_pipeline_states[1].push_constants_name = "point_light_data";
    lighting_pass_pipeline_states[1].push_constants_size = sizeof(PointLightData);

    const char* const lighting_pass_color_attachments[] = {
        "lighting_attachment",
    };

    render_passes[2].name = "lighting_pass";
    render_passes[2].render_pass = nullptr;
    render_passes[2].graphics_pipeline_descriptors = lighting_pass_pipeline_states;
    render_passes[2].graphics_pipeline_descriptor_count = std::size(lighting_pass_pipeline_states);
    render_passes[2].color_attachment_names = lighting_pass_color_attachments;
    render_passes[2].color_attachment_name_count = std::size(lighting_pass_color_attachments);
    render_passes[2].depth_stencil_attachment_name = "depth_attachment";

    //
    // Tonemapping pass
    //

    UniformAttachmentDescriptor tonemapping_uniform_attachment_descriptor{};
    tonemapping_uniform_attachment_descriptor.variable_name = "lighting_map";
    tonemapping_uniform_attachment_descriptor.attachment_name = "lighting_attachment";
    tonemapping_uniform_attachment_descriptor.visibility = ShaderVisibility::FRAGMENT;

    GraphicsPipelineDescriptor tonemapping_pass_pipeline_state{};
    tonemapping_pass_pipeline_state.name = "tonemapping_pipeline";
    tonemapping_pass_pipeline_state.vertex_shader_filename = "resource/shaders/tonemapping_vertex.hlsl";
    tonemapping_pass_pipeline_state.fragment_shader_filename = "resource/shaders/tonemapping_fragment.hlsl";
    tonemapping_pass_pipeline_state.vertex_binding_descriptors = &float4_binding_descriptor;
    tonemapping_pass_pipeline_state.vertex_binding_descriptor_count = 1;
    tonemapping_pass_pipeline_state.uniform_attachment_descriptors = &tonemapping_uniform_attachment_descriptor;
    tonemapping_pass_pipeline_state.uniform_attachment_descriptor_count = 1;
    tonemapping_pass_pipeline_state.sampler_descriptors = &basic_sampler_descriptor;
    tonemapping_pass_pipeline_state.sampler_descriptor_count = 1;
    tonemapping_pass_pipeline_state.push_constants_name = "tonemapping_data";
    tonemapping_pass_pipeline_state.push_constants_size = sizeof(TonemappingData);

    const char* const tonemapping_pass_color_attachments[] = {
        "swapchain_attachment",
    };

    render_passes[3].name = "tonemapping_pass";
    render_passes[3].render_pass = nullptr;
    render_passes[3].graphics_pipeline_descriptors = &tonemapping_pass_pipeline_state;
    render_passes[3].graphics_pipeline_descriptor_count = 1;
    render_passes[3].color_attachment_names = tonemapping_pass_color_attachments;
    render_passes[3].color_attachment_name_count = std::size(tonemapping_pass_color_attachments);

    //
    // Frame graph
    //

    FrameGraphDescriptor frame_graph_descriptor{};
    frame_graph_descriptor.render = render.get();
    frame_graph_descriptor.window = &window;
    frame_graph_descriptor.thread_pool = &thread_pool;
    frame_graph_descriptor.is_aliasing_enabled = true;
    frame_graph_descriptor.is_vsync_enabled = true;
    frame_graph_descriptor.swapchain_attachment_name = "swapchain_attachment";
    frame_graph_descriptor.color_attachment_descriptors = color_attachment_descriptors;
    frame_graph_descriptor.color_attachment_descriptor_count = std::size(color_attachment_descriptors);
    frame_graph_descriptor.depth_stencil_attachment_descriptors = depth_stencil_attachment_descriptors;
    frame_graph_descriptor.depth_stencil_attachment_descriptor_count = std::size(depth_stencil_attachment_descriptors);
    frame_graph_descriptor.render_pass_descriptors = render_passes;
    frame_graph_descriptor.render_pass_descriptor_count = std::size(render_passes);

    std::unique_ptr<FrameGraph> frame_graph(FrameGraph::create_instance(frame_graph_descriptor));

    bool is_running = true;
    while (is_running) {
        transient_memory_resource.reset();

        Event event{};
        while (event_loop.poll_event(event)) {
            switch (event.type) {
            case EventType::QUIT:
                is_running = false;
                break;
            case EventType::SIZE_CHANGED:
                frame_graph->recreate_swapchain();
                break;
            }
        }

        frame_graph->render();
    }

    return 0;
}
