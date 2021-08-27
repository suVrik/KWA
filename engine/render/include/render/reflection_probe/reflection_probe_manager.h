#pragma once

#include "render/geometry/geometry_notifier.h"

#include <core/containers/pair.h>
#include <core/containers/shared_ptr.h>
#include <core/containers/unique_ptr.h>
#include <core/containers/vector.h>

#include <mutex>

namespace kw {

class AttachmentDescriptor;
class FrameGraph;
class ReflectionProbePrimitive;
class Render;
class Scene;
class Task;
class TaskScheduler;
class Texture;
class TextureManager;

struct ReflectionProbeManagerDescriptor {
    TaskScheduler* task_scheduler;
    TextureManager* texture_manager;

    uint32_t cubemap_dimension;
    uint32_t irradiance_map_dimension;
    uint32_t prefiltered_environment_map_dimension;

    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

// TODO: I don't like this "manager". Perhaps some `ReflectionProbeBaker` that can be constructed after Scene and that
//  just casually queries all reflection probes on the scene? The ugly Scene <-> ReflectionProbeManager is annoying.
class ReflectionProbeManager {
public:
    explicit ReflectionProbeManager(const ReflectionProbeManagerDescriptor& descriptor);
    ~ReflectionProbeManager();

    void add(ReflectionProbePrimitive& primitive);
    void remove(ReflectionProbePrimitive& primitive);

    // Constructs specific frame graphs that start rendering reflection probes in parallel.
    // When both the irradiance map and pre-filtered environment maps are rendered, they're assigned to reflection probe.
    // If bake is already in progress, the function won't do anything.
    void bake(Render& render, Scene& scene);

    // The first task assignes textures to reflection probes and must be placed before the lighting pass that uses them.
    // The first task also enqueues the worker tasks that render the cubemaps, irradiance maps and so on during baking.
    // All the worker tasks are guaranteed to execute before the second task executes, which starts the GPU work.
    Pair<Task*, Task*> create_tasks();

private:
    class CubemapFrameGraphContext;
    class IrradianceMapFrameGraphContext;
    class PrefilteredEnvironmentMapFrameGraphContext;
    class BlitTask;
    class BeginTask;

    struct BakeContext {
        // The mip level is guaranteed to be zero for cubemap and irradiance map.
        uint32_t mip_level;
        uint32_t side_index;

        SharedPtr<Texture*> cubemap;
        SharedPtr<Texture*> irradiance_map;
        SharedPtr<Texture*> prefiltered_environment_map;
    };

    void create_bake_contexts();
    void create_cubemap_frame_graph();
    void create_irradiance_map_frame_graph();
    void create_prefiltered_environment_map_frame_graph();

    void convert_relative_to_absolute(Vector<AttachmentDescriptor>& attachment_descriptors) const;

    TaskScheduler& m_task_scheduler;
    TextureManager& m_texture_manager;
    MemoryResource& m_persistent_memory_resource;
    MemoryResource& m_transient_memory_resource;

    uint32_t m_cubemap_dimension;
    uint32_t m_irradiance_map_dimension;
    uint32_t m_prefiltered_environment_map_dimension;

    Vector<ReflectionProbePrimitive*> m_primitives;

    Render* m_render;
    Scene* m_scene;

    UniquePtr<CubemapFrameGraphContext> m_cubemap_frame_graph_context;
    UnorderedMap<ReflectionProbePrimitive*, BakeContext> m_cubemap_bake_contexts;
    ReflectionProbePrimitive* m_current_cubemap_baking_primitive;

    UniquePtr<IrradianceMapFrameGraphContext> m_irradiance_map_frame_graph_context;
    UnorderedMap<ReflectionProbePrimitive*, BakeContext> m_irradiance_map_bake_contexts;
    ReflectionProbePrimitive* m_current_irradiance_map_baking_primitive;

    UniquePtr<PrefilteredEnvironmentMapFrameGraphContext> m_prefiltered_environment_map_frame_graph_context;
    UnorderedMap<ReflectionProbePrimitive*, BakeContext> m_prefiltered_environment_map_bake_contexts;
    ReflectionProbePrimitive* m_current_prefiltered_environment_map_baking_primitive;

    // The same idea as behind `TextureManager`. When ref-counter becomes 1, the texture is destroyed.
    Vector<SharedPtr<Texture*>> m_textures;

    std::mutex m_mutex;
};

} // namespace kw
