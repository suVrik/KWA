#include "render/render_passes/geometry_render_pass.h"
#include "render/camera/camera_manager.h"
#include "render/geometry/geometry.h"
#include "render/geometry/geometry_primitive.h"
#include "render/material/material.h"
#include "render/scene/scene.h"

#include <core/concurrency/task.h>
#include <core/debug/assert.h>
#include <core/debug/cpu_profiler.h>

#include <algorithm>

namespace kw {

class GeometryRenderPass::Task : public kw::Task {
public:
    Task(GeometryRenderPass& render_pass)
        : m_render_pass(render_pass)
    {
    }

    void run() override {
        RenderPassContext* context = m_render_pass.begin();
        if (context != nullptr) {
            Vector<GeometryPrimitive*> primitives(m_render_pass.m_transient_memory_resource);

            {
                KW_CPU_PROFILER("Occlusion Culling");

                primitives = m_render_pass.m_scene.query_geometry(m_render_pass.m_camera_manager.get_occlusion_camera().get_frustum());
            }

            // Sort primitives by graphics pipeline (to avoid graphics pipeline switches),
            // by material (to avoid rebinding uniform data), by geometry (for instancing).
            {
                KW_CPU_PROFILER("Primitive Sort");

                std::sort(primitives.begin(), primitives.end(), GeometrySort());
            }

            // MSVC is freaking out because of iterating past the end iterator.
            if (primitives.empty()) return;

            auto from_it = primitives.begin();
            for (auto to_it = ++primitives.begin(); to_it <= primitives.end(); ++to_it) {
                if (to_it == primitives.end() ||
                    (*to_it)->get_geometry() != (*from_it)->get_geometry() ||
                    (*to_it)->get_material() != (*from_it)->get_material() ||
                    ((*from_it)->get_material() && (*from_it)->get_material()->is_skinned()))
                {
                    SharedPtr<Geometry> geometry = (*from_it)->get_geometry();
                    SharedPtr<Material> material = (*from_it)->get_material();

                    if (geometry && geometry->is_loaded() && material && material->is_loaded() &&
                        (!material->is_skinned() || geometry->get_skinned_vertex_buffer() != nullptr))
                    {
                        KW_ASSERT(
                            !material->is_shadow() && material->is_geometry(),
                            "Invalid geometry primitive material."
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
                            Vector<Material::GeometryInstanceData> instances_data(m_render_pass.m_transient_memory_resource);
                            instances_data.reserve(to_it - from_it);

                            for (auto it = from_it; it != to_it; ++it) {
                                Material::GeometryInstanceData instance_data;
                                instance_data.model = float4x4((*it)->get_global_transform());
                                instance_data.inverse_transpose_model = transpose(inverse(instance_data.model));
                                instances_data.push_back(instance_data);
                            }

                            instance_buffer = context->get_render().acquire_transient_vertex_buffer(instances_data.data(), instances_data.size() * sizeof(Material::GeometryInstanceData));
                            KW_ASSERT(instance_buffer != nullptr);

                            instance_buffer_count = 1;
                        }

                        Vector<Texture*> uniform_textures(m_render_pass.m_transient_memory_resource);
                        uniform_textures.reserve(material->get_textures().size());

                        for (const SharedPtr<Texture*>& texture : material->get_textures()) {
                            KW_ASSERT(texture && *texture != nullptr);
                            uniform_textures.push_back(*texture);
                        }

                        UniformBuffer* uniform_buffer = nullptr;
                        size_t uniform_buffer_count = 0;

                        if (material->is_skinned()) {
                            Vector<float4x4> model_space_joint_matrices = (*from_it)->get_model_space_joint_matrices(m_render_pass.m_transient_memory_resource);

                            Material::UniformData uniform_data{};
                            uniform_data.model = float4x4((*from_it)->get_global_transform());
                            uniform_data.inverse_transpose_model = transpose(inverse(uniform_data.model));
                            std::copy(model_space_joint_matrices.begin(), model_space_joint_matrices.begin() + std::min(model_space_joint_matrices.size(), std::size(uniform_data.joint_data)), std::begin(uniform_data.joint_data));

                            uniform_buffer = context->get_render().acquire_transient_uniform_buffer(&uniform_data, sizeof(Material::UniformData));
                            KW_ASSERT(uniform_buffer != nullptr);

                            uniform_buffer_count = 1;
                        }

                        Material::GeometryPushConstants geometry_push_constants{};
                        geometry_push_constants.view_projection = m_render_pass.m_camera_manager.get_camera().get_view_projection_matrix();

                        DrawCallDescriptor draw_call_descriptor{};
                        draw_call_descriptor.graphics_pipeline = *material->get_graphics_pipeline();
                        draw_call_descriptor.vertex_buffers = vertex_buffers;
                        draw_call_descriptor.vertex_buffer_count = vertex_buffer_count;
                        draw_call_descriptor.instance_buffers = &instance_buffer;
                        draw_call_descriptor.instance_buffer_count = instance_buffer_count;
                        draw_call_descriptor.index_buffer = index_buffer;
                        draw_call_descriptor.index_count = index_count;
                        draw_call_descriptor.instance_count = to_it - from_it;
                        draw_call_descriptor.stencil_reference = 0xFF;
                        draw_call_descriptor.uniform_textures = uniform_textures.data();
                        draw_call_descriptor.uniform_texture_count = uniform_textures.size();
                        draw_call_descriptor.uniform_buffers = &uniform_buffer;
                        draw_call_descriptor.uniform_buffer_count = uniform_buffer_count;
                        draw_call_descriptor.push_constants = &geometry_push_constants;
                        draw_call_descriptor.push_constants_size = sizeof(geometry_push_constants);

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
    }

    const char* get_name() const override {
        return "Geometry Render Pass";
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

    GeometryRenderPass& m_render_pass;
};

GeometryRenderPass::GeometryRenderPass(const GeometryRenderPassDescriptor& descriptor)
    : m_scene(*descriptor.scene)
    , m_camera_manager(*descriptor.camera_manager)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
{
    KW_ASSERT(descriptor.scene != nullptr);
    KW_ASSERT(descriptor.camera_manager != nullptr);
    KW_ASSERT(descriptor.transient_memory_resource != nullptr);
}

void GeometryRenderPass::get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    attachment_descriptors.push_back(AttachmentDescriptor{ "albedo_metalness_attachment", TextureFormat::RGBA8_UNORM,  LoadOp::DONT_CARE });
    attachment_descriptors.push_back(AttachmentDescriptor{ "normal_roughness_attachment", TextureFormat::RGBA16_SNORM, LoadOp::DONT_CARE });
    attachment_descriptors.push_back(AttachmentDescriptor{ "emission_ao_attachment", TextureFormat::RGBA8_UNORM, LoadOp::DONT_CARE });
}

void GeometryRenderPass::get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    AttachmentDescriptor depth_stencil_attachment_descriptor{};
    depth_stencil_attachment_descriptor.name = "depth_attachment";
    depth_stencil_attachment_descriptor.format = TextureFormat::D24_UNORM_S8_UINT;
    depth_stencil_attachment_descriptor.clear_depth = 1.f;
    attachment_descriptors.push_back(depth_stencil_attachment_descriptor);
}

void GeometryRenderPass::get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) {
    static const char* const COLOR_ATTACHMENT_NAMES[] = {
        "albedo_metalness_attachment",
        "normal_roughness_attachment",
        "emission_ao_attachment",
    };

    RenderPassDescriptor render_pass_descriptor{};
    render_pass_descriptor.name = "geometry_render_pass";
    render_pass_descriptor.render_pass = this;
    render_pass_descriptor.write_color_attachment_names = COLOR_ATTACHMENT_NAMES;
    render_pass_descriptor.write_color_attachment_name_count = std::size(COLOR_ATTACHMENT_NAMES);
    render_pass_descriptor.write_depth_stencil_attachment_name = "depth_attachment";
    render_pass_descriptors.push_back(render_pass_descriptor);
}

void GeometryRenderPass::create_graphics_pipelines(FrameGraph& frame_graph) {
    // All graphics pipelines are stored in geometry primirives' materials.
}

void GeometryRenderPass::destroy_graphics_pipelines(FrameGraph& frame_graph) {
    // All geometry graphics pipelines are destroyed by material manager.
}

Task* GeometryRenderPass::create_task() {
    return m_transient_memory_resource.construct<Task>(*this);
}

} // namespace kw
