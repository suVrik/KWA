#include "render/render_passes/opaque_shadow_render_pass.h"
#include "render/geometry/geometry.h"
#include "render/geometry/geometry_primitive.h"
#include "render/light/light_primitive.h"
#include "render/material/material.h"
#include "render/scene/scene.h"
#include "render/shadow/shadow_manager.h"

#include <core/concurrency/task.h>
#include <core/concurrency/task_scheduler.h>
#include <core/containers/unordered_map.h>
#include <core/debug/assert.h>
#include <core/debug/cpu_profiler.h>
#include <core/math/aabbox.h>
#include <core/math/float4x4.h>
#include <core/math/frustum.h>
#include <core/memory/memory_resource.h>

#include <algorithm>

namespace kw {

// TODO: Share across `ReflectionProbeManager`, `OpaqueShadowRenderPass` and `TranslucentShadowRenderPass`.
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

class OpaqueShadowRenderPass::WorkerTask : public Task {
public:
    WorkerTask(OpaqueShadowRenderPass& render_pass, uint32_t shadow_map_index, uint32_t face_index)
        : m_render_pass(render_pass)
        , m_shadow_map_index(shadow_map_index)
        , m_face_index(face_index)
    {
    }

    void run() override {
        ShadowMap& shadow_map = m_render_pass.m_shadow_manager.get_shadow_maps()[m_shadow_map_index];

        float3 translation = shadow_map.light_primitive->get_global_translation();
        float4x4 view = float4x4::look_at_lh(translation, translation + CUBEMAP_VECTORS[m_face_index].direction, CUBEMAP_VECTORS[m_face_index].up);
        float4x4 projection = float4x4::perspective_lh(PI / 2.f, 1.f, 0.1f, 20.f);
        float4x4 view_projection = view * projection;

        Vector<GeometryPrimitive*> primitives(m_render_pass.m_transient_memory_resource);

        {
            KW_CPU_PROFILER("Occlusion Culling");

            primitives = m_render_pass.m_scene.query_geometry(frustum(view_projection));
        }

        // Sort primitives by geometry for instancing.
        {
            KW_CPU_PROFILER("Primitive Sort");

            std::sort(primitives.begin(), primitives.end(), GeometrySort());
        }

        // Find the most recent updated primitive.
        uint64_t max_counter = shadow_map.light_primitive->get_counter();
        for (GeometryPrimitive* geometry_primitive : primitives) {
            max_counter = std::max(max_counter, geometry_primitive->get_counter());
        }

        if (shadow_map.depth_max_counter[m_face_index] == max_counter &&
            shadow_map.depth_primitive_count[m_face_index] == primitives.size())
        {
            // No primitives have been added or removed. No primitives have been updated. No need to re-render the side.
            return;
        }

        RenderPassContext* context = m_render_pass.begin(m_shadow_map_index * 6 + m_face_index);
        if (context != nullptr) {
            // MSVC is freaking out because of iterating past the end iterator.
            if (!primitives.empty()) {
                auto from_it = primitives.begin();
                for (auto to_it = ++primitives.begin(); to_it <= primitives.end(); ++to_it) {
                    if (to_it == primitives.end() ||
                        (*to_it)->get_geometry() != (*from_it)->get_geometry() ||
                        ((*from_it)->get_material() && (*from_it)->get_material()->is_skinned()))
                    {
                        SharedPtr<Geometry> geometry = (*from_it)->get_geometry();
                        SharedPtr<Material> material = (*from_it)->get_shadow_material();
                        
                        if (geometry && geometry->is_loaded() && material && material->is_loaded() &&
                            (!material->is_skinned() || geometry->get_skinned_vertex_buffer() != nullptr))
                        {
                            KW_ASSERT(
                                material->is_shadow() && material->is_geometry(),
                                "Invalid geometry primitive shadow material."
                            );

                            VertexBuffer* vertex_buffers[2] = {
                                geometry->get_vertex_buffer(),
                                geometry->get_skinned_vertex_buffer(),
                            };
                            size_t vertex_buffer_count = material->is_skinned() ? 2 : 1;

                            IndexBuffer* index_buffer = geometry->get_index_buffer();
                            uint32_t index_count = geometry->get_index_count();

                            VertexBuffer* instance_buffer = nullptr;
                            size_t instance_buffer_count = 0;

                            if (!material->is_skinned()) {
                                Vector<Material::ShadowInstanceData> instances_data(m_render_pass.m_transient_memory_resource);
                                instances_data.reserve(to_it - from_it);

                                for (auto it = from_it; it != to_it; ++it) {
                                    Material::ShadowInstanceData instance_data;
                                    instance_data.model = float4x4((*it)->get_global_transform());
                                    instances_data.push_back(instance_data);
                                }

                                instance_buffer = context->get_render().acquire_transient_vertex_buffer(instances_data.data(), instances_data.size() * sizeof(Material::ShadowInstanceData));
                                KW_ASSERT(instance_buffer != nullptr);

                                instance_buffer_count = 1;
                            }

                            UniformBuffer* uniform_buffer = nullptr;
                            size_t uniform_buffer_count = 0;

                            if (material->is_skinned()) {
                                Vector<float4x4> model_space_joint_matrices = (*from_it)->get_model_space_joint_matrices(m_render_pass.m_transient_memory_resource);

                                Material::ShadowUniformData uniform_data{};
                                std::copy(model_space_joint_matrices.begin(), model_space_joint_matrices.begin() + std::min(model_space_joint_matrices.size(), std::size(uniform_data.joint_data)), std::begin(uniform_data.joint_data));

                                uniform_buffer = context->get_render().acquire_transient_uniform_buffer(&uniform_data, sizeof(Material::ShadowUniformData));
                                KW_ASSERT(uniform_buffer != nullptr);

                                uniform_buffer_count = 1;
                            }

                            Material::ShadowPushConstants push_constants{};
                            push_constants.view_projection = view_projection;

                            if (material->is_skinned()) {
                                // `view_projection` is `model_view_projection` for skinned geometry.
                                push_constants.view_projection = float4x4((*from_it)->get_global_transform()) * push_constants.view_projection;
                            }

                            DrawCallDescriptor draw_call_descriptor{};
                            draw_call_descriptor.graphics_pipeline = *material->get_graphics_pipeline();
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

                            {
                                KW_CPU_PROFILER("Draw Call");

                                context->draw(draw_call_descriptor);
                            }
                        }

                        // MSVC is freaking out because of iterating past the end iterator.
                        if (to_it == primitives.end()) break;
                        else from_it = to_it;
                    }
                }
            }

            m_render_pass.blit("proxy_depth_attachment", shadow_map.depth_texture, 0, m_face_index, m_shadow_map_index * 6 + m_face_index);

            shadow_map.depth_max_counter[m_face_index] = max_counter;
            shadow_map.depth_primitive_count[m_face_index] = primitives.size();
        }
    }

