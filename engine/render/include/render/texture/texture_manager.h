#pragma once

#include <core/containers/shared_ptr.h>
#include <core/containers/string.h>
#include <core/containers/unique_ptr.h>
#include <core/containers/unordered_map.h>
#include <core/containers/vector.h>

#include <shared_mutex>

namespace kw {

class Render;
class Task;
class TaskScheduler;
class Texture;
class TextureLoader;

struct TextureManagerDescriptor {
    Render* render;
    TaskScheduler* task_scheduler;

    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;

    // The number of bytes allocated from transient memory resource to load enqueued textures
    // (can take more if too many textures are loaded at once, up to 32 bytes per texture).
    size_t transient_memory_allocation;
};

class TextureManager {
public:
    explicit TextureManager(const TextureManagerDescriptor& descriptor);
    ~TextureManager();

    // Enqueue texture loading if it's not yet loaded. Concurrent loads are allowed.
    SharedPtr<Texture*> load(const char* relative_path);

    // O(n) where n is the total number of loaded textures. Designed for tools.
    const String& get_relative_path(const SharedPtr<Texture*>& texture) const;

    // The first task creates worker tasks that load all enqueued textures at the moment. Those tasks will be finished
    // before the second task starts. If you are planning to load textures on this frame, you need to place your task
    // before the first task. If you are planning to use texture loaded on this frame, you need to place your task
    // after the second task.
    std::pair<Task*, Task*> create_tasks();

private:
    class BeginTask;
    class PendingTask;
    class LoadingTask;

    Render& m_render;
    TaskScheduler& m_task_scheduler;

    MemoryResource& m_persistent_memory_resource;
    MemoryResource& m_transient_memory_resource;

    size_t m_transient_memory_allocation;

    UnorderedMap<String, SharedPtr<Texture*>> m_textures;
    std::shared_mutex m_textures_mutex;

    // Textures that are not even opened yet.
    Vector<std::pair<const String&, SharedPtr<Texture*>>> m_pending_textures;

    // Opened textures with some not yet loaded mip levels.
    Vector<std::pair<UniquePtr<TextureLoader>, SharedPtr<Texture*>>> m_loading_textures;
};

} // namespace kw
