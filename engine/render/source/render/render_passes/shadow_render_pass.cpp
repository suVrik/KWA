#include "render/render_passes/shadow_render_pass.h"
#include "render/geometry/geometry.h"
#include "render/geometry/geometry_primitive.h"
#include "render/light/sphere_light_primitive.h"
#include "render/material/material.h"
#include "render/scene/scene.h"

#include <core/concurrency/task.h>
#include <core/concurrency/task_scheduler.h>
#include <core/containers/unordered_map.h>
#include <core/debug/assert.h>
#include <core/math/aabbox.h>
#include <core/math/float4x4.h>
#include <core/memory/memory_resource.h>

#include <algorithm>

namespace kw {

struct ShadowInstanceData {
    float4x4 model;
};

struct ShadowUniformBuffer {
    float4x4 joint_data[Material::MAX_JOINT_COUNT];
};

struct ShadowPushConstants {
    // `model_view_projection` for skinned geometry.
    float4x4 view_projection;
};

struct CubemapVectors {
    float3 direction;
    float3 up;
};

static CubemapVectors CUBEMAP_VECTORS[] = {
    { float3( 1.f,  0.f,  0.f), float3(0.f, 1.f,  0.f) },
    { float3(-1.f,  0.f,  0.f), float3(0.f, 1.f,  0.f) },
    { float3( 0.f,  1.f,  0.f), float3(0.f, 0.f, -1.f) },
    { float3( 0.f, -1.f,  0.f), float3(0.f, 0.f,  1.f) },
    { float3( 0.f,  0.f,  1.f), float3(0.f, 1.f,  0.f) },
    { float3( 0.f,  0.f, -1.f), float3(0.f, 1.f,  0.f) },
};

class ShadowRenderPass::WorkerTask : public Task {
public:
    WorkerTask(ShadowRenderPass& render_pass, uint32_t shadow_map_index, uint32_t face_index)
        : m_render_pass(render_pass)
        , m_shadow_map_index(shadow_map_index)
        , m_face_index(face_index)
    {
    }

