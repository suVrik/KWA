#include "render/texture/texture_manager.h"
#include "render/texture/texture_loader.h"

#include <core/concurrency/task.h>
#include <core/concurrency/task_scheduler.h>
#include <core/debug/assert.h>
#include <core/error.h>
#include <core/math/scalar.h>

namespace kw {

class TextureManager::LoadingTask final : public Task {
public:
    LoadingTask(TextureManager& manager, TextureLoader& texture_loader, Texture* texture, size_t bytes_per_texture)
        : m_manager(manager)
        , m_texture_loader(texture_loader)
        , m_texture(texture)
        , m_bytes_per_texture(bytes_per_texture)
    {
    }

    void run() override {
        KW_ASSERT(!m_texture_loader.is_loaded());

        UploadTextureDescriptor upload_texture_descriptor = m_texture_loader.load(m_manager.m_transient_memory_resource, m_bytes_per_texture);
        upload_texture_descriptor.texture = m_texture;

        m_manager.m_render.upload_texture(upload_texture_descriptor);
    }

private:
    TextureManager& m_manager;
    TextureLoader& m_texture_loader;
    Texture* m_texture;
    size_t m_bytes_per_texture;
};

class TextureManager::PendingTask final : public Task {
public:
    PendingTask(TextureManager& manager, TextureLoader& texture_loader, Texture*& texture, const char* relative_path, size_t bytes_per_texture, Task* end_task)
        : m_manager(manager)
        , m_texture_loader(texture_loader)
        , m_texture(texture)
        , m_relative_path(relative_path)
        , m_bytes_per_texture(bytes_per_texture)
        , m_end_task(end_task)
    {
    }

    void run() override {
        m_texture_loader = TextureLoader(m_relative_path);
        KW_ASSERT(!m_texture_loader.is_loaded());

        CreateTextureDescriptor create_texture_descriptor = m_texture_loader.get_create_texture_descriptor();
        create_texture_descriptor.name = m_relative_path;

        m_texture = m_manager.m_render.create_texture(create_texture_descriptor);
        KW_ASSERT(m_texture != nullptr);

        LoadingTask* loading_task = new (m_manager.m_transient_memory_resource.allocate<LoadingTask>()) LoadingTask(m_manager, m_texture_loader, m_texture, m_bytes_per_texture);
        KW_ASSERT(loading_task != nullptr);

        loading_task->add_output_dependencies(m_manager.m_transient_memory_resource, { m_end_task });

        m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, loading_task);
    }

private:
    TextureManager& m_manager;
    TextureLoader& m_texture_loader;
    Texture*& m_texture;
    const char* m_relative_path;
    size_t m_bytes_per_texture;
    Task* m_end_task;
};

class TextureManager::BeginTask final : public Task {
public:
    BeginTask(TextureManager& manager, Task* end_task)
        : m_manager(manager)
        , m_end_task(end_task)
    {
    }

