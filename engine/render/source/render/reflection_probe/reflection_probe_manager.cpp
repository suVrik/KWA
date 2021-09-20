#include "render/reflection_probe/reflection_probe_manager.h"
#include "render/camera/camera_manager.h"
#include "render/frame_graph.h"
#include "render/reflection_probe/reflection_probe_primitive.h"
#include "render/render_passes/convolution_render_pass.h"
#include "render/render_passes/emission_render_pass.h"
#include "render/render_passes/geometry_render_pass.h"
#include "render/render_passes/lighting_render_pass.h"
#include "render/render_passes/opaque_shadow_render_pass.h"
#include "render/render_passes/prefilter_render_pass.h"
#include "render/render_passes/reflection_probe_render_pass.h"
#include "render/shadow/shadow_manager.h"

#include <core/concurrency/task.h>
#include <core/concurrency/task_scheduler.h>
#include <core/debug/assert.h>

namespace kw {

struct ReflectionProbeManager::CubemapFrameGraphContext {
    UniquePtr<CameraManager> camera_manager;
    UniquePtr<ShadowManager> shadow_manager;
    UniquePtr<OpaqueShadowRenderPass> opaque_shadow_render_pass;
    UniquePtr<GeometryRenderPass> geometry_render_pass;
    UniquePtr<LightingRenderPass> lighting_render_pass;
    UniquePtr<ReflectionProbeRenderPass> reflection_probe_render_pass;
    UniquePtr<EmissionRenderPass> emission_render_pass;
    UniquePtr<FrameGraph> frame_graph;
};

struct ReflectionProbeManager::IrradianceMapFrameGraphContext {
    UniquePtr<ConvolutionRenderPass> convolution_render_pass;
    UniquePtr<FrameGraph> frame_graph;
};

struct ReflectionProbeManager::PrefilteredEnvironmentMapFrameGraphContext {
    UniquePtr<PrefilterRenderPass> prefilter_render_pass;
    UniquePtr<FrameGraph> frame_graph;
};

class ReflectionProbeManager::BlitTask : public Task {
public:
    BlitTask(RenderPass& render_pass, const char* attachment_name, Texture* destination_texture,
             uint32_t destination_mip_level, uint32_t destination_array_layer)
        : m_render_pass(render_pass)
        , m_attachment_name(attachment_name)
        , m_destination_texture(destination_texture)
        , m_destination_mip_level(destination_mip_level)
        , m_destination_array_layer(destination_array_layer)
    {
    }

    void run() override {
        m_render_pass.blit(m_attachment_name, m_destination_texture, m_destination_mip_level, m_destination_array_layer);
    }

    const char* get_name() const override {
        return "Reflection Probe Manager Blit";
    }

private:
    RenderPass& m_render_pass;
    const char* m_attachment_name;
    Texture* m_destination_texture;
    uint32_t m_destination_mip_level;
    uint32_t m_destination_array_layer;
};

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

class ReflectionProbeManager::BeginTask : public Task {
public:
    BeginTask(ReflectionProbeManager& reflection_probe_manager, Task* end_task)
        : m_manager(reflection_probe_manager)
        , m_end_task(end_task)
    {
    }

    void run() override {
        std::lock_guard lock(m_manager.m_mutex);

        // Destroy unused textures.
        for (auto it = m_manager.m_textures.begin(); it != m_manager.m_textures.end(); ) {
            if (it->use_count() == 1) {
                m_manager.m_render->destroy_texture(**it);
                it = m_manager.m_textures.erase(it);
            } else {
                ++it;
            }
        }

        if (m_manager.m_scene == nullptr) {
            // Bake's isn't in progress yet.
            return;
        }

        start_cubemap_frame_graph();
        start_irradiance_map_frame_graph();
        start_prefiltered_environment_map_frame_graph();
    }

