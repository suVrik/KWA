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
#include <render/camera/camera_controller.h>
#include <render/camera/camera_manager.h>
#include <render/container/container_manager.h>
#include <render/container/container_primitive.h>
#include <render/debug/cpu_profiler_overlay.h>
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
#include <render/render_passes/antialiasing_render_pass.h>
#include <render/render_passes/bloom_render_pass.h>
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

using namespace kw;

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/) {
    DebugUtils::subscribe_to_segfault();

    ConcurrencyUtils::set_current_thread_name("Main Thread");

    MallocMemoryResource& persistent_memory_resource = MallocMemoryResource::instance();
    ScratchMemoryResource transient_memory_resource(persistent_memory_resource, 24 * 1024 * 1024);

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
    render_descriptor.transient_buffer_size = 16 * 1024 * 1024;
    render_descriptor.buffer_allocation_size = 4 * 1024 * 1024;
    render_descriptor.buffer_block_size = 16 * 1024;
    render_descriptor.texture_allocation_size = 32 * 1024 * 1024;
    render_descriptor.texture_block_size = 64 * 1024;

    UniquePtr<Render> render(Render::create_instance(render_descriptor), persistent_memory_resource);
    
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

    ContainerManagerDescriptor container_manager_descriptor{};
    container_manager_descriptor.task_scheduler = &task_scheduler;
    container_manager_descriptor.texture_manager = &texture_manager;
    container_manager_descriptor.geometry_manager = &geometry_manager;
    container_manager_descriptor.material_manager = &material_manager;
    container_manager_descriptor.animation_manager = &animation_manager;
    container_manager_descriptor.particle_system_manager = &particle_system_manager;
    container_manager_descriptor.persistent_memory_resource = &persistent_memory_resource;
    container_manager_descriptor.transient_memory_resource = &transient_memory_resource;

    ContainerManager container_manager(container_manager_descriptor);

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
    reflection_probe_manager_descriptor.texture_manager = &texture_manager;
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

    CameraControllerDescriptor camera_controller_descriptor{};
    camera_controller_descriptor.window = &window;
    camera_controller_descriptor.input = &input;
    camera_controller_descriptor.timer = &timer;
    camera_controller_descriptor.camera_manager = &camera_manager;

    CameraController camera_controller(camera_controller_descriptor);

    DebugDrawManager debug_draw_manager(transient_memory_resource);

    ImguiManagerDescriptor imgui_manager_descriptor{};
    imgui_manager_descriptor.timer = &timer;
    imgui_manager_descriptor.input = &input;
    imgui_manager_descriptor.window = &window;
    imgui_manager_descriptor.persistent_memory_resource = &persistent_memory_resource;
    imgui_manager_descriptor.transient_memory_resource = &transient_memory_resource;

    ImguiManager imgui_manager(imgui_manager_descriptor);

    CpuProfilerOverlayDescriptor cpu_profiler_overlay_descriptor{};
    cpu_profiler_overlay_descriptor.imgui_manager = &imgui_manager;
    cpu_profiler_overlay_descriptor.transient_memory_resource = &transient_memory_resource;

    CpuProfilerOverlay cpu_profiler_overlay(cpu_profiler_overlay_descriptor);

    ShadowManagerDescriptor shadow_manager_descriptor{};
    shadow_manager_descriptor.render = render.get();
    shadow_manager_descriptor.scene = &scene;
    shadow_manager_descriptor.camera_manager = &camera_manager;
    shadow_manager_descriptor.shadow_map_count = 3;
    shadow_manager_descriptor.shadow_map_dimension = 1024;
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
    reflection_probe_render_pass_descriptor.texture_manager = &texture_manager;
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

    BloomRenderPassDescriptor bloom_render_pass_descriptor{};
    bloom_render_pass_descriptor.render = render.get();
    bloom_render_pass_descriptor.mip_count = 4;
    bloom_render_pass_descriptor.blur_radius = 1.f;
    bloom_render_pass_descriptor.transparency = 0.05f;
    bloom_render_pass_descriptor.persistent_memory_resource = &persistent_memory_resource;
    bloom_render_pass_descriptor.transient_memory_resource = &transient_memory_resource;

    BloomRenderPass bloom_render_pass(bloom_render_pass_descriptor);
    
    TonemappingRenderPassDescriptor tonemapping_render_pass_descriptor{};
    tonemapping_render_pass_descriptor.render = render.get();
    tonemapping_render_pass_descriptor.transient_memory_resource = &transient_memory_resource;

    TonemappingRenderPass tonemapping_render_pass(tonemapping_render_pass_descriptor);
    
    AntialiasingRenderPassDescriptor antialiasing_render_pass_descriptor{};
    antialiasing_render_pass_descriptor.render = render.get();
    antialiasing_render_pass_descriptor.transient_memory_resource = &transient_memory_resource;

    AntialiasingRenderPass antialiasing_render_pass(antialiasing_render_pass_descriptor);

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
    bloom_render_pass.get_color_attachment_descriptors(color_attachment_descriptors);
    tonemapping_render_pass.get_color_attachment_descriptors(color_attachment_descriptors);
    antialiasing_render_pass.get_color_attachment_descriptors(color_attachment_descriptors);
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
    bloom_render_pass.get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    tonemapping_render_pass.get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    antialiasing_render_pass.get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
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
    bloom_render_pass.get_render_pass_descriptors(render_pass_descriptors);
    tonemapping_render_pass.get_render_pass_descriptors(render_pass_descriptors);
    antialiasing_render_pass.get_render_pass_descriptors(render_pass_descriptors);
    debug_draw_render_pass.get_render_pass_descriptors(render_pass_descriptors);
    imgui_render_pass.get_render_pass_descriptors(render_pass_descriptors);

    FrameGraphDescriptor frame_graph_descriptor{};
    frame_graph_descriptor.render = render.get();
    frame_graph_descriptor.window = &window;
    frame_graph_descriptor.is_aliasing_enabled = true;
    frame_graph_descriptor.is_vsync_enabled = true;
    frame_graph_descriptor.descriptor_set_count_per_descriptor_pool = 256;
    frame_graph_descriptor.uniform_texture_count_per_descriptor_pool = 4 * 256;
    frame_graph_descriptor.uniform_sampler_count_per_descriptor_pool = 2 * 256;
    frame_graph_descriptor.uniform_buffer_count_per_descriptor_pool = 256;
    frame_graph_descriptor.swapchain_attachment_name = "swapchain_attachment";
    frame_graph_descriptor.color_attachment_descriptors = color_attachment_descriptors.data();
    frame_graph_descriptor.color_attachment_descriptor_count = color_attachment_descriptors.size();
    frame_graph_descriptor.depth_stencil_attachment_descriptors = depth_stencil_attachment_descriptors.data();
    frame_graph_descriptor.depth_stencil_attachment_descriptor_count = depth_stencil_attachment_descriptors.size();
    frame_graph_descriptor.render_pass_descriptors = render_pass_descriptors.data();
    frame_graph_descriptor.render_pass_descriptor_count = render_pass_descriptors.size();

    UniquePtr<FrameGraph> frame_graph(FrameGraph::create_instance(frame_graph_descriptor), persistent_memory_resource);

    // TODO: Once Render can create graphics pipelines, these ugly call will be gone.
    material_manager.set_frame_graph(*frame_graph);

    // TODO: Once Render can create graphics pipelines, these ugly calls will be gone.
    opaque_shadow_render_pass.create_graphics_pipelines(*frame_graph);
    transcluent_shadow_render_pass.create_graphics_pipelines(*frame_graph);
    geometry_render_pass.create_graphics_pipelines(*frame_graph);
    lighting_render_pass.create_graphics_pipelines(*frame_graph);
    reflection_probe_render_pass.create_graphics_pipelines(*frame_graph);
    emission_render_pass.create_graphics_pipelines(*frame_graph);
    particle_system_render_pass.create_graphics_pipelines(*frame_graph);
    bloom_render_pass.create_graphics_pipelines(*frame_graph);
    tonemapping_render_pass.create_graphics_pipelines(*frame_graph);
    antialiasing_render_pass.create_graphics_pipelines(*frame_graph);
    debug_draw_render_pass.create_graphics_pipelines(*frame_graph);
    imgui_render_pass.create_graphics_pipelines(*frame_graph);

    scene.add_child(allocate_unique<ContainerPrimitive>(
        persistent_memory_resource, persistent_memory_resource, container_manager.load("resource/containers/ik.kwm")
    ));

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
        camera_controller.update();
        cpu_profiler_overlay.update();

        if (input.is_key_pressed(Scancode::RETURN)) {
            reflection_probe_manager.bake(*render, scene);
        }

        auto [animation_player_begin, animation_player_end] = animation_player.create_tasks();
        auto [particle_system_player_begin, particle_system_player_end] = particle_system_player.create_tasks();
        auto [texture_manager_begin, texture_manager_end] = texture_manager.create_tasks();
        auto [geometry_manager_begin, geometry_manager_end] = geometry_manager.create_tasks();
        MaterialManagerTasks material_manager_tasks = material_manager.create_tasks();
        auto [animation_manager_begin, animation_manager_end] = animation_manager.create_tasks();
        auto [particle_system_manager_begin, particle_system_manager_end] = particle_system_manager.create_tasks();
        auto [container_manager_begin, container_manager_end] = container_manager.create_tasks();
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
        Vector<Task*> bloom_render_pass_tasks = bloom_render_pass.create_tasks();
        Task* tonemapping_render_pass_task = tonemapping_render_pass.create_task();
        Task* antialiasing_render_pass_task = antialiasing_render_pass.create_task();
        Task* debug_draw_render_pass_task = debug_draw_render_pass.create_task();
        Task* imgui_render_pass_task = imgui_render_pass.create_task();
        Task* flush_task = render->create_task();

        animation_player_begin->add_input_dependencies(transient_memory_resource, { animation_manager_end });
        particle_system_player_begin->add_input_dependencies(transient_memory_resource, { particle_system_manager_end });
        reflection_probe_manager_begin->add_input_dependencies(transient_memory_resource, { acquire_frame_task });
        reflection_probe_manager_end->add_input_dependencies(transient_memory_resource, { reflection_probe_manager_begin, flush_task });
        animation_player_end->add_input_dependencies(transient_memory_resource, { animation_player_begin });
        particle_system_player_end->add_input_dependencies(transient_memory_resource, { particle_system_player_begin });
        material_manager_tasks.begin->add_input_dependencies(transient_memory_resource, { particle_system_manager_end, container_manager_end });
        material_manager_tasks.material_end->add_input_dependencies(transient_memory_resource, { material_manager_tasks.begin });
        material_manager_tasks.graphics_pipeline_end->add_input_dependencies(transient_memory_resource, { material_manager_tasks.material_end });
        texture_manager_begin->add_input_dependencies(transient_memory_resource, { material_manager_tasks.material_end, container_manager_end });
        texture_manager_end->add_input_dependencies(transient_memory_resource, { texture_manager_begin });
        geometry_manager_begin->add_input_dependencies(transient_memory_resource, { container_manager_end });
        geometry_manager_end->add_input_dependencies(transient_memory_resource, { geometry_manager_begin });
        animation_manager_begin->add_input_dependencies(transient_memory_resource, { container_manager_end });
        animation_manager_end->add_input_dependencies(transient_memory_resource, { animation_manager_begin });
        particle_system_manager_begin->add_input_dependencies(transient_memory_resource, { container_manager_end });
        particle_system_manager_end->add_input_dependencies(transient_memory_resource, { particle_system_manager_begin });
        container_manager_end->add_input_dependencies(transient_memory_resource, { container_manager_begin });
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
        antialiasing_render_pass_task->add_input_dependencies(transient_memory_resource, { acquire_frame_task });
        debug_draw_render_pass_task->add_input_dependencies(transient_memory_resource, { acquire_frame_task });
        imgui_render_pass_task->add_input_dependencies(transient_memory_resource, { acquire_frame_task });
        flush_task->add_input_dependencies(transient_memory_resource, {
            opaque_shadow_render_pass_task_end, transcluent_shadow_render_pass_task_end, geometry_render_pass_task,
            lighting_render_pass_task, reflection_probe_render_pass_task, emission_render_pass_task, particle_system_render_pass_task,
            tonemapping_render_pass_task, antialiasing_render_pass_task, debug_draw_render_pass_task, imgui_render_pass_task
        });
        present_frame_task->add_input_dependencies(transient_memory_resource, { acquire_frame_task, flush_task });

        for (Task* bloom_render_pass_task : bloom_render_pass_tasks) {
            bloom_render_pass_task->add_input_dependencies(transient_memory_resource, { acquire_frame_task });
            flush_task->add_input_dependencies(transient_memory_resource, { bloom_render_pass_task });
            task_scheduler.enqueue_task(transient_memory_resource, bloom_render_pass_task);
        }

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
        task_scheduler.enqueue_task(transient_memory_resource, container_manager_begin);
        task_scheduler.enqueue_task(transient_memory_resource, container_manager_end);
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
        task_scheduler.enqueue_task(transient_memory_resource, antialiasing_render_pass_task);
        task_scheduler.enqueue_task(transient_memory_resource, debug_draw_render_pass_task);
        task_scheduler.enqueue_task(transient_memory_resource, imgui_render_pass_task);
        task_scheduler.enqueue_task(transient_memory_resource, flush_task);
        task_scheduler.enqueue_task(transient_memory_resource, present_frame_task);

        task_scheduler.join();

        CpuProfiler::instance().update();
    }

    imgui_render_pass.destroy_graphics_pipelines(*frame_graph);
    debug_draw_render_pass.destroy_graphics_pipelines(*frame_graph);
    antialiasing_render_pass.destroy_graphics_pipelines(*frame_graph);
    tonemapping_render_pass.destroy_graphics_pipelines(*frame_graph);
    bloom_render_pass.destroy_graphics_pipelines(*frame_graph);
    particle_system_render_pass.destroy_graphics_pipelines(*frame_graph);
    emission_render_pass.destroy_graphics_pipelines(*frame_graph);
    reflection_probe_render_pass.destroy_graphics_pipelines(*frame_graph);
    lighting_render_pass.destroy_graphics_pipelines(*frame_graph);
    geometry_render_pass.destroy_graphics_pipelines(*frame_graph);
    transcluent_shadow_render_pass.destroy_graphics_pipelines(*frame_graph);
    opaque_shadow_render_pass.destroy_graphics_pipelines(*frame_graph);

    return 0;
}