    void run() override {
        // No lock here because tasks that load textures are expected to run before begin task.

        if (!m_manager.m_pending_textures.empty() || !m_manager.m_loading_textures.empty()) {
            size_t total_textures = m_manager.m_pending_textures.size() + m_manager.m_loading_textures.size();

            // 32 bytes is enough to load one texel of any texture format.
            size_t bytes_per_texture = std::max(m_manager.m_transient_memory_allocation / total_textures, 32ull);

            //
            // Continue loading mip levels for older textures.
            //

            for (size_t i = 0; i < m_manager.m_loading_textures.size(); ) {
                auto& [texture_loader, texture] = m_manager.m_loading_textures[i];
                if (!texture_loader->is_loaded()) {
                    LoadingTask* loading_task = new (m_manager.m_transient_memory_resource.allocate<LoadingTask>()) LoadingTask(m_manager, *texture_loader, *texture, bytes_per_texture);
                    KW_ASSERT(loading_task != nullptr);

                    loading_task->add_output_dependencies(m_manager.m_transient_memory_resource, { m_end_task });

                    m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, loading_task);

                    i++;
                } else {
                    std::swap(m_manager.m_loading_textures[i], m_manager.m_loading_textures.back());
                    m_manager.m_loading_textures.pop_back();
                }
            }

            //
            // Start loading brand new textures.
            //

            m_manager.m_loading_textures.reserve(m_manager.m_loading_textures.size() + m_manager.m_pending_textures.size());

            for (auto& [relative_path, texture] : m_manager.m_pending_textures) {
                auto& [texture_loader_, texture_] = m_manager.m_loading_textures.emplace_back(allocate_unique<TextureLoader>(m_manager.m_persistent_memory_resource), std::move(texture));

                PendingTask* pending_task = new (m_manager.m_transient_memory_resource.allocate<PendingTask>()) PendingTask(m_manager, *texture_loader_, *texture_, relative_path.c_str(), bytes_per_texture, m_end_task);
                KW_ASSERT(pending_task != nullptr);

                pending_task->add_output_dependencies(m_manager.m_transient_memory_resource, { m_end_task });

                m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, pending_task);
            }

            m_manager.m_pending_textures.clear();
        }

        //
        // Destroy textures that only referenced from `TextureManager`.
        //

        for (auto it = m_manager.m_textures.begin(); it != m_manager.m_textures.end(); ) {
            if (it->second.use_count() == 1) {
                m_manager.m_render.destroy_texture(*it->second);
                it = m_manager.m_textures.erase(it);
            } else {
                ++it;
            }
        }
    }

private:
    TextureManager& m_manager;
    Task* m_end_task;
};

TextureManager::TextureManager(const TextureManagerDescriptor& descriptor)
    : m_render(*descriptor.render)
    , m_task_scheduler(*descriptor.task_scheduler)
    , m_persistent_memory_resource(*descriptor.persistent_memory_resource)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
    , m_transient_memory_allocation(descriptor.transient_memory_allocation)
    , m_textures(*descriptor.persistent_memory_resource)
    , m_pending_textures(*descriptor.persistent_memory_resource)
    , m_loading_textures(*descriptor.persistent_memory_resource)
{
    KW_ASSERT(descriptor.render != nullptr);
    KW_ASSERT(descriptor.task_scheduler != nullptr);
    KW_ASSERT(descriptor.persistent_memory_resource != nullptr);
    KW_ASSERT(descriptor.transient_memory_resource != nullptr);

    m_textures.reserve(32);
    m_pending_textures.reserve(32);
    m_loading_textures.reserve(32);
}

TextureManager::~TextureManager() {
    m_pending_textures.clear();
    m_loading_textures.clear();

    for (auto& [relative_path, texture] : m_textures) {
        KW_ASSERT(texture.use_count() == 1, "Not all texture resources are released.");

        m_render.destroy_texture(*texture);
    }
}

SharedPtr<Texture*> TextureManager::load(const char* relative_path) {
    {
        std::shared_lock shared_lock(m_textures_mutex);

        auto it = m_textures.find(String(relative_path, m_transient_memory_resource));
        if (it != m_textures.end()) {
            return it->second;
        }
    }

    {
        std::lock_guard lock_guard(m_textures_mutex);

        auto result = m_textures.emplace(String(relative_path, m_persistent_memory_resource), SharedPtr<Texture*>());
        if (!result.second) {
            // Could return here if texture is enqueued from multiple threads.
            return result.first->second;
        }

        result.first->second = allocate_shared<Texture*>(m_persistent_memory_resource, nullptr);

        m_pending_textures.emplace_back(result.first->first, result.first->second);

        return result.first->second;
    }
}

std::pair<Task*, Task*> TextureManager::create_tasks() {
    Task* end_task = new (m_transient_memory_resource.allocate<NoopTask>()) NoopTask();
    Task* begin_task = new (m_transient_memory_resource.allocate<BeginTask>()) BeginTask(*this, end_task);

    return { begin_task, end_task };
}

} // namespace kw