    const char* get_name() const override {
        return "Reflection Probe Manager Begin";
    }

private:
    void start_cubemap_frame_graph() {
        if (m_manager.m_cubemap_bake_contexts.empty()) {
            // All cubemaps are rendered.
            return;
        }

        auto it = m_manager.m_cubemap_bake_contexts.find(m_manager.m_current_cubemap_baking_primitive);
        if (it == m_manager.m_cubemap_bake_contexts.end()) {
            it = m_manager.m_cubemap_bake_contexts.begin();
            m_manager.m_current_cubemap_baking_primitive = it->first;
        }

        BakeContext& bake_context = it->second;
        KW_ASSERT(bake_context.mip_level == 0);
        KW_ASSERT(bake_context.side_index <= 6);
        KW_ASSERT(bake_context.cubemap != nullptr && *bake_context.cubemap != nullptr);

        if (bake_context.side_index == 6) {
            // All sides are rendered. Start convoluting an irradiance map.
            bake_context.mip_level = bake_context.side_index = 0;
            m_manager.m_irradiance_map_bake_contexts.emplace(it->first, std::move(it->second));
            m_manager.m_cubemap_bake_contexts.erase(it);
            return;
        }

        KW_ASSERT(m_manager.m_cubemap_frame_graph_context != nullptr);
        KW_ASSERT(m_manager.m_cubemap_frame_graph_context->camera_manager != nullptr);
        KW_ASSERT(m_manager.m_cubemap_frame_graph_context->shadow_manager != nullptr);
        KW_ASSERT(m_manager.m_cubemap_frame_graph_context->opaque_shadow_render_pass != nullptr);
        KW_ASSERT(m_manager.m_cubemap_frame_graph_context->geometry_render_pass != nullptr);
        KW_ASSERT(m_manager.m_cubemap_frame_graph_context->lighting_render_pass != nullptr);
        KW_ASSERT(m_manager.m_cubemap_frame_graph_context->reflection_probe_render_pass != nullptr);
        KW_ASSERT(m_manager.m_cubemap_frame_graph_context->emission_render_pass != nullptr);
        KW_ASSERT(m_manager.m_cubemap_frame_graph_context->frame_graph != nullptr);

        const float3& translation = it->first->get_global_translation();
        float4x4 view = float4x4::look_at_lh(translation, translation + CUBEMAP_VECTORS[bake_context.side_index].direction, CUBEMAP_VECTORS[bake_context.side_index].up);
        transform camera_transform(inverse(view));

        Camera& camera = m_manager.m_cubemap_frame_graph_context->camera_manager->get_camera();
        camera.set_transform(camera_transform);

        auto [frame_graph_acquire, frame_graph_present] = m_manager.m_cubemap_frame_graph_context->frame_graph->create_tasks();
        Task* shadow_manager_task = m_manager.m_cubemap_frame_graph_context->shadow_manager->create_task();
        auto [opaque_shadow_render_pass_begin_task, opaque_shadow_render_pass_end_task] = m_manager.m_cubemap_frame_graph_context->opaque_shadow_render_pass->create_tasks();
        Task* geometry_render_pass_task = m_manager.m_cubemap_frame_graph_context->geometry_render_pass->create_task();
        Task* lighting_render_pass_task = m_manager.m_cubemap_frame_graph_context->lighting_render_pass->create_task();
        Task* reflection_probe_render_pass_task = m_manager.m_cubemap_frame_graph_context->reflection_probe_render_pass->create_task();
        Task* emission_render_pass_task = m_manager.m_cubemap_frame_graph_context->emission_render_pass->create_task();
        Task* blit_task = m_manager.m_transient_memory_resource.construct<BlitTask>(
            *m_manager.m_cubemap_frame_graph_context->emission_render_pass, "lighting_attachment", *bake_context.cubemap, 0, bake_context.side_index
        );

        opaque_shadow_render_pass_begin_task->add_input_dependencies(m_manager.m_transient_memory_resource, { frame_graph_acquire, shadow_manager_task });
        opaque_shadow_render_pass_end_task->add_input_dependencies(m_manager.m_transient_memory_resource, { opaque_shadow_render_pass_begin_task });
        geometry_render_pass_task->add_input_dependencies(m_manager.m_transient_memory_resource, { frame_graph_acquire });
        lighting_render_pass_task->add_input_dependencies(m_manager.m_transient_memory_resource, { frame_graph_acquire, shadow_manager_task });
        reflection_probe_render_pass_task->add_input_dependencies(m_manager.m_transient_memory_resource, { frame_graph_acquire });
        emission_render_pass_task->add_input_dependencies(m_manager.m_transient_memory_resource, { frame_graph_acquire });
        blit_task->add_input_dependencies(m_manager.m_transient_memory_resource, { frame_graph_acquire });
        m_end_task->add_input_dependencies(m_manager.m_transient_memory_resource, {
            opaque_shadow_render_pass_end_task, geometry_render_pass_task, lighting_render_pass_task,
            reflection_probe_render_pass_task, emission_render_pass_task, blit_task
        });
        frame_graph_present->add_input_dependencies(m_manager.m_transient_memory_resource, { m_end_task });

        m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, shadow_manager_task);
        m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, frame_graph_acquire);
        m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, opaque_shadow_render_pass_begin_task);
        m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, opaque_shadow_render_pass_end_task);
        m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, geometry_render_pass_task);
        m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, lighting_render_pass_task);
        m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, reflection_probe_render_pass_task);
        m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, emission_render_pass_task);
        m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, blit_task);
        m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, frame_graph_present);

        bake_context.side_index++;
    }
    
    void start_irradiance_map_frame_graph() {
        if (m_manager.m_irradiance_map_bake_contexts.empty()) {
            // All irradiance maps are rendered.
            return;
        }

        auto it = m_manager.m_irradiance_map_bake_contexts.find(m_manager.m_current_irradiance_map_baking_primitive);
        if (it == m_manager.m_irradiance_map_bake_contexts.end()) {
            it = m_manager.m_irradiance_map_bake_contexts.begin();
            m_manager.m_current_irradiance_map_baking_primitive = it->first;
        }

        BakeContext& bake_context = it->second;
        KW_ASSERT(bake_context.mip_level == 0);
        KW_ASSERT(bake_context.side_index <= 6);
        KW_ASSERT(bake_context.irradiance_map != nullptr && *bake_context.irradiance_map != nullptr);

        if (bake_context.side_index == 6) {
            // All sides are rendered. Start convoluting an irradiance map.
            bake_context.mip_level = bake_context.side_index = 0;
            m_manager.m_prefiltered_environment_map_bake_contexts.emplace(it->first, std::move(it->second));
            m_manager.m_irradiance_map_bake_contexts.erase(it);
            return;
        }

        KW_ASSERT(m_manager.m_irradiance_map_frame_graph_context != nullptr);
        KW_ASSERT(m_manager.m_irradiance_map_frame_graph_context->convolution_render_pass != nullptr);
        KW_ASSERT(m_manager.m_irradiance_map_frame_graph_context->frame_graph != nullptr);

        float4x4 view = float4x4::look_at_lh(float3(), CUBEMAP_VECTORS[bake_context.side_index].direction, CUBEMAP_VECTORS[bake_context.side_index].up);
        float4x4 projection = float4x4::perspective_lh(PI / 2.f, 1.f, 0.1f, 20.f);
        float4x4 view_projection = view * projection;
        
        auto [frame_graph_acquire, frame_graph_present] = m_manager.m_irradiance_map_frame_graph_context->frame_graph->create_tasks();
        Task* convolution_render_pass_task = m_manager.m_irradiance_map_frame_graph_context->convolution_render_pass->create_task(*bake_context.cubemap, view_projection);
        Task* blit_task = m_manager.m_transient_memory_resource.construct<BlitTask>(
            *m_manager.m_irradiance_map_frame_graph_context->convolution_render_pass, "convolution_attachment", *bake_context.irradiance_map, 0, bake_context.side_index
        );

        convolution_render_pass_task->add_input_dependencies(m_manager.m_transient_memory_resource, { frame_graph_acquire });
        blit_task->add_input_dependencies(m_manager.m_transient_memory_resource, { frame_graph_acquire });
        m_end_task->add_input_dependencies(m_manager.m_transient_memory_resource, { convolution_render_pass_task, blit_task });
        frame_graph_present->add_input_dependencies(m_manager.m_transient_memory_resource, { m_end_task });

        m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, frame_graph_acquire);
        m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, convolution_render_pass_task);
        m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, blit_task);
        m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, frame_graph_present);

        bake_context.side_index++;
    }
    
    void start_prefiltered_environment_map_frame_graph() {
        if (m_manager.m_prefiltered_environment_map_bake_contexts.empty()) {
            if (m_manager.m_cubemap_bake_contexts.empty() && m_manager.m_irradiance_map_bake_contexts.empty()) {
                // Bake is finished.
                m_manager.m_current_prefiltered_environment_map_baking_primitive = nullptr;
                m_manager.m_prefiltered_environment_map_frame_graph_context->prefilter_render_pass->destroy_graphics_pipelines(*m_manager.m_prefiltered_environment_map_frame_graph_context->frame_graph);
                m_manager.m_prefiltered_environment_map_frame_graph_context = nullptr;
                m_manager.m_current_irradiance_map_baking_primitive = nullptr;
                m_manager.m_irradiance_map_frame_graph_context->convolution_render_pass->destroy_graphics_pipelines(*m_manager.m_irradiance_map_frame_graph_context->frame_graph);
                m_manager.m_irradiance_map_frame_graph_context = nullptr;
                m_manager.m_current_cubemap_baking_primitive = nullptr;
                m_manager.m_cubemap_frame_graph_context->emission_render_pass->destroy_graphics_pipelines(*m_manager.m_cubemap_frame_graph_context->frame_graph);
                m_manager.m_cubemap_frame_graph_context->reflection_probe_render_pass->destroy_graphics_pipelines(*m_manager.m_cubemap_frame_graph_context->frame_graph);
                m_manager.m_cubemap_frame_graph_context->lighting_render_pass->destroy_graphics_pipelines(*m_manager.m_cubemap_frame_graph_context->frame_graph);
                m_manager.m_cubemap_frame_graph_context->geometry_render_pass->destroy_graphics_pipelines(*m_manager.m_cubemap_frame_graph_context->frame_graph);
                m_manager.m_cubemap_frame_graph_context->opaque_shadow_render_pass->destroy_graphics_pipelines(*m_manager.m_cubemap_frame_graph_context->frame_graph);
                m_manager.m_cubemap_frame_graph_context = nullptr;
                m_manager.m_scene = nullptr;
            }
            return;
        }
        
        auto it = m_manager.m_prefiltered_environment_map_bake_contexts.find(m_manager.m_current_prefiltered_environment_map_baking_primitive);
        if (it == m_manager.m_prefiltered_environment_map_bake_contexts.end()) {
            it = m_manager.m_prefiltered_environment_map_bake_contexts.begin();
            m_manager.m_current_prefiltered_environment_map_baking_primitive = it->first;
        }

        uint32_t mip_level_count = log2(m_manager.m_prefiltered_environment_map_dimension) + 1U;

        BakeContext& bake_context = it->second;
        KW_ASSERT(bake_context.mip_level <= mip_level_count);
        KW_ASSERT(bake_context.side_index <= 6);
        KW_ASSERT(bake_context.irradiance_map != nullptr && *bake_context.irradiance_map != nullptr);

        if (bake_context.side_index == 6) {
            // All sides are rendered. Start prefiltering next mip level.
            bake_context.mip_level++;
            bake_context.side_index = 0;
        }
        
        if (bake_context.mip_level == mip_level_count) {
            // All sides are rendered. We're done with this reflection probe.
            m_manager.m_current_prefiltered_environment_map_baking_primitive->set_irradiance_map(bake_context.irradiance_map);
            m_manager.m_current_prefiltered_environment_map_baking_primitive->set_prefiltered_environment_map(bake_context.prefiltered_environment_map);
            m_manager.m_prefiltered_environment_map_bake_contexts.erase(it);
            return;
        }

        KW_ASSERT(m_manager.m_prefiltered_environment_map_frame_graph_context != nullptr);
        KW_ASSERT(m_manager.m_prefiltered_environment_map_frame_graph_context->prefilter_render_pass != nullptr);
        KW_ASSERT(m_manager.m_prefiltered_environment_map_frame_graph_context->frame_graph != nullptr);

        float roughness = static_cast<float>(bake_context.mip_level) / (mip_level_count - 1);
        uint32_t inverse_scale_factor = 1U << bake_context.mip_level;

        float scale = 1.f / inverse_scale_factor;
        float offset = 1.f - scale;

        float4x4 view = float4x4::look_at_lh(float3(), CUBEMAP_VECTORS[bake_context.side_index].direction, CUBEMAP_VECTORS[bake_context.side_index].up);
        float4x4 projection = float4x4::perspective_lh(PI / 2.f, 1.f, 0.1f, 20.f) * float4x4::scale(float3(scale, scale, 1.f)) * float4x4::translation(float3(-offset, offset, 0.f));
        float4x4 view_projection = view * projection;
        
        auto [frame_graph_acquire, frame_graph_present] = m_manager.m_prefiltered_environment_map_frame_graph_context->frame_graph->create_tasks();
        Task* prefilter_render_pass_task = m_manager.m_prefiltered_environment_map_frame_graph_context->prefilter_render_pass->create_task(*bake_context.cubemap, view_projection, roughness, inverse_scale_factor);
        Task* blit_task = m_manager.m_transient_memory_resource.construct<BlitTask>(
            *m_manager.m_prefiltered_environment_map_frame_graph_context->prefilter_render_pass, "prefilter_attachment", *bake_context.prefiltered_environment_map, bake_context.mip_level, bake_context.side_index
        );

        prefilter_render_pass_task->add_input_dependencies(m_manager.m_transient_memory_resource, { frame_graph_acquire });
        blit_task->add_input_dependencies(m_manager.m_transient_memory_resource, { frame_graph_acquire });
        m_end_task->add_input_dependencies(m_manager.m_transient_memory_resource, { prefilter_render_pass_task, blit_task });
        frame_graph_present->add_input_dependencies(m_manager.m_transient_memory_resource, { m_end_task });

        m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, frame_graph_acquire);
        m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, prefilter_render_pass_task);
        m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, blit_task);
        m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, frame_graph_present);

        bake_context.side_index++;
    }

    ReflectionProbeManager& m_manager;
    Task* m_end_task;
    float m_elapsed_time;
};