    void run() override {
        //
        // TODO: Check whether this side was updated since the last frame.
        //  If any of the primitives or the light itself was updated.
        //

        RenderPassContext* context = m_render_pass.begin(m_shadow_map_index * 6 + m_face_index);
        if (context != nullptr) {
            float3 translation = m_render_pass.m_shadow_maps[m_shadow_map_index].light_primitive->get_global_translation();
            float4x4 view = float4x4::look_at_lh(translation, translation + CUBEMAP_VECTORS[m_face_index].direction, CUBEMAP_VECTORS[m_face_index].up);
            float4x4 projection = float4x4::perspective_lh(PI / 2.f, 1.f, 0.1f, 20.f);
            float4x4 view_projection = view * projection;

            Vector<GeometryPrimitive*> primitives = m_render_pass.m_scene.query_geometry(frustum(view_projection));

            // Sort primitives by geometry for instancing.
            std::sort(primitives.begin(), primitives.end(), GeometrySort());

            // MSVC is freaking out because of iterating past the end iterator.
            if (!primitives.empty()) {
                auto from_it = primitives.begin();
                for (auto to_it = ++primitives.begin(); to_it <= primitives.end(); ++to_it) {
                    if (to_it == primitives.end() ||
                        (*to_it)->get_geometry() != (*from_it)->get_geometry() ||
                        ((*from_it)->get_material() && (*from_it)->get_material()->is_skinned()))
                    {
                        if ((*from_it)->get_geometry() &&
                            (*from_it)->get_material() &&
                            (!(*from_it)->get_material()->is_skinned() || (*from_it)->get_geometry()->get_skinned_vertex_buffer() != nullptr))
                        {
                            Geometry& geometry = *(*from_it)->get_geometry();
                            Material& material = *(*from_it)->get_material();

                            VertexBuffer* vertex_buffers[2] = {
                                geometry.get_vertex_buffer(),
                                geometry.get_skinned_vertex_buffer(),
                            };
                            size_t vertex_buffer_count = material.is_skinned() ? 2 : 1;

                            IndexBuffer* index_buffer = geometry.get_index_buffer();
                            uint32_t index_count = geometry.get_index_count();

                            GraphicsPipeline* graphics_pipeline;

                            if (material.is_skinned()) {
                                graphics_pipeline = m_render_pass.m_skinned_graphics_pipeline;
                            } else {
                                graphics_pipeline = m_render_pass.m_solid_graphics_pipeline;
                            }

                            VertexBuffer* instance_buffer = nullptr;
                            size_t instance_buffer_count = 0;

                            if (!material.is_skinned()) {
                                Vector<ShadowInstanceData> instances_data(m_render_pass.m_transient_memory_resource);
                                instances_data.reserve(to_it - from_it);

                                for (auto it = from_it; it != to_it; ++it) {
                                    ShadowInstanceData instance_data;
                                    instance_data.model = float4x4((*it)->get_global_transform());
                                    instances_data.push_back(instance_data);
                                }

                                instance_buffer = context->get_render().acquire_transient_vertex_buffer(instances_data.data(), instances_data.size() * sizeof(ShadowInstanceData));
                                KW_ASSERT(instance_buffer != nullptr);

                                instance_buffer_count = 1;
                            }

                            UniformBuffer* uniform_buffer = nullptr;
                            size_t uniform_buffer_count = 0;

                            if (material.is_skinned()) {
                                Vector<float4x4> model_space_joint_matrices = (*from_it)->get_model_space_joint_matrices(m_render_pass.m_transient_memory_resource);

                                ShadowUniformBuffer uniform_data{};
                                std::copy(model_space_joint_matrices.begin(), model_space_joint_matrices.begin() + std::min(model_space_joint_matrices.size(), std::size(uniform_data.joint_data)), std::begin(uniform_data.joint_data));

                                uniform_buffer = context->get_render().acquire_transient_uniform_buffer(&uniform_data, sizeof(ShadowUniformBuffer));
                                KW_ASSERT(uniform_buffer != nullptr);

                                uniform_buffer_count = 1;
                            }

                            ShadowPushConstants push_constants{};
                            push_constants.view_projection = view_projection;

                            if (material.is_skinned()) {
                                // `view_projection` is `model_view_projection` for skinned geometry.
                                push_constants.view_projection = float4x4((*from_it)->get_global_transform()) * push_constants.view_projection;
                            }

                            DrawCallDescriptor draw_call_descriptor{};
                            draw_call_descriptor.graphics_pipeline = graphics_pipeline;
                            draw_call_descriptor.vertex_buffers = vertex_buffers;
                            draw_call_descriptor.vertex_buffer_count = vertex_buffer_count;
                            draw_call_descriptor.instance_buffers = &instance_buffer;
                            draw_call_descriptor.instance_buffer_count = instance_buffer_count;
                            draw_call_descriptor.index_buffer = index_buffer;
                            draw_call_descriptor.index_count = index_count;
                            draw_call_descriptor.instance_count = to_it - from_it;
                            draw_call_descriptor.uniform_buffers = &uniform_buffer;
                            draw_call_descriptor.uniform_buffer_count = uniform_buffer_count;
                            draw_call_descriptor.push_constants = &push_constants;
                            draw_call_descriptor.push_constants_size = sizeof(push_constants);

                            context->draw(draw_call_descriptor);
                        }

                        // MSVC is freaking out because of iterating past the end iterator.
                        if (to_it == primitives.end()) break;
                        else from_it = to_it;
                    }
                }
            }

            m_render_pass.blit("proxy_depth_attachment", m_render_pass.m_shadow_maps[m_shadow_map_index].texture, m_face_index, m_shadow_map_index * 6 + m_face_index);
        }
    }

    const char* get_name() const {
        return "Shadow Render Pass Worker";
    }

private:
    struct GeometrySort {
    public:
        bool operator()(GeometryPrimitive* a, GeometryPrimitive* b) const {
            if (a->get_geometry() == b->get_geometry()) {
                return a < b;
            }
            return a->get_geometry() < b->get_geometry();
        }
    };

