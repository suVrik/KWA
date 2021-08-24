#define NOMINMAX

#include <Windows.h>

#undef ABSOLUTE
#undef DELETE
#undef far
#undef IN
#undef near
#undef OUT
#undef RELATIVE

#include <render/acceleration_structure/linear_acceleration_structure.h>
#include <render/acceleration_structure/octree_acceleration_structure.h>
#include <render/animation/animated_geometry_primitive.h>
#include <render/animation/animation_manager.h>
#include <render/animation/animation_player.h>
#include <render/camera/camera_manager.h>
#include <render/container/container_primitive.h>
#include <render/debug/debug_draw_manager.h>
#include <render/debug/imgui_manager.h>
#include <render/frame_graph.h>
#include <render/geometry/geometry.h>
#include <render/geometry/geometry_manager.h>
#include <render/geometry/geometry_primitive.h>
#include <render/geometry/skeleton.h>
#include <render/light/point_light_primitive.h>
#include <render/material/material_manager.h>
#include <render/particles/particle_system_manager.h>
#include <render/particles/particle_system_player.h>
#include <render/particles/particle_system_primitive.h>
#include <render/reflection_probe/reflection_probe_manager.h>
#include <render/reflection_probe/reflection_probe_primitive.h>
#include <render/render.h>
#include <render/render_passes/debug_draw_render_pass.h>
#include <render/render_passes/emission_render_pass.h>
#include <render/render_passes/geometry_render_pass.h>
#include <render/render_passes/imgui_render_pass.h>
#include <render/render_passes/lighting_render_pass.h>
#include <render/render_passes/opaque_shadow_render_pass.h>
#include <render/render_passes/particle_system_render_pass.h>
#include <render/render_passes/reflection_probe_render_pass.h>
#include <render/render_passes/tonemapping_render_pass.h>
#include <render/render_passes/translucent_shadow_render_pass.h>
#include <render/scene/scene.h>
#include <render/shadow/shadow_manager.h>
#include <render/texture/texture_manager.h>

#include <system/event_loop.h>
#include <system/input.h>
#include <system/timer.h>
#include <system/window.h>

#include <core/concurrency/concurrency_utils.h>
#include <core/concurrency/task.h>
#include <core/concurrency/task_scheduler.h>
#include <core/containers/pair.h>
#include <core/containers/set.h>
#include <core/debug/assert.h>
#include <core/debug/cpu_profiler.h>
#include <core/debug/debug_utils.h>
#include <core/debug/log.h>
#include <core/error.h>
#include <core/math/float4x4.h>
#include <core/memory/malloc_memory_resource.h>
#include <core/memory/scratch_memory_resource.h>

#include <fstream>