ReflectionProbeManager::ReflectionProbeManager(const ReflectionProbeManagerDescriptor& descriptor)
    : m_task_scheduler(*descriptor.task_scheduler)
    , m_texture_manager(*descriptor.texture_manager)
    , m_cubemap_dimension(descriptor.cubemap_dimension)
    , m_irradiance_map_dimension(descriptor.irradiance_map_dimension)
    , m_prefiltered_environment_map_dimension(descriptor.prefiltered_environment_map_dimension)
    , m_persistent_memory_resource(*descriptor.persistent_memory_resource)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
    , m_primitives(m_persistent_memory_resource)
    , m_render(nullptr)
    , m_scene(nullptr)
    , m_cubemap_bake_contexts(m_persistent_memory_resource)
    , m_current_cubemap_baking_primitive(nullptr)
    , m_irradiance_map_bake_contexts(m_persistent_memory_resource)
    , m_current_irradiance_map_baking_primitive(nullptr)
    , m_prefiltered_environment_map_bake_contexts(m_persistent_memory_resource)
    , m_current_prefiltered_environment_map_baking_primitive(nullptr)
    , m_textures(m_persistent_memory_resource)
{
    KW_ASSERT(descriptor.task_scheduler != nullptr);
    KW_ASSERT(descriptor.texture_manager != nullptr);
    KW_ASSERT(descriptor.cubemap_dimension != 0 && is_pow2(descriptor.cubemap_dimension));
    KW_ASSERT(descriptor.irradiance_map_dimension != 0 && is_pow2(descriptor.irradiance_map_dimension));
    KW_ASSERT(descriptor.prefiltered_environment_map_dimension != 0 && is_pow2(descriptor.prefiltered_environment_map_dimension));
    KW_ASSERT(descriptor.persistent_memory_resource != nullptr);
    KW_ASSERT(descriptor.transient_memory_resource != nullptr);
}

