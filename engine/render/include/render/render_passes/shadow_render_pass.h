#pragma once

#include "render/render_passes/base_render_pass.h"

namespace kw {

class float4x4;
class LightPrimitive;
class Scene;
class TaskScheduler;


struct ShadowRenderPassDescriptor {
    Render* render;
    Scene* scene;
    TaskScheduler* task_scheduler;
    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

class ShadowRenderPass : public BaseRenderPass {
public:
    struct ShadowMap {
        LightPrimitive* light_primitive;
        Texture* texture;

        // For internal usage by `BeginTask` and `WorkerTask`.
        uint64_t max_counter[6];
        size_t primitive_count[6];
    };

    explicit ShadowRenderPass(const ShadowRenderPassDescriptor& descriptor);
    ~ShadowRenderPass();

    void get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) override;
    void get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) override;
    void get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) override;
    void create_graphics_pipelines(FrameGraph& frame_graph) override;
    void destroy_graphics_pipelines(FrameGraph& frame_graph) override;

    // Must be placed between acquire and present frame graph's tasks.
    std::pair<Task*, Task*> create_tasks();

    // Shadow maps are available after first render pass's task.
    const Vector<ShadowMap>& get_shadow_maps() const;

    // With this shadow map, everything is considered to be lit.
    Texture* get_dummy_shadow_map() const;

private:
    class BeginTask;
    class WorkerTask;

    Render& m_render;
    Scene& m_scene;
    TaskScheduler& m_task_scheduler;
    MemoryResource& m_persistent_memory_resource;
    MemoryResource& m_transient_memory_resource;

    GraphicsPipeline* m_solid_graphics_pipeline;
    GraphicsPipeline* m_skinned_graphics_pipeline;

    Vector<ShadowMap> m_shadow_maps;
    Texture* m_dummy_shadow_map;
};

} // namespace kw