    ShadowRenderPass& m_render_pass;
    uint32_t m_shadow_map_index;
    uint32_t m_face_index;
};

class ShadowRenderPass::BeginTask : public Task {
public:
    BeginTask(ShadowRenderPass& render_pass, Task* end_task)
        : m_render_pass(render_pass)
        , m_end_task(end_task)
    {
    }

    void run() override {
        //
        // Query all point lights in a frustum.
        //

        Vector<LightPrimitive*> primitives = m_render_pass.m_scene.query_lights(m_render_pass.m_scene.get_occlusion_camera().get_frustum());

        std::sort(primitives.begin(), primitives.end(), LightSort(m_render_pass.m_scene.get_camera()));
        
        Vector<SphereLightPrimitive*> shadow_lights(m_render_pass.m_transient_memory_resource);
        shadow_lights.reserve(m_render_pass.m_shadow_maps.size());

        for (uint32_t i = 0; i < primitives.size() && shadow_lights.size() < m_render_pass.m_shadow_maps.size(); i++) {
            if (SphereLightPrimitive* sphere_light_primitive = dynamic_cast<SphereLightPrimitive*>(primitives[i])) {
                if (sphere_light_primitive->is_shadow_enabled()) {
                    shadow_lights.push_back(sphere_light_primitive);
                }
            } else {
                KW_ASSERT(false, "Invalid light type.");
            }
        }

        //
        // Unlink shadow maps from light primitives that don't cast shadows this frame.
        //

        for (ShadowMap& shadow_map : m_render_pass.m_shadow_maps) {
            bool ok = false;
            for (SphereLightPrimitive*& sphere_light : shadow_lights) {
                if (shadow_map.light_primitive == sphere_light) {
                    std::swap(sphere_light, shadow_lights.back());
                    shadow_lights.pop_back();
                    ok = true;
                    break;
                }
            }
            if (!ok) {
                shadow_map.light_primitive = nullptr;
            }
        }

        //
        // Link new light primitives that cast shadows this frame.
        //

        for (SphereLightPrimitive* sphere_light : shadow_lights) {
            for (ShadowMap& shadow_map : m_render_pass.m_shadow_maps) {
                if (shadow_map.light_primitive == nullptr) {
                    //
                    // TODO: Shadow map have been reassigned, update all sides.
                    //

                    shadow_map.light_primitive = sphere_light;
                    break;
                }
            }
        }

        //
        // Redraw cubemap faces in different threads.
        //

        for (uint32_t shadow_map_index = 0; shadow_map_index < m_render_pass.m_shadow_maps.size(); shadow_map_index++) {
            if (m_render_pass.m_shadow_maps[shadow_map_index].light_primitive != nullptr) {
                for (uint32_t face_index = 0; face_index < 6; face_index++) {
                    WorkerTask* worker_task = new (m_render_pass.m_transient_memory_resource.allocate<WorkerTask>()) WorkerTask(m_render_pass, shadow_map_index, face_index);
                    KW_ASSERT(worker_task != nullptr);

                    worker_task->add_output_dependencies(m_render_pass.m_transient_memory_resource, { m_end_task });

                    m_render_pass.m_task_scheduler.enqueue_task(m_render_pass.m_transient_memory_resource, worker_task);
                }
            }
        }
    }

    const char* get_name() const {
        return "Shadow Render Pass Begin";
    }

private:
    struct LightSort {
    public:
        explicit LightSort(Camera& camera)
            : m_camera(camera)
        {
        }

        bool operator()(LightPrimitive* a, LightPrimitive* b) const {
            float distance_a = square_distance(a->get_global_translation(), m_camera.get_translation());
            float distance_b = square_distance(b->get_global_translation(), m_camera.get_translation());
            if (distance_a == distance_b) {
                return a < b;
            }
            return distance_a < distance_b;
        }

    private:
        Camera& m_camera;
    };