ReflectionProbeManager::~ReflectionProbeManager() {
    for (auto it = m_textures.begin(); it != m_textures.end(); ++it) {
        KW_ASSERT(it->use_count() == 1, "Not all textures are released.");

        m_render->destroy_texture(**it);
    }
}

void ReflectionProbeManager::add(ReflectionProbePrimitive& primitive) {
    std::lock_guard lock(m_mutex);

    KW_ASSERT(primitive.m_reflection_probe_manager == nullptr);
    primitive.m_reflection_probe_manager = this;

    for (ReflectionProbePrimitive*& reflection_probe_primitive : m_primitives) {
        if (reflection_probe_primitive == nullptr) {
            reflection_probe_primitive = &primitive;
            return;
        }
    }

    m_primitives.push_back(&primitive);
}

void ReflectionProbeManager::remove(ReflectionProbePrimitive& primitive) {
    std::lock_guard lock(m_mutex);

    KW_ASSERT(primitive.m_reflection_probe_manager == this);
    primitive.m_reflection_probe_manager = nullptr;

    auto it = std::find(m_primitives.begin(), m_primitives.end(), &primitive);
    KW_ASSERT(it != m_primitives.end());

    *it = nullptr;
}

void ReflectionProbeManager::bake(Render& render, RenderScene& scene) {
    std::lock_guard lock(m_mutex);

    if (m_scene != nullptr) {
        // Bake's already in progress.
        return;
    }

    // If reflection probe manager allocates any textures via a render, it must release them via the same render.
    KW_ASSERT(m_render == nullptr || m_render == &render);

    m_render = &render;
    m_scene = &scene;

    create_bake_contexts();
    create_cubemap_frame_graph();
    create_irradiance_map_frame_graph();
    create_prefiltered_environment_map_frame_graph();
}