    const char* get_name() const override {
        return "Opaque Shadow Render Pass Worker";
    }

private:
    struct GeometrySort {
    public:
        bool operator()(GeometryPrimitive* a, GeometryPrimitive* b) const {
            if (a->get_material()->get_graphics_pipeline() == b->get_material()->get_graphics_pipeline()) {
                if (a->get_material() == b->get_material()) {
                    if (a->get_geometry() == b->get_geometry()) {
                        return a < b;
                    }
                    return a->get_geometry() < b->get_geometry();
                }
                return a->get_material() < b->get_material();
            }
            return a->get_material()->get_graphics_pipeline() < b->get_material()->get_graphics_pipeline();
        }
    };

    OpaqueShadowRenderPass& m_render_pass;
    uint32_t m_shadow_map_index;
    uint32_t m_face_index;
};

class OpaqueShadowRenderPass::BeginTask : public Task {
public:
    BeginTask(OpaqueShadowRenderPass& render_pass, Task* end_task)
        : m_render_pass(render_pass)
        , m_end_task(end_task)
    {
    }

    void run() override {
        const Vector<ShadowMap>& shadow_maps = m_render_pass.m_shadow_manager.get_shadow_maps();
        for (uint32_t shadow_map_index = 0; shadow_map_index < shadow_maps.size(); shadow_map_index++) {
            if (shadow_maps[shadow_map_index].light_primitive != nullptr) {
                for (uint32_t face_index = 0; face_index < 6; face_index++) {
                    WorkerTask* worker_task = m_render_pass.m_transient_memory_resource.construct<WorkerTask>(m_render_pass, shadow_map_index, face_index);
                    KW_ASSERT(worker_task != nullptr);

                    worker_task->add_output_dependencies(m_render_pass.m_transient_memory_resource, { m_end_task });

                    m_render_pass.m_task_scheduler.enqueue_task(m_render_pass.m_transient_memory_resource, worker_task);
                }
            }
        }
    }