    ShadowRenderPass& m_render_pass;
    Task* m_end_task;
};

ShadowRenderPass::ShadowRenderPass(Render& render, Scene& scene, TaskScheduler& task_scheduler, MemoryResource& persistent_memory_resource, MemoryResource& transient_memory_resource)
    : m_render(render)
    , m_scene(scene)
    , m_task_scheduler(task_scheduler)
    , m_persistent_memory_resource(persistent_memory_resource)
    , m_transient_memory_resource(transient_memory_resource)
    , m_shadow_maps(persistent_memory_resource)
{
    m_shadow_maps.resize(3);

    for (ShadowMap& shadow_map : m_shadow_maps) {
        CreateTextureDescriptor create_texture_descriptor{};
        create_texture_descriptor.name = "shadow_texture";
        create_texture_descriptor.type = TextureType::TEXTURE_CUBE;
        create_texture_descriptor.format = TextureFormat::D16_UNORM;
        create_texture_descriptor.array_layer_count = 6;
        create_texture_descriptor.width = 512;
        create_texture_descriptor.height = 512;

        shadow_map.light_primitive = nullptr;
        shadow_map.texture = render.create_texture(create_texture_descriptor);
    }

    CreateTextureDescriptor create_texture_descriptor{};
    create_texture_descriptor.name = "dummy_shadow_texture";
    create_texture_descriptor.type = TextureType::TEXTURE_CUBE;
    create_texture_descriptor.format = TextureFormat::R16_UNORM;
    create_texture_descriptor.array_layer_count = 6;
    create_texture_descriptor.width = 1;
    create_texture_descriptor.height = 1;

    m_dummy_shadow_map = render.create_texture(create_texture_descriptor);

    uint16_t data[6];
    std::fill(std::begin(data), std::end(data), UINT16_MAX);

    UploadTextureDescriptor upload_texture_descriptor{};
    upload_texture_descriptor.texture = m_dummy_shadow_map;
    upload_texture_descriptor.data = data;
    upload_texture_descriptor.size = sizeof(data);
    upload_texture_descriptor.array_layer_count = 6;
    upload_texture_descriptor.width = 1;
    upload_texture_descriptor.height = 1;

    render.upload_texture(upload_texture_descriptor);

}

ShadowRenderPass::~ShadowRenderPass() {
    m_render.destroy_texture(m_dummy_shadow_map);

    for (ShadowMap& shadow_map : m_shadow_maps) {
        m_render.destroy_texture(shadow_map.texture);
    }
}

void ShadowRenderPass::get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // No color attachment descriptors.
}

void ShadowRenderPass::get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    AttachmentDescriptor depth_stencil_attachment_descriptor{};
    depth_stencil_attachment_descriptor.name = "proxy_depth_attachment";
    depth_stencil_attachment_descriptor.format = TextureFormat::D16_UNORM;
    depth_stencil_attachment_descriptor.size_class = SizeClass::ABSOLUTE;
    depth_stencil_attachment_descriptor.width = 512.f;
    depth_stencil_attachment_descriptor.height = 512.f;
    depth_stencil_attachment_descriptor.clear_depth = 1.f;
    depth_stencil_attachment_descriptor.is_blit_source = true;
    attachment_descriptors.push_back(depth_stencil_attachment_descriptor);
}

void ShadowRenderPass::get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) {
    RenderPassDescriptor render_pass_descriptor{};
    render_pass_descriptor.name = "shadow_render_pass";
    render_pass_descriptor.render_pass = this;
    render_pass_descriptor.write_depth_stencil_attachment_name = "proxy_depth_attachment";
    render_pass_descriptors.push_back(render_pass_descriptor);
}