void ReflectionProbeManager::create_bake_contexts() {
    for (ReflectionProbePrimitive* primitive : m_primitives) {
        char cubemap_name[64];
        std::snprintf(cubemap_name, sizeof(cubemap_name), "cubemap_%p", static_cast<void*>(primitive));

        CreateTextureDescriptor cubemap_create_texture_descriptor{};
        cubemap_create_texture_descriptor.name = cubemap_name;
        cubemap_create_texture_descriptor.type = TextureType::TEXTURE_CUBE;
        cubemap_create_texture_descriptor.format = TextureFormat::RGBA16_FLOAT;
        cubemap_create_texture_descriptor.array_layer_count = 6;
        cubemap_create_texture_descriptor.width = m_cubemap_dimension;
        cubemap_create_texture_descriptor.height = m_cubemap_dimension;

        SharedPtr<Texture*> cubemap = allocate_shared<Texture*>(m_persistent_memory_resource, m_render->create_texture(cubemap_create_texture_descriptor));

        ClearTextureDescriptor cubemap_clear_texture_descriptor{};
        cubemap_clear_texture_descriptor.texture = *cubemap;
        std::fill(std::begin(cubemap_clear_texture_descriptor.clear_color), std::end(cubemap_clear_texture_descriptor.clear_color), 0.f);

        m_render->clear_texture(cubemap_clear_texture_descriptor);

        char irradiance_map_name[64];
        std::snprintf(irradiance_map_name, sizeof(irradiance_map_name), "irradiance_map_%p", static_cast<void*>(primitive));

        CreateTextureDescriptor irradiance_map_create_texture_descriptor{};
        irradiance_map_create_texture_descriptor.name = irradiance_map_name;
        irradiance_map_create_texture_descriptor.type = TextureType::TEXTURE_CUBE;
        irradiance_map_create_texture_descriptor.format = TextureFormat::RGBA16_FLOAT;
        irradiance_map_create_texture_descriptor.array_layer_count = 6;
        irradiance_map_create_texture_descriptor.width = m_irradiance_map_dimension;
        irradiance_map_create_texture_descriptor.height = m_irradiance_map_dimension;

        SharedPtr<Texture*> irradiance_map = allocate_shared<Texture*>(m_persistent_memory_resource, m_render->create_texture(irradiance_map_create_texture_descriptor));

        ClearTextureDescriptor irradiance_map_clear_texture_descriptor{};
        irradiance_map_clear_texture_descriptor.texture = *irradiance_map;
        std::fill(std::begin(irradiance_map_clear_texture_descriptor.clear_color), std::end(irradiance_map_clear_texture_descriptor.clear_color), 0.f);

        m_render->clear_texture(irradiance_map_clear_texture_descriptor);

        char prefiltered_environment_map_name[64];
        std::snprintf(prefiltered_environment_map_name, sizeof(prefiltered_environment_map_name), "prefiltered_environment_map_%p", static_cast<void*>(primitive));

        CreateTextureDescriptor prefiltered_environment_map_create_texture_descriptor{};
        prefiltered_environment_map_create_texture_descriptor.name = prefiltered_environment_map_name;
        prefiltered_environment_map_create_texture_descriptor.type = TextureType::TEXTURE_CUBE;
        prefiltered_environment_map_create_texture_descriptor.format = TextureFormat::RGBA16_FLOAT;
        prefiltered_environment_map_create_texture_descriptor.mip_level_count = log2(m_prefiltered_environment_map_dimension) + 1;
        prefiltered_environment_map_create_texture_descriptor.array_layer_count = 6;
        prefiltered_environment_map_create_texture_descriptor.width = m_prefiltered_environment_map_dimension;
        prefiltered_environment_map_create_texture_descriptor.height = m_prefiltered_environment_map_dimension;

        SharedPtr<Texture*> prefiltered_environment_map = allocate_shared<Texture*>(m_persistent_memory_resource, m_render->create_texture(prefiltered_environment_map_create_texture_descriptor));

        ClearTextureDescriptor prefiltered_environment_map_clear_texture_descriptor{};
        prefiltered_environment_map_clear_texture_descriptor.texture = *prefiltered_environment_map;
        std::fill(std::begin(prefiltered_environment_map_clear_texture_descriptor.clear_color), std::end(prefiltered_environment_map_clear_texture_descriptor.clear_color), 0.f);

        m_render->clear_texture(prefiltered_environment_map_clear_texture_descriptor);

        m_textures.push_back(cubemap);
        m_textures.push_back(irradiance_map);
        m_textures.push_back(prefiltered_environment_map);

        BakeContext bake_context{};
        bake_context.cubemap = std::move(cubemap);
        bake_context.irradiance_map = std::move(irradiance_map);
        bake_context.prefiltered_environment_map = std::move(prefiltered_environment_map);

        m_cubemap_bake_contexts.emplace(primitive, std::move(bake_context));
    }
}