using namespace kw;

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/) {
    DebugUtils::subscribe_to_segfault();

    ConcurrencyUtils::set_current_thread_name("Main Thread");

    MallocMemoryResource& persistent_memory_resource = MallocMemoryResource::instance();
    ScratchMemoryResource transient_memory_resource(persistent_memory_resource, 32 * 1024 * 1024);

    EventLoop event_loop;

    WindowDescriptor window_descriptor{};
    window_descriptor.title = "Render Example";
    window_descriptor.width = 1600;
    window_descriptor.height = 800;

    Window window(window_descriptor);

    Input input(window);

    Timer timer;

    TaskScheduler task_scheduler(persistent_memory_resource, 3);
    
    RenderDescriptor render_descriptor{};
    render_descriptor.api = RenderApi::VULKAN;
    render_descriptor.persistent_memory_resource = &persistent_memory_resource;
    render_descriptor.transient_memory_resource = &transient_memory_resource;
    render_descriptor.is_validation_enabled = true;
    render_descriptor.is_debug_names_enabled = true;
    render_descriptor.staging_buffer_size = 4 * 1024 * 1024;
    render_descriptor.transient_buffer_size = 4 * 1024 * 1024;
    render_descriptor.buffer_allocation_size = 1024 * 1024;
    render_descriptor.buffer_block_size = 32 * 1024;
    render_descriptor.texture_allocation_size = 128 * 1024 * 1024;
    render_descriptor.texture_block_size = 1024 * 1024;

    UniquePtr<Render> render(Render::create_instance(render_descriptor), persistent_memory_resource);

    AnimationPlayerDescriptor animation_player_descriptor{};
    animation_player_descriptor.timer = &timer;
    animation_player_descriptor.task_scheduler = &task_scheduler;
    animation_player_descriptor.persistent_memory_resource = &persistent_memory_resource;
    animation_player_descriptor.transient_memory_resource = &transient_memory_resource;

    AnimationPlayer animation_player(animation_player_descriptor);

    ParticleSystemPlayerDescriptor particle_system_player_descriptor{};
    particle_system_player_descriptor.timer = &timer;
    particle_system_player_descriptor.task_scheduler = &task_scheduler;
    particle_system_player_descriptor.persistent_memory_resource = &persistent_memory_resource;
    particle_system_player_descriptor.transient_memory_resource = &transient_memory_resource;
    
    ParticleSystemPlayer particle_system_player(particle_system_player_descriptor);

    ReflectionProbeManagerDescriptor reflection_probe_manager_descriptor{};
    reflection_probe_manager_descriptor.task_scheduler = &task_scheduler;
    reflection_probe_manager_descriptor.cubemap_dimension = 512;
    reflection_probe_manager_descriptor.irradiance_map_dimension = 64;
    reflection_probe_manager_descriptor.prefiltered_environment_map_dimension = 256;
    reflection_probe_manager_descriptor.persistent_memory_resource = &persistent_memory_resource;
    reflection_probe_manager_descriptor.transient_memory_resource = &transient_memory_resource;

    ReflectionProbeManager reflection_probe_manager(reflection_probe_manager_descriptor);

    OctreeAccelerationStructure geometry_acceleration_structure(persistent_memory_resource);

    LinearAccelerationStructure light_acceleration_structure(persistent_memory_resource);

    LinearAccelerationStructure particle_system_acceleration_structure(persistent_memory_resource);

    LinearAccelerationStructure reflection_probe_acceleration_structure(persistent_memory_resource);

    SceneDescriptor scene_descriptor{};
    scene_descriptor.animation_player = &animation_player;
    scene_descriptor.particle_system_player = &particle_system_player;
    scene_descriptor.reflection_probe_manager = &reflection_probe_manager;
    scene_descriptor.geometry_acceleration_structure = &geometry_acceleration_structure;
    scene_descriptor.light_acceleration_structure = &light_acceleration_structure;
    scene_descriptor.particle_system_acceleration_structure = &particle_system_acceleration_structure;
    scene_descriptor.reflection_probe_acceleration_structure = &reflection_probe_acceleration_structure;
    scene_descriptor.persistent_memory_resource = &persistent_memory_resource;
    scene_descriptor.transient_memory_resource = &transient_memory_resource;

    Scene scene(scene_descriptor);

    CameraManager camera_manager;

    DebugDrawManager debug_draw_manager(transient_memory_resource);

    ImguiManagerDescriptor imgui_manager_descriptor{};
    imgui_manager_descriptor.timer = &timer;
    imgui_manager_descriptor.input = &input;
    imgui_manager_descriptor.window = &window;
    imgui_manager_descriptor.persistent_memory_resource = &persistent_memory_resource;
    imgui_manager_descriptor.transient_memory_resource = &transient_memory_resource;

    ImguiManager imgui_manager(imgui_manager_descriptor);

    ShadowManagerDescriptor shadow_manager_descriptor{};
    shadow_manager_descriptor.render = render.get();
    shadow_manager_descriptor.scene = &scene;
    shadow_manager_descriptor.camera_manager = &camera_manager;
    shadow_manager_descriptor.shadow_map_count = 3;
    shadow_manager_descriptor.shadow_map_dimension = 512;
    shadow_manager_descriptor.persistent_memory_resource = &persistent_memory_resource;
    shadow_manager_descriptor.transient_memory_resource = &transient_memory_resource;

    ShadowManager shadow_manager(shadow_manager_descriptor);

    OpaqueShadowRenderPassDescriptor opaque_shadow_render_pass_descriptor{};
    opaque_shadow_render_pass_descriptor.scene = &scene;
    opaque_shadow_render_pass_descriptor.shadow_manager = &shadow_manager;
    opaque_shadow_render_pass_descriptor.task_scheduler = &task_scheduler;
    opaque_shadow_render_pass_descriptor.transient_memory_resource = &transient_memory_resource;

    OpaqueShadowRenderPass opaque_shadow_render_pass(opaque_shadow_render_pass_descriptor);
    
    TranslucentShadowRenderPassDescriptor transcluent_shadow_render_pass_descriptor{};
    transcluent_shadow_render_pass_descriptor.scene = &scene;
    transcluent_shadow_render_pass_descriptor.shadow_manager = &shadow_manager;
    transcluent_shadow_render_pass_descriptor.task_scheduler = &task_scheduler;
    transcluent_shadow_render_pass_descriptor.transient_memory_resource = &transient_memory_resource;

    TranslucentShadowRenderPass transcluent_shadow_render_pass(transcluent_shadow_render_pass_descriptor);

    GeometryRenderPassDescriptor geometry_render_pass_descriptor{};
    geometry_render_pass_descriptor.scene = &scene;
    geometry_render_pass_descriptor.camera_manager = &camera_manager;
    geometry_render_pass_descriptor.transient_memory_resource = &transient_memory_resource;

    GeometryRenderPass geometry_render_pass(geometry_render_pass_descriptor);

    LightingRenderPassDescriptor lighting_render_pass_descriptor{};
    lighting_render_pass_descriptor.render = render.get();
    lighting_render_pass_descriptor.scene = &scene;
    lighting_render_pass_descriptor.camera_manager = &camera_manager;
    lighting_render_pass_descriptor.shadow_manager = &shadow_manager;
    lighting_render_pass_descriptor.transient_memory_resource = &transient_memory_resource;

    LightingRenderPass lighting_render_pass(lighting_render_pass_descriptor);
    
    ReflectionProbeRenderPassDescriptor reflection_probe_render_pass_descriptor{};
    reflection_probe_render_pass_descriptor.render = render.get();
    reflection_probe_render_pass_descriptor.scene = &scene;
    reflection_probe_render_pass_descriptor.camera_manager = &camera_manager;
    reflection_probe_render_pass_descriptor.transient_memory_resource = &transient_memory_resource;

    ReflectionProbeRenderPass reflection_probe_render_pass(reflection_probe_render_pass_descriptor);
    
    EmissionRenderPassDescriptor emission_render_pass_descriptor{};
    emission_render_pass_descriptor.render = render.get();
    emission_render_pass_descriptor.transient_memory_resource = &transient_memory_resource;

    EmissionRenderPass emission_render_pass(emission_render_pass_descriptor);

    ParticleSystemRenderPassDescriptor particle_system_render_pass_descriptor{};
    particle_system_render_pass_descriptor.scene = &scene;
    particle_system_render_pass_descriptor.camera_manager = &camera_manager;
    particle_system_render_pass_descriptor.transient_memory_resource = &transient_memory_resource;

    ParticleSystemRenderPass particle_system_render_pass(particle_system_render_pass_descriptor);

    TonemappingRenderPassDescriptor tonemapping_render_pass_descriptor{};
    tonemapping_render_pass_descriptor.render = render.get();
    tonemapping_render_pass_descriptor.transient_memory_resource = &transient_memory_resource;

    TonemappingRenderPass tonemapping_render_pass(tonemapping_render_pass_descriptor);

    DebugDrawRenderPassDescriptor debug_draw_render_pass_descriptor{};
    debug_draw_render_pass_descriptor.debug_draw_manager = &debug_draw_manager;
    debug_draw_render_pass_descriptor.camera_manager = &camera_manager;
    debug_draw_render_pass_descriptor.transient_memory_resource = &transient_memory_resource;

    DebugDrawRenderPass debug_draw_render_pass(debug_draw_render_pass_descriptor);

    ImguiRenderPassDescriptor imgui_render_pass_descriptor{};
    imgui_render_pass_descriptor.render = render.get();
    imgui_render_pass_descriptor.imgui_manager = &imgui_manager;
    imgui_render_pass_descriptor.transient_memory_resource = &transient_memory_resource;

    ImguiRenderPass imgui_render_pass(imgui_render_pass_descriptor);

    Vector<AttachmentDescriptor> color_attachment_descriptors(persistent_memory_resource);
    opaque_shadow_render_pass.get_color_attachment_descriptors(color_attachment_descriptors);
    transcluent_shadow_render_pass.get_color_attachment_descriptors(color_attachment_descriptors);
    geometry_render_pass.get_color_attachment_descriptors(color_attachment_descriptors);
    lighting_render_pass.get_color_attachment_descriptors(color_attachment_descriptors);
    reflection_probe_render_pass.get_color_attachment_descriptors(color_attachment_descriptors);
    emission_render_pass.get_color_attachment_descriptors(color_attachment_descriptors);
    particle_system_render_pass.get_color_attachment_descriptors(color_attachment_descriptors);
    tonemapping_render_pass.get_color_attachment_descriptors(color_attachment_descriptors);
    debug_draw_render_pass.get_color_attachment_descriptors(color_attachment_descriptors);
    imgui_render_pass.get_color_attachment_descriptors(color_attachment_descriptors);

    Vector<AttachmentDescriptor> depth_stencil_attachment_descriptors(persistent_memory_resource);
    opaque_shadow_render_pass.get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    transcluent_shadow_render_pass.get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    geometry_render_pass.get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    lighting_render_pass.get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    reflection_probe_render_pass.get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    emission_render_pass.get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    particle_system_render_pass.get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    tonemapping_render_pass.get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    debug_draw_render_pass.get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    imgui_render_pass.get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);

    Vector<RenderPassDescriptor> render_pass_descriptors(persistent_memory_resource);
    opaque_shadow_render_pass.get_render_pass_descriptors(render_pass_descriptors);
    transcluent_shadow_render_pass.get_render_pass_descriptors(render_pass_descriptors);
    geometry_render_pass.get_render_pass_descriptors(render_pass_descriptors);
    lighting_render_pass.get_render_pass_descriptors(render_pass_descriptors);
    reflection_probe_render_pass.get_render_pass_descriptors(render_pass_descriptors);
    emission_render_pass.get_render_pass_descriptors(render_pass_descriptors);
    particle_system_render_pass.get_render_pass_descriptors(render_pass_descriptors);
    tonemapping_render_pass.get_render_pass_descriptors(render_pass_descriptors);
    debug_draw_render_pass.get_render_pass_descriptors(render_pass_descriptors);
    imgui_render_pass.get_render_pass_descriptors(render_pass_descriptors);

    FrameGraphDescriptor frame_graph_descriptor{};
    frame_graph_descriptor.render = render.get();
    frame_graph_descriptor.window = &window;
    frame_graph_descriptor.is_aliasing_enabled = true;
    frame_graph_descriptor.is_vsync_enabled = true;
    frame_graph_descriptor.descriptor_set_count_per_descriptor_pool = 256;
    frame_graph_descriptor.uniform_texture_count_per_descriptor_pool = 4 * 256;
    frame_graph_descriptor.uniform_sampler_count_per_descriptor_pool = 256;
    frame_graph_descriptor.uniform_buffer_count_per_descriptor_pool = 256;
    frame_graph_descriptor.swapchain_attachment_name = "swapchain_attachment";
    frame_graph_descriptor.color_attachment_descriptors = color_attachment_descriptors.data();
    frame_graph_descriptor.color_attachment_descriptor_count = color_attachment_descriptors.size();
    frame_graph_descriptor.depth_stencil_attachment_descriptors = depth_stencil_attachment_descriptors.data();
    frame_graph_descriptor.depth_stencil_attachment_descriptor_count = depth_stencil_attachment_descriptors.size();
    frame_graph_descriptor.render_pass_descriptors = render_pass_descriptors.data();
    frame_graph_descriptor.render_pass_descriptor_count = render_pass_descriptors.size();

    UniquePtr<FrameGraph> frame_graph(FrameGraph::create_instance(frame_graph_descriptor), persistent_memory_resource);

    opaque_shadow_render_pass.create_graphics_pipelines(*frame_graph);
    transcluent_shadow_render_pass.create_graphics_pipelines(*frame_graph);
    geometry_render_pass.create_graphics_pipelines(*frame_graph);
    lighting_render_pass.create_graphics_pipelines(*frame_graph);
    reflection_probe_render_pass.create_graphics_pipelines(*frame_graph);
    emission_render_pass.create_graphics_pipelines(*frame_graph);
    particle_system_render_pass.create_graphics_pipelines(*frame_graph);
    tonemapping_render_pass.create_graphics_pipelines(*frame_graph);
    debug_draw_render_pass.create_graphics_pipelines(*frame_graph);
    imgui_render_pass.create_graphics_pipelines(*frame_graph);

    TextureManagerDescriptor texture_manager_descriptor{};
    texture_manager_descriptor.render = render.get();
    texture_manager_descriptor.task_scheduler = &task_scheduler;
    texture_manager_descriptor.persistent_memory_resource = &persistent_memory_resource;
    texture_manager_descriptor.transient_memory_resource = &transient_memory_resource;
    texture_manager_descriptor.transient_memory_allocation = 4 * 1024 * 1024;

    TextureManager texture_manager(texture_manager_descriptor);

    GeometryManagerDescriptor geometry_manager_descriptor{};
    geometry_manager_descriptor.render = render.get();
    geometry_manager_descriptor.task_scheduler = &task_scheduler;
    geometry_manager_descriptor.persistent_memory_resource = &persistent_memory_resource;
    geometry_manager_descriptor.transient_memory_resource = &transient_memory_resource;

    GeometryManager geometry_manager(geometry_manager_descriptor);

    MaterialManagerDescriptor material_manager_descriptor{};
    material_manager_descriptor.frame_graph = frame_graph.get();
    material_manager_descriptor.task_scheduler = &task_scheduler;
    material_manager_descriptor.texture_manager = &texture_manager;
    material_manager_descriptor.persistent_memory_resource = &persistent_memory_resource;
    material_manager_descriptor.transient_memory_resource = &transient_memory_resource;

    MaterialManager material_manager(material_manager_descriptor);
    
    AnimationManagerDescriptor animation_manager_descriptor{};
    animation_manager_descriptor.task_scheduler = &task_scheduler;
    animation_manager_descriptor.persistent_memory_resource = &persistent_memory_resource;
    animation_manager_descriptor.transient_memory_resource = &transient_memory_resource;

    AnimationManager animation_manager(animation_manager_descriptor);
    
    ParticleSystemManagerDescriptor particle_system_manager_descriptor{};
    particle_system_manager_descriptor.task_scheduler = &task_scheduler;
    particle_system_manager_descriptor.geometry_manager = &geometry_manager;
    particle_system_manager_descriptor.material_manager = &material_manager;
    particle_system_manager_descriptor.persistent_memory_resource = &persistent_memory_resource;
    particle_system_manager_descriptor.transient_memory_resource = &transient_memory_resource;

    ParticleSystemManager particle_system_manager(particle_system_manager_descriptor);

    SharedPtr<Texture*> brdf_lut = texture_manager.load("resource/textures/brdf_lut.kwt");
    reflection_probe_render_pass.set_brdf_lut(brdf_lut);

    std::ifstream level_stream("resource/levels/level1.txt");
    KW_ERROR(level_stream, "Failed to open level.");

    uint32_t prototype_count;
    uint32_t instance_count;
    level_stream >> prototype_count >> instance_count;

    UnorderedMap<String, Pair<String, String>> prototypes(persistent_memory_resource);
    prototypes.reserve(prototype_count);

    for (uint32_t i = 0; i < prototype_count; i++) {
        String name(persistent_memory_resource);
        String geometry(persistent_memory_resource);
        String material(persistent_memory_resource);

        level_stream >> name;
        level_stream >> geometry;
        level_stream >> material;

        prototypes.emplace(std::move(name), Pair<String, String>{ geometry, material });
    }

    Vector<GeometryPrimitive> instances(persistent_memory_resource);
    instances.reserve(instance_count);

    ContainerPrimitive container(persistent_memory_resource);
    container.set_local_transform(transform(float3(), quaternion::rotation(float3(0.f, 0.f, 1.f), PI), float3(1.f)));
    scene.add_child(container);

    for (uint32_t i = 0; i < instance_count; i++) {
        String name(persistent_memory_resource);
        level_stream >> name;

        auto it = prototypes.find(name);
        KW_ERROR(it != prototypes.end(), "Invalid prototype name.");

        GeometryPrimitive& primitive = instances.emplace_back(GeometryPrimitive(
            geometry_manager.load(it->second.first.c_str()),
            material_manager.load(it->second.second.c_str()),
            material_manager.load("resource/materials/solid_shadow.kwm")
        ));

        float4x4 primitive_transform;

        level_stream >> primitive_transform[0][0] >> primitive_transform[0][1] >> primitive_transform[0][2] >> primitive_transform[0][3];
        level_stream >> primitive_transform[1][0] >> primitive_transform[1][1] >> primitive_transform[1][2] >> primitive_transform[1][3];
        level_stream >> primitive_transform[2][0] >> primitive_transform[2][1] >> primitive_transform[2][2] >> primitive_transform[2][3];
        level_stream >> primitive_transform[3][0] >> primitive_transform[3][1] >> primitive_transform[3][2] >> primitive_transform[3][3];

        float4x4 transform_matrix(1.f,  0.f, 0.f, 0.f,
                                  0.f,  0.f, 1.f, 0.f,
                                  0.f, -1.f, 0.f, 0.f,
                                  0.f,  0.f, 0.f, 1.f);

        primitive_transform = transform_matrix * primitive_transform * transform_matrix;

        transform local_transform = transform(primitive_transform);
        primitive.set_local_transform(local_transform);

        container.add_child(primitive);
    }

    KW_ERROR(level_stream, "Failed to read level.");

    ReflectionProbePrimitive reflection_probe_primitive(
        nullptr, nullptr, 8.f, aabbox(float3(5.f, 3.f, 0.f), float3(7.5f, 2.f, 7.5f)), transform(float3(5.f, 2.5f, 0.f))
    );
    scene.add_child(reflection_probe_primitive);

    ParticleSystemPrimitive fire_particle_system_primitive(
        persistent_memory_resource,
        particle_system_manager.load("resource/particles/fire.kwm"),
        transform(float3(5.f, 0.f, 0.f))
    );
    scene.add_child(fire_particle_system_primitive);

    ParticleSystemPrimitive smoke_particle_system_primitive(
        persistent_memory_resource,
        particle_system_manager.load("resource/particles/smoke.kwm"),
        transform(float3(5.f, 0.f, 0.f))
    );
    scene.add_child(smoke_particle_system_primitive);

    ParticleSystemPrimitive blow_ember_particle_system_primitive(
        persistent_memory_resource,
        particle_system_manager.load("resource/particles/blow_ember.kwm"),
        transform(float3(5.f, 0.f, 0.f))
    );
    scene.add_child(blow_ember_particle_system_primitive);

    AnimatedGeometryPrimitive robot_primitives[5]{
        AnimatedGeometryPrimitive(
            persistent_memory_resource,
            animation_manager.load("resource/animations/robot_orange/idle_look_back.kwg"),
            geometry_manager.load("resource/geometry/robot_orange.kwg"),
            material_manager.load("resource/materials/robot_orange.kwm"),
            material_manager.load("resource/materials/skinned_shadow.kwm"),
            transform(float3(2.f, 0.05f, -3.f))
        ),
        AnimatedGeometryPrimitive(
            persistent_memory_resource,
            animation_manager.load("resource/animations/robot_blue/idle.kwg"),
            geometry_manager.load("resource/geometry/robot_blue.kwg"),
            material_manager.load("resource/materials/robot_blue.kwm"),
            material_manager.load("resource/materials/skinned_shadow.kwm"),
            transform(float3(5.f, 0.f, 0.f))
        ),
        AnimatedGeometryPrimitive(
            persistent_memory_resource,
            animation_manager.load("resource/animations/robot_orange/idle_look_side.kwg"),
            geometry_manager.load("resource/geometry/robot_orange.kwg"),
            material_manager.load("resource/materials/robot_orange.kwm"),
            material_manager.load("resource/materials/skinned_shadow.kwm"),
            transform(float3(8.f, 0.05f, -3.f))
        ),
        AnimatedGeometryPrimitive(
            persistent_memory_resource,
            animation_manager.load("resource/animations/robot_blue/idle.kwg"),
            geometry_manager.load("resource/geometry/robot_blue.kwg"),
            material_manager.load("resource/materials/robot_blue.kwm"),
            material_manager.load("resource/materials/skinned_shadow.kwm"),
            transform(float3(3.5f, 1.f, 18.f), quaternion::rotation(float3(0.f, 1.f, 0.f), PI / 4.f))
        ),
        AnimatedGeometryPrimitive(
            persistent_memory_resource,
            animation_manager.load("resource/animations/robot_orange/idle.kwg"),
            geometry_manager.load("resource/geometry/robot_orange.kwg"),
            material_manager.load("resource/materials/robot_orange.kwm"),
            material_manager.load("resource/materials/skinned_shadow.kwm"),
            transform(float3(6.5f, 1.05f, -22.f), quaternion::rotation(float3(0.f, 1.f, 0.f), -PI / 4.f))
        )
    };
    scene.add_child(robot_primitives[0]);
    scene.add_child(robot_primitives[1]);
    scene.add_child(robot_primitives[2]);
    scene.add_child(robot_primitives[3]);
    scene.add_child(robot_primitives[4]);

    PointLightPrimitive point_light_primitives[3]{
        PointLightPrimitive(true, float3(0.6f, 1.f, 1.f), 5.f, transform(float3(3.f, 4.f, 3.f))),
        PointLightPrimitive(true, float3(0.6f, 1.f, 1.f), 5.f, transform(float3(5.f, 3.5f, 20.f))),
        PointLightPrimitive(true, float3(0.6f, 1.f, 1.f), 5.f, transform(float3(5.f, 3.5f, -20.f))),
    };
    scene.add_child(point_light_primitives[0]);
    scene.add_child(point_light_primitives[1]);
    scene.add_child(point_light_primitives[2]);

    reflection_probe_manager.bake(*render, scene, std::move(brdf_lut));

    bool draw_light[std::size(point_light_primitives)]{};

    float camera_yaw = radians(0.f);
    float camera_pitch = radians(5.f);
    float3 camera_position(5.f, 3.5f, 7.f);
    float mouse_sensitivity = 0.0025f;
    float camera_speed = 12.f;

    bool draw_occlusion_camera = false;

    bool auto_play = true;
    float time_ = 0.f;
    float old_time = 0.f;

    int cpu_profiler_offset = 0;

    Camera& camera = camera_manager.get_camera();
    camera.set_fov(radians(60.f));
    camera.set_z_near(0.1f);
    camera.set_z_far(100.f);

    ImGui& imgui = imgui_manager.get_imgui();

    bool is_running = true;
    while (is_running) {
        transient_memory_resource.reset();

        Event event;
        while (event_loop.poll_event(transient_memory_resource, event)) {
            if (event.type == EventType::QUIT) {
                is_running = false;
            } else {
                input.push_event(event);
            }
        }

        input.update();
        timer.update();
        debug_draw_manager.update();
        imgui_manager.update();

        if (input.is_button_down(BUTTON_LEFT)) {
            camera_yaw += input.get_mouse_dx() * mouse_sensitivity;
            camera_pitch += input.get_mouse_dy() * mouse_sensitivity;
        }

        quaternion camera_rotation = quaternion::rotation(float3(0.f, 1.f, 0.f), camera_yaw) * quaternion::rotation(float3(1.f, 0.f, 0.f), camera_pitch);

        float3 forward = float3(0.f, 0.f, -1.f) * camera_rotation;
        float3 left = float3(-1.f, 0.f, 0.f) * camera_rotation;
        float3 up = float3(0.f, 1.f, 0.f);

        if (input.is_key_down(Scancode::W)) {
            camera_position -= forward * camera_speed * timer.get_elapsed_time();
        }
        if (input.is_key_down(Scancode::A)) {
            camera_position += left * camera_speed * timer.get_elapsed_time();
        }
        if (input.is_key_down(Scancode::S)) {
            camera_position += forward * camera_speed * timer.get_elapsed_time();
        }
        if (input.is_key_down(Scancode::D)) {
            camera_position -= left * camera_speed * timer.get_elapsed_time();
        }
        if (input.is_key_down(Scancode::Q)) {
            camera_position -= up * camera_speed * timer.get_elapsed_time();
        }
        if (input.is_key_down(Scancode::E)) {
            camera_position += up * camera_speed * timer.get_elapsed_time();
        }

        camera.set_aspect_ratio(static_cast<float>(window.get_width()) / window.get_height());
        camera.set_rotation(camera_rotation);
        camera.set_translation(camera_position);

        if (imgui.Begin("Lights")) {
            for (size_t i = 0; i < std::size(point_light_primitives); i++) {
                char header_text[] = "light0";
                header_text[5] = char('0' + i);

                imgui.PushID(header_text);

                if (imgui.CollapsingHeader(header_text)) {
                    float3 light_position = point_light_primitives[i].get_global_translation();
                    float3 light_color = point_light_primitives[i].get_color();
                    float light_power = point_light_primitives[i].get_power();
                    PointLightPrimitive::ShadowParams shadow_params = point_light_primitives[i].get_shadow_params();

                    imgui.DragFloat3("Light Position", &light_position, 0.01f);
                    imgui.ColorEdit3("Light Color", &light_color);
                    imgui.DragFloat("Light Power", &light_power, 0.01f, 0.f, FLT_MAX);
                    imgui.DragFloat("normal_bias", &shadow_params.normal_bias, 0.001, 0.f, FLT_MAX);
                    imgui.DragFloat("perspective_bias", &shadow_params.perspective_bias, 0.00001, 0.f, FLT_MAX, "%.6f");
                    imgui.DragFloat("pcss_radius", &shadow_params.pcss_radius, 0.1, 0.f, FLT_MAX);
                    imgui.DragFloat("pcss_filter_factor", &shadow_params.pcss_filter_factor, 0.01, 0.f, FLT_MAX);
                    imgui.Checkbox("Draw Light", &draw_light[i]);

                    point_light_primitives[i].set_global_translation(light_position);
                    point_light_primitives[i].set_color(light_color);
                    point_light_primitives[i].set_power(light_power);
                    point_light_primitives[i].set_shadow_params(shadow_params);

                    if (draw_light[i]) {
                        debug_draw_manager.icosahedron(light_position, 0.01f, float3(1.f, 0.f, 0.f));
                        debug_draw_manager.icosahedron(light_position, shadow_params.pcss_radius * 0.1f, float3(1.f));
                    }

                    static quaternion SIDE_ROTATIONS[] = {
                        quaternion::rotation(float3(0.f, 1.f, 0.f), 0.f),
                        quaternion::rotation(float3(0.f, 1.f, 0.f), PI / 2),
                        quaternion::rotation(float3(0.f, 1.f, 0.f), PI),
                        quaternion::rotation(float3(0.f, 1.f, 0.f), -PI / 2),
                        quaternion::rotation(float3(1.f, 0.f, 0.f), PI / 2),
                        quaternion::rotation(float3(1.f, 0.f, 0.f), -PI / 2),
                    };

                    for (size_t j = 0; j < 6; j++) {
                        String button_label("Side 0: Set Occlusion Camera", transient_memory_resource);
                        button_label[5] = '0' + j;
                        if (imgui.Button(button_label.c_str())) {
                            transform transform;
                            transform.translation = light_position;
                            transform.rotation = SIDE_ROTATIONS[j];
                            float fov = PI / 2;
                            float aspect_ratio = 1.f;
                            float z_near = 0.1f;
                            float z_far = 20.f;

                            bool use_occlusion_camera = camera_manager.is_occlusion_camera_used();

                            camera_manager.toggle_occlusion_camera_used(true);
                            Camera& occlusion_camera = camera_manager.get_occlusion_camera();
                            camera_manager.toggle_occlusion_camera_used(use_occlusion_camera);

                            occlusion_camera.set_transform(transform);
                            occlusion_camera.set_fov(fov);
                            occlusion_camera.set_aspect_ratio(aspect_ratio);
                            occlusion_camera.set_z_near(z_near);
                            occlusion_camera.set_z_far(z_far);
                        }
                    }
                }

                imgui.PopID();
            }
        }
        imgui.End();

        if (robot_primitives[1].get_geometry()) {
            const Skeleton* skeleton = robot_primitives[1].get_geometry()->get_skeleton();
            if (skeleton != nullptr) {
                SkeletonPose& skeleton_pose = robot_primitives[1].get_skeleton_pose();
                const Vector<float4x4>& joint_space_matrices = skeleton_pose.get_joint_space_matrices();
                const Vector<float4x4>& model_space_matrices = skeleton_pose.get_model_space_matrices();
                size_t joint_count = skeleton->get_joint_count();

                if (imgui.Begin("Skinning")) {
                    imgui.Checkbox("Play", &auto_play);
                    imgui.DragFloat("Time", &time_, 0.01, 1.3f, 1.6f);

                    for (size_t i = 0; i < joint_count; i++) {
                        const String& name = skeleton->get_joint_name(i);

                        imgui.PushID(name.c_str());

                        if (imgui.CollapsingHeader(name.c_str())) {
                            transform transform(joint_space_matrices[i]);

                            bool changed_translation = imgui.DragFloat3("translation", &transform.translation, 0.01f);
                            bool changed_rotation = imgui.DragFloat4("rotation", &transform.rotation, 0.01f);
                            bool changed_scale = imgui.DragFloat3("scale", &transform.scale, 0.01f);

                            if (changed_translation || changed_rotation || changed_scale) {
                                transform.rotation = normalize(transform.rotation);
                                skeleton_pose.set_joint_space_matrix(i, float4x4(transform));
                                skeleton_pose.build_model_space_matrices(*skeleton);
                            }
                        }

                        imgui.PopID();
                    }
                }
                imgui.End();
            }
        }

        if (imgui.Begin("Camera")) {
            float2 rotation(camera_yaw, camera_pitch);
            float fov = camera.get_fov();
            float z_near = camera.get_z_near();
            float z_far = camera.get_z_far();

            imgui.DragFloat3("translation", &camera_position, 0.01f);
            imgui.DragFloat2("rotation", &rotation, 0.01f);
            imgui.DragFloat("fov", &fov, 0.01f);
            imgui.DragFloat("z_near", &z_near, 0.01f);
            imgui.DragFloat("z_far", &z_far, 0.01f);

            camera_yaw = rotation.x;
            camera_pitch = rotation.y;

            camera_rotation = quaternion::rotation(float3(0.f, 1.f, 0.f), camera_yaw) * quaternion::rotation(float3(1.f, 0.f, 0.f), camera_pitch);

            camera.set_rotation(camera_rotation);
            camera.set_translation(camera_position);
            camera.set_fov(fov);
            camera.set_z_near(z_near);
            camera.set_z_far(z_far);
        }
        imgui.End();

        if (imgui.Begin("Occlusion Camera")) {
            bool use_occlusion_camera = camera_manager.is_occlusion_camera_used();

            camera_manager.toggle_occlusion_camera_used(true);
            Camera& occlusion_camera = camera_manager.get_occlusion_camera();
            camera_manager.toggle_occlusion_camera_used(use_occlusion_camera);

            transform transform = occlusion_camera.get_transform();
            float fov = occlusion_camera.get_fov();
            float aspect_ratio = occlusion_camera.get_aspect_ratio();
            float z_near = occlusion_camera.get_z_near();
            float z_far = occlusion_camera.get_z_far();
            frustum frustum = occlusion_camera.get_frustum();

            if (imgui.Button("Set from camera")) {
                transform.translation = camera_position;
                transform.rotation = camera_rotation;
                fov = camera.get_fov();
                aspect_ratio = camera.get_aspect_ratio();
                z_near = camera.get_z_near();
                z_far = camera.get_z_far();
            }

            imgui.DragFloat3("translation", &transform.translation, 0.01f);
            imgui.DragFloat4("rotation", &transform.rotation, 0.01f);
            imgui.DragFloat("fov", &fov, 0.01f);
            imgui.DragFloat("aspect_ratio", &aspect_ratio, 0.01f);
            imgui.DragFloat("z_near", &z_near, 0.01f);
            imgui.DragFloat("z_far", &z_far, 0.01f);
            imgui.Checkbox("Draw occlusion camera", &draw_occlusion_camera);
            imgui.Checkbox("Use occlusion camera", &use_occlusion_camera);

            transform.rotation = normalize(transform.rotation);

            occlusion_camera.set_transform(transform);
            occlusion_camera.set_fov(fov);
            occlusion_camera.set_aspect_ratio(aspect_ratio);
            occlusion_camera.set_z_near(z_near);
            occlusion_camera.set_z_far(z_far);

            if (draw_occlusion_camera) {
                debug_draw_manager.frustum(occlusion_camera.get_inverse_view_projection_matrix(), float3(1.f));
            }

            camera_manager.toggle_occlusion_camera_used(use_occlusion_camera);
        }
        imgui.End();

        if (imgui.Begin("CPU Profiler")) {
            CpuProfiler& cpu_profiler = CpuProfiler::instance();

            if (imgui.Button("Pause/Resume")) {
                cpu_profiler.toggle_pause(!CpuProfiler::instance().is_paused());
            }

            imgui.SameLine();

            imgui.SliderInt("##Offset", &cpu_profiler_offset, 0, cpu_profiler.get_frame_count() - 1);

            ImDrawList* draw_list = imgui.GetWindowDrawList();
            ImVec2 size = imgui.GetWindowSize();
            ImVec2 mouse_position = imgui.GetIO().MousePos;

            Vector<CpuProfiler::Scope> scopes = cpu_profiler.get_scopes(transient_memory_resource, size_t(cpu_profiler_offset));
            if (!scopes.empty()) {
                uint64_t min_timestamp = UINT64_MAX;
                uint64_t max_timestamp = 0;

                Set<const char*> unique_threads(transient_memory_resource);

                for (const CpuProfiler::Scope& scope : scopes) {
                    min_timestamp = std::min(min_timestamp, scope.begin_timestamp);
                    max_timestamp = std::max(max_timestamp, scope.end_timestamp);

                    unique_threads.emplace(scope.thread_name);
                }

                uint64_t frame_duration = max_timestamp - min_timestamp;

                imgui.SameLine();
                imgui.Text("Frame time: %f ms", frame_duration / 1e6f);

                static const uint32_t CIEDE2000_COLORS[] = {
                    0xFF3B9700, 0xFFFFFF00, 0xFF1CE6FF, 0xFFFF34FF, 0xFFFF4A46, 0xFF008941, 0xFF006FA6, 0xFFA30059,
                    0xFFFFDBE5, 0xFF7A4900, 0xFF0000A6, 0xFF63FFAC, 0xFFB79762, 0xFF004D43, 0xFF8FB0FF, 0xFF997D87,
                    0xFF5A0007, 0xFF809693, 0xFFFEFFE6, 0xFF1B4400, 0xFF4FC601, 0xFF3B5DFF, 0xFF4A3B53, 0xFFFF2F80,
                    0xFF61615A, 0xFFBA0900, 0xFF6B7900, 0xFF00C2A0, 0xFFFFAA92, 0xFFFF90C9, 0xFFB903AA, 0xFFD16100,
                    0xFFDDEFFF, 0xFF000035, 0xFF7B4F4B, 0xFFA1C299, 0xFF300018, 0xFF0AA6D8, 0xFF013349, 0xFF00846F,
                    0xFF372101, 0xFFFFB500, 0xFFC2FFED, 0xFFA079BF, 0xFFCC0744, 0xFFC0B9B2, 0xFFC2FF99, 0xFF001E09,
                    0xFF00489C, 0xFF6F0062, 0xFF0CBD66, 0xFFEEC3FF, 0xFF456D75, 0xFFB77B68, 0xFF7A87A1, 0xFF788D66,
                    0xFF885578, 0xFFFAD09F, 0xFFFF8A9A, 0xFFD157A0, 0xFFBEC459, 0xFF456648, 0xFF0086ED, 0xFF886F4C,
                    0xFF34362D, 0xFFB4A8BD, 0xFF00A6AA, 0xFF452C2C, 0xFF636375, 0xFFA3C8C9, 0xFFFF913F, 0xFF938A81,
                    0xFF575329, 0xFF00FECF, 0xFFB05B6F, 0xFF8CD0FF, 0xFFD83D66, 0xFF04F757, 0xFFC8A1A1, 0xFF1E6E00,
                    0xFF7900D7, 0xFFA77500, 0xFF6367A9, 0xFFA05837, 0xFF6B002C, 0xFF772600, 0xFFD790FF, 0xFF9B9700,
                    0xFF549E79, 0xFFFFF69F, 0xFF201625, 0xFF72418F, 0xFFBC23FF, 0xFF99ADC0, 0xFF3A2465, 0xFF922329,
                    0xFF5B4534, 0xFFFDE8DC, 0xFF404E55, 0xFF0089A3, 0xFFCB7E98, 0xFFA4E804, 0xFF324E72, 0xFF6A3A4C,
                    0xFF83AB58, 0xFF001C1E, 0xFFD1F7CE, 0xFF004B28, 0xFFC8D0F6, 0xFFA3A489, 0xFF806C66, 0xFF222800,
                    0xFFBF5650, 0xFFE83000, 0xFF66796D, 0xFFDA007C, 0xFFFF1A59, 0xFF8ADBB4, 0xFF1E0200, 0xFF5B4E51,
                    0xFFC895C5, 0xFF320033, 0xFFFF6832, 0xFF66E1D3, 0xFFCFCDAC, 0xFFD0AC94, 0xFF7ED379, 0xFF012C58,
                    0xFF7A7BFF, 0xFFD68E01, 0xFF353339, 0xFF78AFA1, 0xFFFEB2C6, 0xFF75797C, 0xFF837393, 0xFF943A4D,
                    0xFFB5F4FF, 0xFFD2DCD5, 0xFF9556BD, 0xFF6A714A, 0xFF001325, 0xFF02525F, 0xFF0AA3F7, 0xFFE98176,
                    0xFFDBD5DD, 0xFF5EBCD1, 0xFF3D4F44, 0xFF7E6405, 0xFF02684E, 0xFF962B75, 0xFF8D8546, 0xFF9695C5,
                    0xFFE773CE, 0xFFD86A78, 0xFF3E89BE, 0xFFCA834E, 0xFF518A87, 0xFF5B113C, 0xFF55813B, 0xFFE704C4,
                    0xFF00005F, 0xFFA97399, 0xFF4B8160, 0xFF59738A, 0xFFFF5DA7, 0xFFF7C9BF, 0xFF643127, 0xFF513A01,
                    0xFF6B94AA, 0xFF51A058, 0xFFA45B02, 0xFF1D1702, 0xFFE20027, 0xFFE7AB63, 0xFF4C6001, 0xFF9C6966,
                    0xFF64547B, 0xFF97979E, 0xFF006A66, 0xFF391406, 0xFFF4D749, 0xFF0045D2, 0xFF006C31, 0xFFDDB6D0,
                    0xFF7C6571, 0xFF9FB2A4, 0xFF00D891, 0xFF15A08A, 0xFFBC65E9, 0xFFFFFFFE, 0xFFC6DC99, 0xFF203B3C,
                    0xFF671190, 0xFF6B3A64, 0xFFF5E1FF, 0xFFFFA0F2, 0xFFCCAA35, 0xFF374527, 0xFF8BB400, 0xFF797868,
                    0xFFC6005A, 0xFF3B000A, 0xFFC86240, 0xFF29607C, 0xFF402334, 0xFF7D5A44, 0xFFCCB87C, 0xFFB88183,
                    0xFFAA5199, 0xFFB5D6C3, 0xFFA38469, 0xFF9F94F0, 0xFFA74571, 0xFFB894A6, 0xFF71BB8C, 0xFF00B433,
                    0xFF789EC9, 0xFF6D80BA, 0xFF953F00, 0xFF5EFF03, 0xFFE4FFFC, 0xFF1BE177, 0xFFBCB1E5, 0xFF76912F,
                    0xFF003109, 0xFF0060CD, 0xFFD20096, 0xFF895563, 0xFF29201D, 0xFF5B3213, 0xFFA76F42, 0xFF89412E,
                    0xFF1A3A2A, 0xFF494B5A, 0xFFA88C85, 0xFFF4ABAA, 0xFFA3F3AB, 0xFF00C6C8, 0xFFEA8B66, 0xFF958A9F,
                    0xFFBDC9D2, 0xFF9FA064, 0xFFBE4700, 0xFF658188, 0xFF83A485, 0xFF453C23, 0xFF47675D, 0xFF3A3F00,
                    0xFF061203, 0xFFDFFB71, 0xFF868E7E, 0xFF98D058, 0xFF6C8F7D, 0xFFD7BFC2, 0xFF3C3E6E, 0xFF000000,
                };

                uint32_t i = 0;

                for (const char* thread_name : unique_threads) {
                    imgui.Text("%s:", thread_name);

                    ImVec2 position = imgui.GetCursorScreenPos();

                    Vector<uint64_t> end_timestamp_stack(transient_memory_resource);
                    end_timestamp_stack.reserve(8);

                    uint32_t y = 0;
                    uint32_t max_y = 1;

                    for (const CpuProfiler::Scope& scope : scopes) {
                        if (scope.thread_name == thread_name) {
                            float relative_begin = static_cast<float>(scope.begin_timestamp - min_timestamp) / frame_duration;
                            float relative_end = static_cast<float>(scope.end_timestamp - min_timestamp) / frame_duration;

                            while (!end_timestamp_stack.empty() && scope.begin_timestamp >= end_timestamp_stack.back()) {
                                end_timestamp_stack.pop_back();
                                y--;
                            }

                            ImVec2 left_top(position.x + relative_begin * size.x, position.y + 24.f * y);
                            ImVec2 right_bottom(position.x + relative_end * size.x, position.y + 24.f * (y + 1));

                            ImVec2 text_size = imgui.CalcTextSize(scope.scope_name);

                            float text_left = left_top.x + std::max(((right_bottom.x - left_top.x) - text_size.x) / 2.f, 0.f);
                            float text_top = (left_top.y + right_bottom.y - text_size.y) / 2.f;
                            ImVec4 text_bounds(left_top.x, left_top.y, right_bottom.x, right_bottom.y);

                            draw_list->AddRectFilled(left_top, right_bottom, CIEDE2000_COLORS[i % std::size(CIEDE2000_COLORS)]);
                            draw_list->AddText(nullptr, 0.f, ImVec2(text_left, text_top), 0xFF000000, scope.scope_name, nullptr, 0.f, &text_bounds);

                            if (mouse_position.x >= left_top.x && mouse_position.y >= left_top.y &&
                                mouse_position.x < right_bottom.x && mouse_position.y < right_bottom.y) {
                                imgui.SetTooltip("%s (%f ms)", scope.scope_name, (scope.end_timestamp - scope.begin_timestamp) / 1e6f);
                            }

                            end_timestamp_stack.push_back(scope.end_timestamp);
                            max_y = std::max(max_y, ++y);

                            i++;
                        }
                    }

                    imgui.Dummy(ImVec2(size.x, max_y * 24.f));
                }
            }
        }
        imgui.End();

        auto [animation_player_begin, animation_player_end] = animation_player.create_tasks();
        auto [particle_system_player_begin, particle_system_player_end] = particle_system_player.create_tasks();
        auto [texture_manager_begin, texture_manager_end] = texture_manager.create_tasks();
        auto [geometry_manager_begin, geometry_manager_end] = geometry_manager.create_tasks();
        MaterialManagerTasks material_manager_tasks = material_manager.create_tasks();
        auto [animation_manager_begin, animation_manager_end] = animation_manager.create_tasks();
        auto [particle_system_manager_begin, particle_system_manager_end] = particle_system_manager.create_tasks();
        auto [acquire_frame_task, present_frame_task] = frame_graph->create_tasks();
        auto [reflection_probe_manager_begin, reflection_probe_manager_end] = reflection_probe_manager.create_tasks();
        Task* shadow_manager_task = shadow_manager.create_task();
        auto [opaque_shadow_render_pass_task_begin, opaque_shadow_render_pass_task_end] = opaque_shadow_render_pass.create_tasks();
        auto [transcluent_shadow_render_pass_task_begin, transcluent_shadow_render_pass_task_end] = transcluent_shadow_render_pass.create_tasks();
        Task* geometry_render_pass_task = geometry_render_pass.create_task();
        Task* lighting_render_pass_task = lighting_render_pass.create_task();
        Task* reflection_probe_render_pass_task = reflection_probe_render_pass.create_task();
        Task* emission_render_pass_task = emission_render_pass.create_task();
        Task* particle_system_render_pass_task = particle_system_render_pass.create_task();
        Task* tonemapping_render_pass_task = tonemapping_render_pass.create_task();
        Task* debug_draw_render_pass_task = debug_draw_render_pass.create_task();
        Task* imgui_render_pass_task = imgui_render_pass.create_task();
        Task* flush_task = render->create_task();

        animation_player_begin->add_input_dependencies(transient_memory_resource, { animation_manager_end });
        particle_system_player_begin->add_input_dependencies(transient_memory_resource, { particle_system_manager_end });
        reflection_probe_manager_begin->add_input_dependencies(transient_memory_resource, { acquire_frame_task });
        reflection_probe_manager_end->add_input_dependencies(transient_memory_resource, { reflection_probe_manager_begin, flush_task });
        animation_player_end->add_input_dependencies(transient_memory_resource, { animation_player_begin });
        particle_system_player_end->add_input_dependencies(transient_memory_resource, { particle_system_player_begin });
        material_manager_tasks.begin->add_input_dependencies(transient_memory_resource, { particle_system_manager_end });
        material_manager_tasks.material_end->add_input_dependencies(transient_memory_resource, { material_manager_tasks.begin });
        material_manager_tasks.graphics_pipeline_end->add_input_dependencies(transient_memory_resource, { material_manager_tasks.material_end });
        texture_manager_begin->add_input_dependencies(transient_memory_resource, { material_manager_tasks.material_end });
        texture_manager_end->add_input_dependencies(transient_memory_resource, { texture_manager_begin });
        geometry_manager_end->add_input_dependencies(transient_memory_resource, { geometry_manager_begin });
        animation_manager_end->add_input_dependencies(transient_memory_resource, { animation_manager_begin });
        particle_system_manager_end->add_input_dependencies(transient_memory_resource, { particle_system_manager_begin });
        acquire_frame_task->add_input_dependencies(transient_memory_resource, { animation_manager_end, material_manager_tasks.graphics_pipeline_end, texture_manager_end, geometry_manager_end });
        opaque_shadow_render_pass_task_begin->add_input_dependencies(transient_memory_resource, { acquire_frame_task, animation_player_end, shadow_manager_task });
        opaque_shadow_render_pass_task_end->add_input_dependencies(transient_memory_resource, { opaque_shadow_render_pass_task_begin });
        transcluent_shadow_render_pass_task_begin->add_input_dependencies(transient_memory_resource, { acquire_frame_task, particle_system_player_end, shadow_manager_task });
        transcluent_shadow_render_pass_task_end->add_input_dependencies(transient_memory_resource, { transcluent_shadow_render_pass_task_begin });
        geometry_render_pass_task->add_input_dependencies(transient_memory_resource, { acquire_frame_task, animation_player_end });
        lighting_render_pass_task->add_input_dependencies(transient_memory_resource, { acquire_frame_task, shadow_manager_task });
        reflection_probe_render_pass_task->add_input_dependencies(transient_memory_resource, { acquire_frame_task });
        emission_render_pass_task->add_input_dependencies(transient_memory_resource, { acquire_frame_task });
        particle_system_render_pass_task->add_input_dependencies(transient_memory_resource, { acquire_frame_task, particle_system_player_end });
        tonemapping_render_pass_task->add_input_dependencies(transient_memory_resource, { acquire_frame_task });
        debug_draw_render_pass_task->add_input_dependencies(transient_memory_resource, { acquire_frame_task });
        imgui_render_pass_task->add_input_dependencies(transient_memory_resource, { acquire_frame_task });
        flush_task->add_input_dependencies(transient_memory_resource, {
            opaque_shadow_render_pass_task_end, transcluent_shadow_render_pass_task_end, geometry_render_pass_task,
            lighting_render_pass_task, reflection_probe_render_pass_task, emission_render_pass_task, particle_system_render_pass_task,
            tonemapping_render_pass_task, debug_draw_render_pass_task, imgui_render_pass_task
        });
        present_frame_task->add_input_dependencies(transient_memory_resource, { acquire_frame_task, flush_task });

        task_scheduler.enqueue_task(transient_memory_resource, reflection_probe_manager_begin);
        task_scheduler.enqueue_task(transient_memory_resource, reflection_probe_manager_end);
        task_scheduler.enqueue_task(transient_memory_resource, animation_player_begin);
        task_scheduler.enqueue_task(transient_memory_resource, animation_player_end);
        task_scheduler.enqueue_task(transient_memory_resource, particle_system_player_begin);
        task_scheduler.enqueue_task(transient_memory_resource, particle_system_player_end);
        task_scheduler.enqueue_task(transient_memory_resource, animation_manager_begin);
        task_scheduler.enqueue_task(transient_memory_resource, animation_manager_end);
        task_scheduler.enqueue_task(transient_memory_resource, particle_system_manager_begin);
        task_scheduler.enqueue_task(transient_memory_resource, particle_system_manager_end);
        task_scheduler.enqueue_task(transient_memory_resource, material_manager_tasks.begin);
        task_scheduler.enqueue_task(transient_memory_resource, material_manager_tasks.material_end);
        task_scheduler.enqueue_task(transient_memory_resource, material_manager_tasks.graphics_pipeline_end);
        task_scheduler.enqueue_task(transient_memory_resource, texture_manager_begin);
        task_scheduler.enqueue_task(transient_memory_resource, texture_manager_end);
        task_scheduler.enqueue_task(transient_memory_resource, geometry_manager_begin);
        task_scheduler.enqueue_task(transient_memory_resource, geometry_manager_end);
        task_scheduler.enqueue_task(transient_memory_resource, acquire_frame_task);
        task_scheduler.enqueue_task(transient_memory_resource, shadow_manager_task);
        task_scheduler.enqueue_task(transient_memory_resource, opaque_shadow_render_pass_task_begin);
        task_scheduler.enqueue_task(transient_memory_resource, opaque_shadow_render_pass_task_end);
        task_scheduler.enqueue_task(transient_memory_resource, transcluent_shadow_render_pass_task_begin);
        task_scheduler.enqueue_task(transient_memory_resource, transcluent_shadow_render_pass_task_end);
        task_scheduler.enqueue_task(transient_memory_resource, geometry_render_pass_task);
        task_scheduler.enqueue_task(transient_memory_resource, lighting_render_pass_task);
        task_scheduler.enqueue_task(transient_memory_resource, reflection_probe_render_pass_task);
        task_scheduler.enqueue_task(transient_memory_resource, emission_render_pass_task);
        task_scheduler.enqueue_task(transient_memory_resource, particle_system_render_pass_task);
        task_scheduler.enqueue_task(transient_memory_resource, tonemapping_render_pass_task);
        task_scheduler.enqueue_task(transient_memory_resource, debug_draw_render_pass_task);
        task_scheduler.enqueue_task(transient_memory_resource, imgui_render_pass_task);
        task_scheduler.enqueue_task(transient_memory_resource, flush_task);
        task_scheduler.enqueue_task(transient_memory_resource, present_frame_task);

        task_scheduler.join();

        CpuProfiler::instance().update();
    }

    reflection_probe_render_pass.set_brdf_lut(nullptr);
    
    imgui_render_pass.destroy_graphics_pipelines(*frame_graph);
    debug_draw_render_pass.destroy_graphics_pipelines(*frame_graph);
    tonemapping_render_pass.destroy_graphics_pipelines(*frame_graph);
    particle_system_render_pass.destroy_graphics_pipelines(*frame_graph);
    emission_render_pass.destroy_graphics_pipelines(*frame_graph);
    reflection_probe_render_pass.destroy_graphics_pipelines(*frame_graph);
    lighting_render_pass.destroy_graphics_pipelines(*frame_graph);
    geometry_render_pass.destroy_graphics_pipelines(*frame_graph);
    transcluent_shadow_render_pass.destroy_graphics_pipelines(*frame_graph);
    opaque_shadow_render_pass.destroy_graphics_pipelines(*frame_graph);

    return 0;
}
