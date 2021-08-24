#include "render/render_passes/particle_system_render_pass.h"
#include "render/camera/camera_manager.h"
#include "render/geometry/geometry.h"
#include "render/material/material.h"
#include "render/particles/particle_system.h"
#include "render/particles/particle_system_primitive.h"
#include "render/scene/scene.h"

#include <core/concurrency/task.h>
#include <core/debug/assert.h>
#include <core/debug/cpu_profiler.h>

#include <algorithm>

namespace kw {

class ParticleSystemRenderPass::Task : public kw::Task {
public:
    Task(ParticleSystemRenderPass& render_pass)
        : m_render_pass(render_pass)
    {
    }

    void run() override {
        RenderPassContext* context = m_render_pass.begin();
        if (context != nullptr) {
            Vector<ParticleSystemPrimitive*> primitives(m_render_pass.m_transient_memory_resource);

            {
                KW_CPU_PROFILER("Occlusion Culling");

                primitives = m_render_pass.m_scene.query_particle_systems(m_render_pass.m_camera_manager.get_occlusion_camera().get_frustum());
            }

            Camera& camera = m_render_pass.m_camera_manager.get_camera();

            {
                KW_CPU_PROFILER("Primitive Sort");

                std::sort(primitives.begin(), primitives.end(), ParticleSystemSort(camera));
            }

            for (ParticleSystemPrimitive* primitive : primitives) {
                SharedPtr<ParticleSystem> particle_system = primitive->get_particle_system();
                if (particle_system && particle_system->is_loaded() && primitive->get_particle_count() > 0) {
                    SharedPtr<Geometry> geometry = particle_system->get_geometry();
                    SharedPtr<Material> material = particle_system->get_material();
                    if (geometry && geometry->is_loaded() && material && material->is_loaded() && material->is_particle()) {
                        uint32_t spritesheet_x = particle_system->get_spritesheet_x();
                        uint32_t spritesheet_y = particle_system->get_spritesheet_y();

                        VertexBuffer* vertex_buffer = geometry->get_vertex_buffer();
                        IndexBuffer* index_buffer = geometry->get_index_buffer();
                        uint32_t index_count = geometry->get_index_count();
                        uint32_t instance_count = primitive->get_particle_count();

                        Vector<Texture*> uniform_textures(m_render_pass.m_transient_memory_resource);
                        uniform_textures.reserve(material->get_textures().size());

                        for (const SharedPtr<Texture*>& texture : material->get_textures()) {
                            KW_ASSERT(texture && *texture != nullptr);
                            uniform_textures.push_back(*texture);
                        }

                        Material::ParticlePushConstants push_constants{};
                        push_constants.view_projection = camera.get_view_projection_matrix();
                        push_constants.uv_scale = float4(1.f / spritesheet_x, 1.f / spritesheet_y, 0.f, 0.f);

                        float* position_x_stream = primitive->get_particle_system_stream(ParticleSystemStream::POSITION_X);
                        float* position_y_stream = primitive->get_particle_system_stream(ParticleSystemStream::POSITION_Y);
                        float* position_z_stream = primitive->get_particle_system_stream(ParticleSystemStream::POSITION_Z);

                        float* generated_scale_x_stream = primitive->get_particle_system_stream(ParticleSystemStream::GENERATED_SCALE_X);
                        float* generated_scale_y_stream = primitive->get_particle_system_stream(ParticleSystemStream::GENERATED_SCALE_Y);
                        float* generated_scale_z_stream = primitive->get_particle_system_stream(ParticleSystemStream::GENERATED_SCALE_Z);
                        
                        float* scale_x_stream = primitive->get_particle_system_stream(ParticleSystemStream::SCALE_X);
                        float* scale_y_stream = primitive->get_particle_system_stream(ParticleSystemStream::SCALE_Y);
                        float* scale_z_stream = primitive->get_particle_system_stream(ParticleSystemStream::SCALE_Z);

                        float* color_r_stream = primitive->get_particle_system_stream(ParticleSystemStream::COLOR_R);
                        float* color_g_stream = primitive->get_particle_system_stream(ParticleSystemStream::COLOR_G);
                        float* color_b_stream = primitive->get_particle_system_stream(ParticleSystemStream::COLOR_B);
                        float* color_a_stream = primitive->get_particle_system_stream(ParticleSystemStream::COLOR_A);

                        float* frame_stream = primitive->get_particle_system_stream(ParticleSystemStream::FRAME);
                    
                        Vector<Material::ParticleInstanceData> instance_data(instance_count, m_render_pass.m_transient_memory_resource);

                        for (uint32_t i = 0; i < instance_count; i++) {
                            Material::ParticleInstanceData& instance = instance_data[i];

                            float position_x = 0.f;
                            float position_y = 0.f;
                            float position_z = 0.f;

                            if (position_x_stream != nullptr) {
                                position_x = position_x_stream[i];
                            }

                            if (position_y_stream != nullptr) {
                                position_y = position_y_stream[i];
                            }

                            if (position_z_stream != nullptr) {
                                position_z = position_z_stream[i];
                            }

                            auto look_at = [](const float3& source, const float3& target, const float3& up) {
                                float3 f(normalize(target - source));
                                float3 s(normalize(cross(up, f)));
                                float3 u(cross(f, s));

                                return float4x4(s.x,      s.y,      s.z,      0.f,
                                                u.x,      u.y,      u.z,      0.f,
                                                f.x,      f.y,      f.z,      0.f,
                                                source.x, source.y, source.z, 1.f);
                            };

                            switch (particle_system->get_axes()) {
                            case ParticleSystemAxes::NONE:
                                instance.model = float4x4::translation(float3(position_x, position_y, position_z));
                                break;
                            case ParticleSystemAxes::Y:
                                instance.model = look_at(float3(position_x, position_y, position_z), float3(camera.get_translation().x, position_y, camera.get_translation().z), float3(0.f, 1.f, 0.f));
                                break;
                            case ParticleSystemAxes::YZ:
                                instance.model = look_at(float3(position_x, position_y, position_z), camera.get_translation(), float3(0.f, 1.f, 0.f));
                                break;
                            };
                            
                            float scale_x = 1.f;
                            float scale_y = 1.f;
                            float scale_z = 1.f;

                            if (generated_scale_x_stream != nullptr) {
                                scale_x = generated_scale_x_stream[i];
                            }

                            if (generated_scale_y_stream != nullptr) {
                                scale_y = generated_scale_y_stream[i];
                            }

                            if (generated_scale_z_stream != nullptr) {
                                scale_z = generated_scale_z_stream[i];
                            }

                            if (scale_x_stream != nullptr) {
                                scale_x *= scale_x_stream[i];
                            }

                            if (scale_y_stream != nullptr) {
                                scale_y *= scale_y_stream[i];
                            }

                            if (scale_z_stream != nullptr) {
                                scale_z *= scale_z_stream[i];
                            }

                            instance.model = float4x4::scale(float3(scale_x, scale_y, scale_z)) * instance.model;

                            if (color_r_stream != nullptr) {
                                instance.color.r = color_r_stream[i];
                            } else {
                                instance.color.r = 1.f;
                            }

                            if (color_g_stream != nullptr) {
                                instance.color.g = color_g_stream[i];
                            } else {
                                instance.color.g = 1.f;
                            }

                            if (color_b_stream != nullptr) {
                                instance.color.b = color_b_stream[i];
                            } else {
                                instance.color.b = 1.f;
                            }

                            if (color_a_stream != nullptr) {
                                instance.color.a = color_a_stream[i];
                            } else {
                                instance.color.a = 1.f;
                            }

                            if (frame_stream != nullptr) {
                                uint32_t frame_index = static_cast<uint32_t>(frame_stream[i]);
                                instance.uv_translation.x = (frame_index % spritesheet_x) * push_constants.uv_scale.x;
                                instance.uv_translation.y = ((frame_index / spritesheet_x) % spritesheet_y) * push_constants.uv_scale.y;
                            }
                        }

                        VertexBuffer* instance_buffer = context->get_render().acquire_transient_vertex_buffer(instance_data.data(), instance_data.size() * sizeof(Material::ParticleInstanceData));
                        KW_ASSERT(instance_buffer != nullptr);

                        DrawCallDescriptor draw_call_descriptor{};
                        draw_call_descriptor.graphics_pipeline = *material->get_graphics_pipeline();
                        draw_call_descriptor.vertex_buffers = &vertex_buffer;
                        draw_call_descriptor.vertex_buffer_count = 1;
                        draw_call_descriptor.instance_buffers = &instance_buffer;
                        draw_call_descriptor.instance_buffer_count = 1;
                        draw_call_descriptor.index_buffer = index_buffer;
                        draw_call_descriptor.index_count = index_count;
                        draw_call_descriptor.instance_count = instance_count;
                        draw_call_descriptor.uniform_textures = uniform_textures.data();
                        draw_call_descriptor.uniform_texture_count = uniform_textures.size();
                        draw_call_descriptor.push_constants = &push_constants;
                        draw_call_descriptor.push_constants_size = sizeof(Material::ParticlePushConstants);

                        {
                            KW_CPU_PROFILER("Draw Call");

                            context->draw(draw_call_descriptor);
                        }
                    }
                }
            }
        }
    }