void ReflectionProbeManager::create_cubemap_frame_graph() {
    UniquePtr<CubemapFrameGraphContext> context = allocate_unique<CubemapFrameGraphContext>(m_persistent_memory_resource);

    context->camera_manager = allocate_unique<CameraManager>(m_persistent_memory_resource);

    Camera& camera = context->camera_manager->get_camera();
    camera.set_fov(PI / 2.f);
    camera.set_aspect_ratio(1.f);
    camera.set_z_near(0.1f);
    camera.set_z_far(100.f);

    ShadowManagerDescriptor shadow_manager_descriptor{};
    shadow_manager_descriptor.render = m_render;
    shadow_manager_descriptor.scene = m_scene;
    shadow_manager_descriptor.camera_manager = context->camera_manager.get();
    shadow_manager_descriptor.shadow_map_count = 3;
    shadow_manager_descriptor.shadow_map_dimension = 512;
    shadow_manager_descriptor.disable_translucent_shadows = true;
    shadow_manager_descriptor.persistent_memory_resource = &m_persistent_memory_resource;
    shadow_manager_descriptor.transient_memory_resource = &m_transient_memory_resource;

    context->shadow_manager = allocate_unique<ShadowManager>(m_persistent_memory_resource, shadow_manager_descriptor);

    OpaqueShadowRenderPassDescriptor opaque_shadow_render_pass_descriptor{};
    opaque_shadow_render_pass_descriptor.scene = m_scene;
    opaque_shadow_render_pass_descriptor.shadow_manager = context->shadow_manager.get();
    opaque_shadow_render_pass_descriptor.task_scheduler = &m_task_scheduler;
    opaque_shadow_render_pass_descriptor.transient_memory_resource = &m_transient_memory_resource;

    context->opaque_shadow_render_pass = allocate_unique<OpaqueShadowRenderPass>(m_persistent_memory_resource, opaque_shadow_render_pass_descriptor);
    
    GeometryRenderPassDescriptor geometry_render_pass_descriptor{};
    geometry_render_pass_descriptor.scene = m_scene;
    geometry_render_pass_descriptor.camera_manager = context->camera_manager.get();
    geometry_render_pass_descriptor.transient_memory_resource = &m_transient_memory_resource;

    context->geometry_render_pass = allocate_unique<GeometryRenderPass>(m_persistent_memory_resource, geometry_render_pass_descriptor);

    LightingRenderPassDescriptor lighting_render_pass_descriptor{};
    lighting_render_pass_descriptor.render = m_render;
    lighting_render_pass_descriptor.scene = m_scene;
    lighting_render_pass_descriptor.camera_manager = context->camera_manager.get();
    lighting_render_pass_descriptor.shadow_manager = context->shadow_manager.get();
    lighting_render_pass_descriptor.transient_memory_resource = &m_transient_memory_resource;

    context->lighting_render_pass = allocate_unique<LightingRenderPass>(m_persistent_memory_resource, lighting_render_pass_descriptor);

    ReflectionProbeRenderPassDescriptor reflection_probe_render_pass_descriptor{};
    reflection_probe_render_pass_descriptor.render = m_render;
    reflection_probe_render_pass_descriptor.texture_manager = &m_texture_manager;
    reflection_probe_render_pass_descriptor.scene = m_scene;
    reflection_probe_render_pass_descriptor.camera_manager = context->camera_manager.get();
    reflection_probe_render_pass_descriptor.transient_memory_resource = &m_transient_memory_resource;

    context->reflection_probe_render_pass = allocate_unique<ReflectionProbeRenderPass>(m_persistent_memory_resource, reflection_probe_render_pass_descriptor);

    EmissionRenderPassDescriptor emission_render_pass_descriptor{};
    emission_render_pass_descriptor.render = m_render;
    emission_render_pass_descriptor.transient_memory_resource = &m_transient_memory_resource;

    context->emission_render_pass = allocate_unique<EmissionRenderPass>(m_persistent_memory_resource, emission_render_pass_descriptor);

    Vector<AttachmentDescriptor> color_attachment_descriptors(m_transient_memory_resource);
    context->opaque_shadow_render_pass->get_color_attachment_descriptors(color_attachment_descriptors);
    context->geometry_render_pass->get_color_attachment_descriptors(color_attachment_descriptors);
    context->lighting_render_pass->get_color_attachment_descriptors(color_attachment_descriptors);
    context->reflection_probe_render_pass->get_color_attachment_descriptors(color_attachment_descriptors);
    context->emission_render_pass->get_color_attachment_descriptors(color_attachment_descriptors);

    for (AttachmentDescriptor& attachment_descriptor : color_attachment_descriptors) {
        if (std::strcmp(attachment_descriptor.name, "lighting_attachment") == 0) {
            attachment_descriptor.is_blit_source = true;
        }
    }

    convert_relative_to_absolute(color_attachment_descriptors);

    Vector<AttachmentDescriptor> depth_stencil_attachment_descriptors(m_transient_memory_resource);
    context->opaque_shadow_render_pass->get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    context->geometry_render_pass->get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    context->lighting_render_pass->get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    context->reflection_probe_render_pass->get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);
    context->emission_render_pass->get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);

    convert_relative_to_absolute(depth_stencil_attachment_descriptors);

    Vector<RenderPassDescriptor> render_pass_descriptors(m_transient_memory_resource);
    context->opaque_shadow_render_pass->get_render_pass_descriptors(render_pass_descriptors);
    context->geometry_render_pass->get_render_pass_descriptors(render_pass_descriptors);
    context->lighting_render_pass->get_render_pass_descriptors(render_pass_descriptors);
    context->reflection_probe_render_pass->get_render_pass_descriptors(render_pass_descriptors);
    context->emission_render_pass->get_render_pass_descriptors(render_pass_descriptors);

    FrameGraphDescriptor frame_graph_descriptor{};
    frame_graph_descriptor.render = m_render;
    frame_graph_descriptor.is_aliasing_enabled = true;
    frame_graph_descriptor.descriptor_set_count_per_descriptor_pool = 256;
    frame_graph_descriptor.uniform_texture_count_per_descriptor_pool = 4 * 256;
    frame_graph_descriptor.uniform_sampler_count_per_descriptor_pool = 256;
    frame_graph_descriptor.uniform_buffer_count_per_descriptor_pool = 256;
    frame_graph_descriptor.color_attachment_descriptors = color_attachment_descriptors.data();
    frame_graph_descriptor.color_attachment_descriptor_count = color_attachment_descriptors.size();
    frame_graph_descriptor.depth_stencil_attachment_descriptors = depth_stencil_attachment_descriptors.data();
    frame_graph_descriptor.depth_stencil_attachment_descriptor_count = depth_stencil_attachment_descriptors.size();
    frame_graph_descriptor.render_pass_descriptors = render_pass_descriptors.data();
    frame_graph_descriptor.render_pass_descriptor_count = render_pass_descriptors.size();

    context->frame_graph = UniquePtr<FrameGraph>(FrameGraph::create_instance(frame_graph_descriptor), m_persistent_memory_resource);

    context->opaque_shadow_render_pass->create_graphics_pipelines(*context->frame_graph);
    context->geometry_render_pass->create_graphics_pipelines(*context->frame_graph);
    context->lighting_render_pass->create_graphics_pipelines(*context->frame_graph);
    context->reflection_probe_render_pass->create_graphics_pipelines(*context->frame_graph);
    context->emission_render_pass->create_graphics_pipelines(*context->frame_graph);

    m_cubemap_frame_graph_context = std::move(context);
}