void ShadowRenderPass::create_graphics_pipelines(FrameGraph& frame_graph) {
    AttributeDescriptor vertex_attribute_descriptors[4]{};
    vertex_attribute_descriptors[0].semantic = Semantic::POSITION;
    vertex_attribute_descriptors[0].format = TextureFormat::RGB32_FLOAT;
    vertex_attribute_descriptors[0].offset = offsetof(Geometry::Vertex, position);
    vertex_attribute_descriptors[1].semantic = Semantic::NORMAL;
    vertex_attribute_descriptors[1].format = TextureFormat::RGB32_FLOAT;
    vertex_attribute_descriptors[1].offset = offsetof(Geometry::Vertex, normal);
    vertex_attribute_descriptors[2].semantic = Semantic::TANGENT;
    vertex_attribute_descriptors[2].format = TextureFormat::RGBA32_FLOAT;
    vertex_attribute_descriptors[2].offset = offsetof(Geometry::Vertex, tangent);
    vertex_attribute_descriptors[3].semantic = Semantic::TEXCOORD;
    vertex_attribute_descriptors[3].format = TextureFormat::RG32_FLOAT;
    vertex_attribute_descriptors[3].offset = offsetof(Geometry::Vertex, texcoord_0);

    AttributeDescriptor joint_attribute_descriptors[2]{};
    joint_attribute_descriptors[0].semantic = Semantic::JOINTS;
    joint_attribute_descriptors[0].format = TextureFormat::RGBA8_UINT;
    joint_attribute_descriptors[0].offset = offsetof(Geometry::SkinnedVertex, joints);
    joint_attribute_descriptors[1].semantic = Semantic::WEIGHTS;
    joint_attribute_descriptors[1].format = TextureFormat::RGBA8_UNORM;
    joint_attribute_descriptors[1].offset = offsetof(Geometry::SkinnedVertex, weights);

    // Only the first binding will be used for solid geometry.
    BindingDescriptor binding_descriptors[2]{};
    binding_descriptors[0].attribute_descriptors = vertex_attribute_descriptors;
    binding_descriptors[0].attribute_descriptor_count = std::size(vertex_attribute_descriptors);
    binding_descriptors[0].stride = sizeof(Geometry::Vertex);
    binding_descriptors[1].attribute_descriptors = joint_attribute_descriptors;
    binding_descriptors[1].attribute_descriptor_count = std::size(joint_attribute_descriptors);
    binding_descriptors[1].stride = sizeof(Geometry::SkinnedVertex);

    AttributeDescriptor instance_attribute_descriptors[4]{};
    instance_attribute_descriptors[0].semantic = Semantic::POSITION;
    instance_attribute_descriptors[0].semantic_index = 1;
    instance_attribute_descriptors[0].format = TextureFormat::RGBA32_FLOAT;
    instance_attribute_descriptors[0].offset = offsetof(ShadowInstanceData, model) + offsetof(float4x4, _r0);
    instance_attribute_descriptors[1].semantic = Semantic::POSITION;
    instance_attribute_descriptors[1].semantic_index = 2;
    instance_attribute_descriptors[1].format = TextureFormat::RGBA32_FLOAT;
    instance_attribute_descriptors[1].offset = offsetof(ShadowInstanceData, model) + offsetof(float4x4, _r1);
    instance_attribute_descriptors[2].semantic = Semantic::POSITION;
    instance_attribute_descriptors[2].semantic_index = 3;
    instance_attribute_descriptors[2].format = TextureFormat::RGBA32_FLOAT;
    instance_attribute_descriptors[2].offset = offsetof(ShadowInstanceData, model) + offsetof(float4x4, _r2);
    instance_attribute_descriptors[3].semantic = Semantic::POSITION;
    instance_attribute_descriptors[3].semantic_index = 4;
    instance_attribute_descriptors[3].format = TextureFormat::RGBA32_FLOAT;
    instance_attribute_descriptors[3].offset = offsetof(ShadowInstanceData, model) + offsetof(float4x4, _r3);

    // Won't be used for skinned geometry.
    BindingDescriptor instance_binding_descriptor{};
    instance_binding_descriptor.attribute_descriptors = instance_attribute_descriptors;
    instance_binding_descriptor.attribute_descriptor_count = std::size(instance_attribute_descriptors);
    instance_binding_descriptor.stride = sizeof(ShadowInstanceData);

    // Won't be used for solid geometry.
    UniformBufferDescriptor uniform_buffer_descriptor{};
    uniform_buffer_descriptor.variable_name = "ShadowUniformBuffer";
    uniform_buffer_descriptor.size = sizeof(ShadowUniformBuffer);

    GraphicsPipelineDescriptor solid_graphics_pipeline_descriptor{};
    solid_graphics_pipeline_descriptor.graphics_pipeline_name = "shadow_solid_graphics_pipeline";
    solid_graphics_pipeline_descriptor.render_pass_name = "shadow_render_pass";
    solid_graphics_pipeline_descriptor.vertex_shader_filename = "resource/shaders/shadow_solid_vertex.hlsl";
    solid_graphics_pipeline_descriptor.vertex_binding_descriptors = binding_descriptors;
    solid_graphics_pipeline_descriptor.vertex_binding_descriptor_count = 1;
    solid_graphics_pipeline_descriptor.instance_binding_descriptors = &instance_binding_descriptor;
    solid_graphics_pipeline_descriptor.instance_binding_descriptor_count = 1;
    solid_graphics_pipeline_descriptor.front_face = FrontFace::CLOCKWISE;
    solid_graphics_pipeline_descriptor.depth_bias_constant_factor = 2.f;
    solid_graphics_pipeline_descriptor.depth_bias_clamp = 0.f;
    solid_graphics_pipeline_descriptor.depth_bias_slope_factor = 1.5f;
    solid_graphics_pipeline_descriptor.is_depth_test_enabled = true;
    solid_graphics_pipeline_descriptor.is_depth_write_enabled = true;
    solid_graphics_pipeline_descriptor.depth_compare_op = CompareOp::LESS;
    solid_graphics_pipeline_descriptor.push_constants_name = "shadow_push_constants";
    solid_graphics_pipeline_descriptor.push_constants_size = sizeof(ShadowPushConstants);

    m_solid_graphics_pipeline = frame_graph.create_graphics_pipeline(solid_graphics_pipeline_descriptor);

    GraphicsPipelineDescriptor skinned_graphics_pipeline_descriptor{};
    skinned_graphics_pipeline_descriptor.graphics_pipeline_name = "shadow_skinned_graphics_pipeline";
    skinned_graphics_pipeline_descriptor.render_pass_name = "shadow_render_pass";
    skinned_graphics_pipeline_descriptor.vertex_shader_filename = "resource/shaders/shadow_skinned_vertex.hlsl";
    skinned_graphics_pipeline_descriptor.vertex_binding_descriptors = binding_descriptors;
    skinned_graphics_pipeline_descriptor.vertex_binding_descriptor_count = 2;
    skinned_graphics_pipeline_descriptor.front_face = FrontFace::CLOCKWISE;
    skinned_graphics_pipeline_descriptor.depth_bias_constant_factor = 2.f;
    skinned_graphics_pipeline_descriptor.depth_bias_clamp = 0.f;
    skinned_graphics_pipeline_descriptor.depth_bias_slope_factor = 1.5f;
    skinned_graphics_pipeline_descriptor.is_depth_test_enabled = true;
    skinned_graphics_pipeline_descriptor.is_depth_write_enabled = true;
    skinned_graphics_pipeline_descriptor.depth_compare_op = CompareOp::LESS;
    skinned_graphics_pipeline_descriptor.uniform_buffer_descriptors = &uniform_buffer_descriptor;
    skinned_graphics_pipeline_descriptor.uniform_buffer_descriptor_count = 1;
    skinned_graphics_pipeline_descriptor.push_constants_name = "shadow_push_constants";
    skinned_graphics_pipeline_descriptor.push_constants_size = sizeof(ShadowPushConstants);

    m_skinned_graphics_pipeline = frame_graph.create_graphics_pipeline(skinned_graphics_pipeline_descriptor);
}

void ShadowRenderPass::destroy_graphics_pipelines(FrameGraph& frame_graph) {
    frame_graph.destroy_graphics_pipeline(m_skinned_graphics_pipeline);
    frame_graph.destroy_graphics_pipeline(m_solid_graphics_pipeline);
}

std::pair<Task*, Task*> ShadowRenderPass::create_tasks() {
    Task* end_task = new (m_transient_memory_resource.allocate<NoopTask>()) NoopTask("Shadow Render Pass End");
    Task* begin_task = new (m_transient_memory_resource.allocate<BeginTask>()) BeginTask(*this, end_task);

    return { begin_task, end_task };
}

const Vector<ShadowRenderPass::ShadowMap>& ShadowRenderPass::get_shadow_maps() const {
    return m_shadow_maps;
}

Texture* ShadowRenderPass::get_dummy_shadow_map() const {
    return m_dummy_shadow_map;
}

} // namespace kw
