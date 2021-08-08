#pragma once

#include <core/containers/shared_ptr.h>
#include <core/containers/string.h>
#include <core/containers/unordered_map.h>
#include <core/containers/vector.h>

#include <shared_mutex>

namespace kw {

class FrameGraph;
class GraphicsPipeline;
class Material;
class Task;
class TaskScheduler;
class TextureManager;

struct MaterialManagerDescriptor {
    FrameGraph* frame_graph;
    TaskScheduler* task_scheduler;
    TextureManager* texture_manager;

    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

struct MaterialManagerTasks {
    Task* begin;
    Task* material_end;
    Task* graphics_pipeline_end;
};

class MaterialManager {
public:
    explicit MaterialManager(const MaterialManagerDescriptor& descriptor);
    ~MaterialManager();

    // Enqueue material loading if it's not yet loaded. Concurrent loads are allowed.
    SharedPtr<Material> load(const char* material_path);

    // `begin` task creates worker tasks that load all enqueued materials at the moment. These tasks may load new
    // textures from texture manager and create graphics pipeline tasks if new graphics pipeline is encountered.
    // The material tasks will be finished before `material_end` task starts. The graphics pipeline tasks will be
    // finished before the `graphics_pipeline_end` task starts. Texture manager must start after `material_end` to
    // load material textures (yet it is not obligated to start after `graphics_pipeline_end` which might take longer).
    MaterialManagerTasks create_tasks();

private:
    class BeginTask;
    class MaterialTask;
    class GraphicsPipelineTask;

    struct GraphicsPipelineContext {
        GraphicsPipelineContext(MemoryResource& memory_resource);

        SharedPtr<GraphicsPipeline*> graphics_pipeline;

        // These are used on graphics pipeline creation and to validate further graphics pipeline loads.
        Vector<String> textures;
        bool is_skinned;
    };

    struct GraphicsPipelineHash {
        size_t operator()(const std::pair<String, String>& value) const;
    };

    // If graphics pipeline with given vertex and fragment shaders doesn't exist, graphics pipeline task is enqueued.
    SharedPtr<GraphicsPipeline*> load(const char* vertex_shader, const char* fragment_shader,
                                      const Vector<String>& textures, bool is_skinned,
                                      Task* graphics_pipeline_end);

    FrameGraph& m_frame_graph;
    TaskScheduler& m_task_scheduler;
    TextureManager& m_texture_manager;

    MemoryResource& m_persistent_memory_resource;
    MemoryResource& m_transient_memory_resource;

    UnorderedMap<std::pair<String, String>, GraphicsPipelineContext, GraphicsPipelineHash> m_graphics_pipelines;
    std::shared_mutex m_graphics_pipelines_mutex;

    UnorderedMap<String, SharedPtr<Material>> m_materials;
    Vector<std::pair<const String&, SharedPtr<Material>>> m_pending_materials;
    std::shared_mutex m_materials_mutex;
};

} // namespace kw