void ReflectionProbeManager::create_irradiance_map_frame_graph() {
    UniquePtr<IrradianceMapFrameGraphContext> context = allocate_unique<IrradianceMapFrameGraphContext>(m_persistent_memory_resource);

    ConvolutionRenderPassDescriptor convolution_render_pass_descriptor{};
    convolution_render_pass_descriptor.render = m_render;
    convolution_render_pass_descriptor.side_dimension = m_irradiance_map_dimension;
    convolution_render_pass_descriptor.transient_memory_resource = &m_transient_memory_resource;

    context->convolution_render_pass = allocate_unique<ConvolutionRenderPass>(m_persistent_memory_resource, convolution_render_pass_descriptor);
    
    Vector<AttachmentDescriptor> color_attachment_descriptors(m_transient_memory_resource);
    context->convolution_render_pass->get_color_attachment_descriptors(color_attachment_descriptors);

    Vector<AttachmentDescriptor> depth_stencil_attachment_descriptors(m_transient_memory_resource);
    context->convolution_render_pass->get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);

    Vector<RenderPassDescriptor> render_pass_descriptors(m_transient_memory_resource);
    context->convolution_render_pass->get_render_pass_descriptors(render_pass_descriptors);

    FrameGraphDescriptor frame_graph_descriptor{};
    frame_graph_descriptor.render = m_render;
    frame_graph_descriptor.is_aliasing_enabled = true;
    frame_graph_descriptor.descriptor_set_count_per_descriptor_pool = 36;
    frame_graph_descriptor.uniform_texture_count_per_descriptor_pool = 36;
    frame_graph_descriptor.uniform_sampler_count_per_descriptor_pool = 36;
    frame_graph_descriptor.uniform_buffer_count_per_descriptor_pool = 1;
    frame_graph_descriptor.color_attachment_descriptors = color_attachment_descriptors.data();
    frame_graph_descriptor.color_attachment_descriptor_count = color_attachment_descriptors.size();
    frame_graph_descriptor.depth_stencil_attachment_descriptors = depth_stencil_attachment_descriptors.data();
    frame_graph_descriptor.depth_stencil_attachment_descriptor_count = depth_stencil_attachment_descriptors.size();
    frame_graph_descriptor.render_pass_descriptors = render_pass_descriptors.data();
    frame_graph_descriptor.render_pass_descriptor_count = render_pass_descriptors.size();

    context->frame_graph = UniquePtr<FrameGraph>(FrameGraph::create_instance(frame_graph_descriptor), m_persistent_memory_resource);

    context->convolution_render_pass->create_graphics_pipelines(*context->frame_graph);

    m_irradiance_map_frame_graph_context = std::move(context);
}