    const char* get_name() const override {
        return "Particle System Render Pass";
    }

private:
    struct ParticleSystemSort {
        ParticleSystemSort(Camera& camera)
            : origin(camera.get_translation())
            , normal(float3(0.f, 0.f, 1.f) * camera.get_rotation())
        {
        }

        bool operator()(ParticleSystemPrimitive* a, ParticleSystemPrimitive* b) const {
            return dot(a->get_global_translation() - origin, normal) > dot(b->get_global_translation() - origin, normal);
        }

        float3 origin;
        float3 normal;
    };

    ParticleSystemRenderPass& m_render_pass;
};

ParticleSystemRenderPass::ParticleSystemRenderPass(const ParticleSystemRenderPassDescriptor& descriptor)
    : m_scene(*descriptor.scene)
    , m_camera_manager(*descriptor.camera_manager)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
{
    KW_ASSERT(descriptor.scene != nullptr);
    KW_ASSERT(descriptor.camera_manager != nullptr);
    KW_ASSERT(descriptor.transient_memory_resource != nullptr);
}

void ParticleSystemRenderPass::get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // Write to `lighting_attachment`.
}

void ParticleSystemRenderPass::get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // Test against `depth_attachment`.
}

void ParticleSystemRenderPass::get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) {
    static const char* const COLOR_ATTACHMENT_NAME = "lighting_attachment";

    RenderPassDescriptor render_pass_descriptor{};
    render_pass_descriptor.name = "particle_system_render_pass";
    render_pass_descriptor.render_pass = this;
    render_pass_descriptor.write_color_attachment_names = &COLOR_ATTACHMENT_NAME;
    render_pass_descriptor.write_color_attachment_name_count = 1;
    render_pass_descriptor.read_depth_stencil_attachment_name = "depth_attachment";
    render_pass_descriptors.push_back(render_pass_descriptor);
}

void ParticleSystemRenderPass::create_graphics_pipelines(FrameGraph& frame_graph) {
    // All graphics pipelines are stored in particle system's materials.
}

void ParticleSystemRenderPass::destroy_graphics_pipelines(FrameGraph& frame_graph) {
    // All particle system graphics pipelines are destroyed by material manager.
}

Task* ParticleSystemRenderPass::create_task() {
    return m_transient_memory_resource.construct<Task>(*this);
}

} // namespace kw
