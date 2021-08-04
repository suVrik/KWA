#define NOMINMAX

#include <Windows.h>

#undef ABSOLUTE
#undef DELETE
#undef IN
#undef OUT
#undef RELATIVE

#include <render/animation/animated_geometry_primitive.h>
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
#include <render/render.h>
#include <render/render_passes/debug_draw_render_pass.h>
#include <render/render_passes/geometry_render_pass.h>
#include <render/render_passes/imgui_render_pass.h>
#include <render/render_passes/lighting_render_pass.h>
#include <render/render_passes/shadow_render_pass.h>
#include <render/render_passes/tonemapping_render_pass.h>
#include <render/scene/scene.h>
#include <render/texture/texture_manager.h>

#include <system/event_loop.h>
#include <system/input.h>
#include <system/window.h>

#include <core/concurrency/task.h>
#include <core/concurrency/task_scheduler.h>
#include <core/debug/assert.h>
#include <core/debug/debug_utils.h>
#include <core/debug/log.h>
#include <core/error.h>
#include <core/math/float4x4.h>
#include <core/memory/linear_memory_resource.h>
#include <core/memory/malloc_memory_resource.h>

#include <fstream>

using namespace kw;

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/) {
    DebugUtils::subscribe_to_segfault();

    MallocMemoryResource& persistent_memory_resource = MallocMemoryResource::instance();
    LinearMemoryResource transient_memory_resource(persistent_memory_resource, 32 * 1024 * 1024);

    EventLoop event_loop;

    WindowDescriptor window_descriptor{};
    window_descriptor.title = "Render Example";
    window_descriptor.width = 1600;
    window_descriptor.height = 800;

    Window window(window_descriptor);

    Input input(window);

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

    std::unique_ptr<Render> render(Render::create_instance(render_descriptor));

    Scene scene(persistent_memory_resource, transient_memory_resource);
    DebugDrawManager debug_draw_manager(transient_memory_resource);
    ImguiManager imgui_manager(input, window, transient_memory_resource, persistent_memory_resource);
    TaskScheduler task_scheduler(persistent_memory_resource, 1);

    ShadowRenderPass shadow_render_pass(*render, scene, task_scheduler, persistent_memory_resource, transient_memory_resource);
    GeometryRenderPass geometry_render_pass(*render, scene, transient_memory_resource);
    LightingRenderPass lighting_render_pass(*render, scene, shadow_render_pass, transient_memory_resource);
    TonemappingRenderPass tonemapping_render_pass(*render, transient_memory_resource);
    DebugDrawRenderPass debug_draw_render_pass(*render, scene, debug_draw_manager, transient_memory_resource);
    ImguiRenderPass imgui_render_pass(*render, imgui_manager, transient_memory_resource);

    Vector<AttachmentDescriptor> color_attachment_descriptors(persistent_memory_resource);
    shadow_render_pass.get_color_attachment_descriptors(color_attachment_descriptors);
    geometry_render_pass.get_color_attachment_descriptors(color_attachment_descriptors);
    lighting_render_pass.get_color_attachment_descriptors(color_attachment_descriptors);
    tonemapping_render_pass.get_color_attachment_descriptors(color_attachment_descriptors);
    debug_draw_render_pass.get_color_attachment_descriptors(color_attachment_descriptors);
    imgui_render_pass.get_color_attachment_descriptors(color_attachment_descriptors);

    Vector<AttachmentDescriptor> depth_stencil_attachment_descriptors(persistent_memory_resource);
    shadow_render_pass.get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    geometry_render_pass.get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    lighting_render_pass.get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    tonemapping_render_pass.get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    debug_draw_render_pass.get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    imgui_render_pass.get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);

    Vector<RenderPassDescriptor> render_pass_descriptors(persistent_memory_resource);
    shadow_render_pass.get_render_pass_descriptors(render_pass_descriptors);
    geometry_render_pass.get_render_pass_descriptors(render_pass_descriptors);
    lighting_render_pass.get_render_pass_descriptors(render_pass_descriptors);
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

    std::unique_ptr<FrameGraph> frame_graph(FrameGraph::create_instance(frame_graph_descriptor));

    shadow_render_pass.create_graphics_pipelines(*frame_graph);
    geometry_render_pass.create_graphics_pipelines(*frame_graph);
    lighting_render_pass.create_graphics_pipelines(*frame_graph);
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

    std::ifstream level_stream("resource/levels/level1.txt");
    KW_ERROR(level_stream, "Failed to open level.");

    uint32_t prototype_count;
    uint32_t instance_count;
    level_stream >> prototype_count >> instance_count;

    UnorderedMap<String, std::pair<String, String>> prototypes(persistent_memory_resource);
    prototypes.reserve(prototype_count);

    for (uint32_t i = 0; i < prototype_count; i++) {
        String name(persistent_memory_resource);
        String geometry(persistent_memory_resource);
        String material(persistent_memory_resource);

        level_stream >> name;
        level_stream >> geometry;
        level_stream >> material;

        prototypes.emplace(std::move(name), std::pair<String, String>{ geometry, material });
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
            material_manager.load(it->second.second.c_str())
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

    AnimatedGeometryPrimitive robot_primitive(
        persistent_memory_resource,
        geometry_manager.load("resource/geometry/robot_blue.kwg"),
        material_manager.load("resource/materials/robot_blue.kwm")
    );
    robot_primitive.set_local_translation(float3(5.f, 0.f, 0.f));
    scene.add_child(robot_primitive);

    PointLightPrimitive point_light_primitives[3]{
        PointLightPrimitive(0.3f, true, float3(0.6f, 1.f, 1.f), 30.f, transform(float3(5.f, 4.f, 0.f))),
        PointLightPrimitive(0.3f, true, float3(0.6f, 1.f, 1.f), 30.f, transform(float3(5.f, 3.5f, 20.f))),
        PointLightPrimitive(0.3f, true, float3(0.6f, 1.f, 1.f), 30.f, transform(float3(5.f, 3.5f, -20.f))),
    };
    scene.add_child(point_light_primitives[0]);
    scene.add_child(point_light_primitives[1]);
    scene.add_child(point_light_primitives[2]);

    bool draw_light[std::size(point_light_primitives)]{};

    float camera_yaw = radians(60.f);
    float camera_pitch = radians(-20.f);
    float3 camera_position(6.f, 3.f, 5.f);
    float mouse_sensitivity = 0.0025f;
    float camera_speed = 0.2f;

    Camera& camera = scene.get_camera();
    camera.set_fov(radians(60.f));
    camera.set_z_near(0.05f);
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
        debug_draw_manager.update();
        imgui_manager.update();

        if (input.is_button_down(BUTTON_LEFT)) {
            camera_yaw -= input.get_mouse_dx() * mouse_sensitivity;
            camera_pitch -= input.get_mouse_dy() * mouse_sensitivity;
        }

        quaternion camera_rotation = quaternion::rotation(float3(0.f, 1.f, 0.f), camera_yaw) * quaternion::rotation(float3(1.f, 0.f, 0.f), camera_pitch);

        float3 forward = float3(0.f, 0.f, -1.f) * camera_rotation;
        float3 left = float3(-1.f, 0.f, 0.f) * camera_rotation;
        float3 up = float3(0.f, 1.f, 0.f);

        if (input.is_key_down(Scancode::W)) {
            camera_position += forward * camera_speed;
        }
        if (input.is_key_down(Scancode::A)) {
            camera_position += left * camera_speed;
        }
        if (input.is_key_down(Scancode::S)) {
            camera_position -= forward * camera_speed;
        }
        if (input.is_key_down(Scancode::D)) {
            camera_position -= left * camera_speed;
        }
        if (input.is_key_down(Scancode::Q)) {
            camera_position -= up * camera_speed;
        }
        if (input.is_key_down(Scancode::E)) {
            camera_position += up * camera_speed;
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
                    float light_radius = point_light_primitives[i].get_radius();
                    PointLightPrimitive::ShadowParams shadow_params = point_light_primitives[i].get_shadow_params();

                    imgui.DragFloat3("Light Position", &light_position, 0.01f);
                    imgui.ColorEdit3("Light Color", &light_color);
                    imgui.DragFloat("Light Power", &light_power, 0.01f, 0.f, FLT_MAX);
                    imgui.DragFloat("Light Radius", &light_radius, 0.01f, 0.f, 1.5f);
                    imgui.DragFloat("normal_bias", &shadow_params.normal_bias, 0.001, 0.f, FLT_MAX);
                    imgui.DragFloat("perspective_bias", &shadow_params.perspective_bias, 0.00001, 0.f, FLT_MAX, "%.6f");
                    imgui.DragFloat("pcss_radius_factor", &shadow_params.pcss_radius_factor, 0.1, 0.f, FLT_MAX);
                    imgui.DragFloat("pcss_filter_factor", &shadow_params.pcss_filter_factor, 0.01, 0.f, FLT_MAX);
                    imgui.Checkbox("Draw Light", &draw_light[i]);

                    point_light_primitives[i].set_global_translation(light_position);
                    point_light_primitives[i].set_color(light_color);
                    point_light_primitives[i].set_power(light_power);
                    point_light_primitives[i].set_radius(light_radius);
                    point_light_primitives[i].set_shadow_params(shadow_params);

                    if (draw_light[i]) {
                        debug_draw_manager.icosahedron(light_position, 0.01f, float3(1.f, 0.f, 0.f));
                        debug_draw_manager.icosahedron(light_position, light_radius, float3(1.f));
                    }
                }

                imgui.PopID();
            }
        }
        imgui.End();

        if (robot_primitive.get_geometry()) {
            const Skeleton* skeleton = robot_primitive.get_geometry()->get_skeleton();
            if (skeleton != nullptr) {
                SkeletonPose& skeleton_pose = robot_primitive.get_skeleton_pose();
                const Vector<float4x4>& joint_space_matrices = skeleton_pose.get_joint_space_matrices();
                size_t joint_count = skeleton->get_joint_count();

                if (joint_space_matrices.size() != joint_count) {
                    for (uint32_t i = 0; i < joint_count; i++) {
                        skeleton_pose.set_joint_space_matrix(i, skeleton->get_bind_matrix(i));
                    }
                    skeleton_pose.build_model_space_matrices(*skeleton);
                }

                if (imgui.Begin("Skinning")) {
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

        auto [texture_manager_begin, texture_manager_end] = texture_manager.create_tasks();
        auto [geometry_manager_begin, geometry_manager_end] = geometry_manager.create_tasks();
        MaterialManagerTasks material_manager_tasks = material_manager.create_tasks();
        auto [acquire_frame_task, present_frame_task] = frame_graph->create_tasks();
        auto [shadow_render_pass_task_begin, shadow_render_pass_task_end] = shadow_render_pass.create_tasks();
        Task* geometry_render_pass_task = geometry_render_pass.create_task();
        Task* lighting_render_pass_task = lighting_render_pass.create_task();
        Task* tonemapping_render_pass_task = tonemapping_render_pass.create_task();
        Task* debug_draw_render_pass_task = debug_draw_render_pass.create_task();
        Task* imgui_render_pass_task = imgui_render_pass.create_task();
        Task* flush_task = render->create_task();

        material_manager_tasks.material_end->add_input_dependencies(transient_memory_resource, { material_manager_tasks.begin });
        material_manager_tasks.graphics_pipeline_end->add_input_dependencies(transient_memory_resource, { material_manager_tasks.material_end });
        texture_manager_begin->add_input_dependencies(transient_memory_resource, { material_manager_tasks.material_end });
        texture_manager_end->add_input_dependencies(transient_memory_resource, { texture_manager_begin });
        geometry_manager_end->add_input_dependencies(transient_memory_resource, { geometry_manager_begin });
        acquire_frame_task->add_input_dependencies(transient_memory_resource, { material_manager_tasks.graphics_pipeline_end, texture_manager_end, geometry_manager_end });
        shadow_render_pass_task_begin->add_input_dependencies(transient_memory_resource, { acquire_frame_task });
        shadow_render_pass_task_end->add_input_dependencies(transient_memory_resource, { shadow_render_pass_task_begin });
        geometry_render_pass_task->add_input_dependencies(transient_memory_resource, { acquire_frame_task });
        lighting_render_pass_task->add_input_dependencies(transient_memory_resource, { acquire_frame_task });
        tonemapping_render_pass_task->add_input_dependencies(transient_memory_resource, { acquire_frame_task });
        debug_draw_render_pass_task->add_input_dependencies(transient_memory_resource, { acquire_frame_task });
        imgui_render_pass_task->add_input_dependencies(transient_memory_resource, { acquire_frame_task });
        flush_task->add_input_dependencies(transient_memory_resource, { shadow_render_pass_task_end, geometry_render_pass_task, lighting_render_pass_task, tonemapping_render_pass_task, debug_draw_render_pass_task, imgui_render_pass_task });
        present_frame_task->add_input_dependencies(transient_memory_resource, { flush_task });

        task_scheduler.enqueue_task(transient_memory_resource, material_manager_tasks.begin);
        task_scheduler.enqueue_task(transient_memory_resource, material_manager_tasks.material_end);
        task_scheduler.enqueue_task(transient_memory_resource, material_manager_tasks.graphics_pipeline_end);
        task_scheduler.enqueue_task(transient_memory_resource, texture_manager_begin);
        task_scheduler.enqueue_task(transient_memory_resource, texture_manager_end);
        task_scheduler.enqueue_task(transient_memory_resource, geometry_manager_begin);
        task_scheduler.enqueue_task(transient_memory_resource, geometry_manager_end);
        task_scheduler.enqueue_task(transient_memory_resource, acquire_frame_task);
        task_scheduler.enqueue_task(transient_memory_resource, shadow_render_pass_task_begin);
        task_scheduler.enqueue_task(transient_memory_resource, shadow_render_pass_task_end);
        task_scheduler.enqueue_task(transient_memory_resource, geometry_render_pass_task);
        task_scheduler.enqueue_task(transient_memory_resource, lighting_render_pass_task);
        task_scheduler.enqueue_task(transient_memory_resource, tonemapping_render_pass_task);
        task_scheduler.enqueue_task(transient_memory_resource, debug_draw_render_pass_task);
        task_scheduler.enqueue_task(transient_memory_resource, imgui_render_pass_task);
        task_scheduler.enqueue_task(transient_memory_resource, flush_task);
        task_scheduler.enqueue_task(transient_memory_resource, present_frame_task);

        task_scheduler.join();
    }
    
    imgui_render_pass.destroy_graphics_pipelines(*frame_graph);
    debug_draw_render_pass.destroy_graphics_pipelines(*frame_graph);
    tonemapping_render_pass.destroy_graphics_pipelines(*frame_graph);
    lighting_render_pass.destroy_graphics_pipelines(*frame_graph);
    geometry_render_pass.destroy_graphics_pipelines(*frame_graph);
    shadow_render_pass.destroy_graphics_pipelines(*frame_graph);

    return 0;
}