void ReflectionProbeManager::create_prefiltered_environment_map_frame_graph() {
    UniquePtr<PrefilteredEnvironmentMapFrameGraphContext> context = allocate_unique<PrefilteredEnvironmentMapFrameGraphContext>(m_persistent_memory_resource);

    PrefilterRenderPassDescriptor prefilter_render_pass_descriptor{};
    prefilter_render_pass_descriptor.render = m_render;
    prefilter_render_pass_descriptor.side_dimension = m_prefiltered_environment_map_dimension;
    prefilter_render_pass_descriptor.transient_memory_resource = &m_transient_memory_resource;

    context->prefilter_render_pass = allocate_unique<PrefilterRenderPass>(m_persistent_memory_resource, prefilter_render_pass_descriptor);
    
    Vector<AttachmentDescriptor> color_attachment_descriptors(m_transient_memory_resource);
    context->prefilter_render_pass->get_color_attachment_descriptors(color_attachment_descriptors);

    Vector<AttachmentDescriptor> depth_stencil_attachment_descriptors(m_transient_memory_resource);
    context->prefilter_render_pass->get_depth_stencil_attachment_descriptors(depth_stencil_attachment_descriptors);

    Vector<RenderPassDescriptor> render_pass_descriptors(m_transient_memory_resource);
    context->prefilter_render_pass->get_render_pass_descriptors(render_pass_descriptors);

    FrameGraphDescriptor frame_graph_descriptor{};
    frame_graph_descriptor.render = m_render;
    frame_graph_descriptor.is_aliasing_enabled = true;
    frame_graph_descriptor.descriptor_set_count_per_descriptor_pool = 36;
    frame_graph_descriptor.uniform_texture_count_per_descriptor_pool = 36;
    frame_graph_descriptor.uniform_sampler_count_per_descriptor_pool = 36;
    frame_graph_descriptor.uniform_buffer_count_per_descriptor_pool = 1;
    frame_graph_descriptor.color_attachment_descriptors = color_attachment_descriptors.data();
    frame_graph_descriptor.color_attachment_descriptor_count = color_attachment_descriptors.size();
    frame_graph_descriptor.depth_stencil_attachment_descriptors = depth_stencil_attachment_descriptors.data();
    frame_graph_descriptor.depth_stencil_attachment_descriptor_count = depth_stencil_attachment_descriptors.size();
    frame_graph_descriptor.render_pass_descriptors = render_pass_descriptors.data();
    frame_graph_descriptor.render_pass_descriptor_count = render_pass_descriptors.size();

    context->frame_graph = UniquePtr<FrameGraph>(FrameGraph::create_instance(frame_graph_descriptor), m_persistent_memory_resource);

    context->prefilter_render_pass->create_graphics_pipelines(*context->frame_graph);

    m_prefiltered_environment_map_frame_graph_context = std::move(context);
}

void ReflectionProbeManager::convert_relative_to_absolute(Vector<AttachmentDescriptor>& attachment_descriptors) const {
    for (AttachmentDescriptor& attachment_descriptor : attachment_descriptors) {
        attachment_descriptor.size_class = SizeClass::ABSOLUTE;
        attachment_descriptor.width = m_cubemap_dimension;
        attachment_descriptor.height = m_cubemap_dimension;
    }
}

Pair<Task*, Task*> ReflectionProbeManager::create_tasks() {
    Task* present_task = m_transient_memory_resource.construct<NoopTask>("Reflection Probe Manager End");
    Task* acquire_task = m_transient_memory_resource.construct<BeginTask>(*this, present_task);

    return { acquire_task, present_task };
}

} // namespace kw