    const char* get_name() const override {
        return "Opaque Shadow Render Pass Begin";
    }

private:
    OpaqueShadowRenderPass& m_render_pass;
    Task* m_end_task;
};

OpaqueShadowRenderPass::OpaqueShadowRenderPass(const OpaqueShadowRenderPassDescriptor& descriptor)
    : m_scene(*descriptor.scene)
    , m_shadow_manager(*descriptor.shadow_manager)
    , m_task_scheduler(*descriptor.task_scheduler)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
{
    KW_ASSERT(descriptor.scene != nullptr);
    KW_ASSERT(descriptor.shadow_manager != nullptr);
    KW_ASSERT(descriptor.task_scheduler != nullptr);
    KW_ASSERT(descriptor.transient_memory_resource != nullptr);
}

void OpaqueShadowRenderPass::get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // No color attachment descriptors.
}

void OpaqueShadowRenderPass::get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    float shadow_map_dimension = static_cast<float>(m_shadow_manager.get_shadow_map_dimension());

    AttachmentDescriptor depth_stencil_attachment_descriptor{};
    depth_stencil_attachment_descriptor.name = "proxy_depth_attachment";
    depth_stencil_attachment_descriptor.format = TextureFormat::D16_UNORM;
    depth_stencil_attachment_descriptor.size_class = SizeClass::ABSOLUTE;
    depth_stencil_attachment_descriptor.width = shadow_map_dimension;
    depth_stencil_attachment_descriptor.height = shadow_map_dimension;
    depth_stencil_attachment_descriptor.clear_depth = 1.f;
    depth_stencil_attachment_descriptor.is_blit_source = true;
    attachment_descriptors.push_back(depth_stencil_attachment_descriptor);
}

void OpaqueShadowRenderPass::get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) {
    RenderPassDescriptor render_pass_descriptor{};
    render_pass_descriptor.name = "opaque_shadow_render_pass";
    render_pass_descriptor.render_pass = this;
    render_pass_descriptor.write_depth_stencil_attachment_name = "proxy_depth_attachment";
    render_pass_descriptors.push_back(render_pass_descriptor);
}

void OpaqueShadowRenderPass::create_graphics_pipelines(FrameGraph& frame_graph) {
    // All graphics pipelines are stored in geometry primirives' materials.
}

void OpaqueShadowRenderPass::destroy_graphics_pipelines(FrameGraph& frame_graph) {
    // All opaque shadow graphics pipelines are destroyed by material manager.
}

Pair<Task*, Task*> OpaqueShadowRenderPass::create_tasks() {
    Task* end_task = m_transient_memory_resource.construct<NoopTask>("Opaque Shadow Render Pass End");
    Task* begin_task = m_transient_memory_resource.construct<BeginTask>(*this, end_task);

    return { begin_task, end_task };
}

} // namespace kw
