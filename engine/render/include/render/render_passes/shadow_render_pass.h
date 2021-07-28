#pragma once

#include "render/frame_graph.h"

#include <core/containers/vector.h>

namespace kw {

class LightPrimitive;
class Scene;
class TaskScheduler;

class ShadowRenderPass : public RenderPass {
public:
    struct ShadowMap {
        LightPrimitive* light_primitive;
        Texture* texture;
    };

    ShadowRenderPass(Render& render, Scene& scene, TaskScheduler& task_scheduler, MemoryResource& persistent_memory_resource, MemoryResource& transient_memory_resource);

    // Fill color attachments created by this render pass.
    void get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors);

    // Fill depth stencil attachment created by this render pass.
    void get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors);

    // Fill render pass descriptors created by this render pass.
    void get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors);

    // Create graphics pipelines for this render pass.
    void create_graphics_pipelines(FrameGraph& frame_graph);

    // The first task creates other tasks that render different cubemap faces. The second task runs after all of
    // those tasks. Both must run between first and second frame graph's tasks.
    std::pair<Task*, Task*> create_tasks();

    const Vector<ShadowMap>& get_shadow_maps() const;
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
