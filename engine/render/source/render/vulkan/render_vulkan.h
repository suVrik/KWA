#pragma once

#include "render/buddy_allocator.h"
#include "render/render.h"

#include <core/concurrency/spinlock.h>
#include <core/concurrency/task.h>
#include <core/containers/queue.h>
#include <core/containers/shared_ptr.h>
#include <core/containers/unique_ptr.h>
#include <core/containers/vector.h>

#include <vulkan/vulkan.h>

#include <mutex>

namespace kw {

class TimelineSemaphore;

class BufferVulkan {
public:
    // For transient buffer this is equal to `m_transient_buffer` from `RenderVulkan`.
    VkBuffer buffer;

    // For transient buffer this is equal to 0.
    // For a new buffer this is equal to 0.
    uint64_t transfer_semaphore_value;

    union {
        struct {
            uint64_t device_data_index : 16;
            uint64_t device_data_offset : 48;
        };

        uint64_t transient_buffer_offset;
    };
};

static_assert(sizeof(BufferVulkan) == 24);

class VertexBufferVulkan : public VertexBuffer, public BufferVulkan {
public:
    VertexBufferVulkan(size_t size_, VkBuffer buffer_, uint64_t device_data_index_, uint64_t device_data_offset_);
    VertexBufferVulkan(size_t size_, VkBuffer buffer_, uint64_t transient_buffer_offset_);

    void set_available_size(size_t value) {
        m_available_size = value;
    }
};

static_assert(sizeof(VertexBufferVulkan) == 40);

class IndexBufferVulkan : public IndexBuffer, public BufferVulkan {
public:
    IndexBufferVulkan(size_t size_, IndexSize index_size_, VkBuffer buffer_, uint64_t device_data_index_, uint64_t device_data_offset_);
    IndexBufferVulkan(size_t size_, IndexSize index_size_, VkBuffer buffer_, uint64_t transient_buffer_offset_);

    void set_available_size(size_t value) {
        m_available_size = value;
    }
};

static_assert(sizeof(IndexBufferVulkan) == 40);

class UniformBufferVulkan : public UniformBuffer {
public:
    // Buffer is implicitly equal to `m_transient_buffer` from `RenderVulkan`.
    UniformBufferVulkan(size_t size_, uint64_t transient_buffer_offset_);

    uint64_t transient_buffer_offset;
};

static_assert(sizeof(UniformBufferVulkan) == 16);

class TextureVulkan : public Texture {
public:
    TextureVulkan(TextureType type, TextureFormat format,
                  uint32_t mip_level_count, uint32_t array_layer_count, uint32_t width, uint32_t height, uint32_t depth,
                  VkImage image_, uint64_t device_data_index_, uint64_t device_data_offset_);

    void set_available_mip_level_count(uint32_t value) {
        m_available_mip_level_count = value;
    }

    VkImage image;

    // For a new texture this is equal to `VK_NULL_HANDLE`. First upload creates the image view,
    // newer uploads destroy older image views and create new ones.
    VkImageView image_view;

    // For a new texture this is equal to 0.
    uint64_t transfer_semaphore_value;

    uint64_t device_data_index : 16;
    uint64_t device_data_offset : 48;
};

static_assert(sizeof(TextureVulkan) == 48);

class HostTextureVulkan : public HostTexture {
public:
    HostTextureVulkan(TextureFormat format, uint32_t width, uint32_t height,
                      VkBuffer buffer_, VkDeviceMemory device_memory_, void* memory_mapping_, size_t size_);

    VkBuffer buffer;
    VkDeviceMemory device_memory;
    void* memory_mapping;
    size_t size;
};

static_assert(sizeof(HostTextureVulkan) == 48);

class RenderVulkan : public Render {
public:
    struct DeviceAllocation {
        VkDeviceMemory memory;
        uint64_t data_index : 16;
        uint64_t data_offset : 48;
    };

    explicit RenderVulkan(const RenderDescriptor& descriptor);
    ~RenderVulkan() override;

    VertexBuffer* create_vertex_buffer(const char* name, size_t size) override;
    void upload_vertex_buffer(VertexBuffer* vertex_buffer, const void* data, size_t size) override;
    void destroy_vertex_buffer(VertexBuffer* vertex_buffer) override;

    IndexBuffer* create_index_buffer(const char* name, size_t size, IndexSize index_size) override;
    void upload_index_buffer(IndexBuffer* index_buffer, const void* data, size_t size) override;
    void destroy_index_buffer(IndexBuffer* index_buffer) override;

    Texture* create_texture(const CreateTextureDescriptor& create_texture_descriptor) override;
    void upload_texture(const UploadTextureDescriptor& upload_texture_descriptor) override;
    void destroy_texture(Texture* texture) override;

    HostTexture* create_host_texture(const char* name, TextureFormat format, uint32_t width, uint32_t height) override;
    void read_host_texture(HostTexture* host_texture, void* buffer, size_t size) override;
    void destroy_host_texture(HostTexture* host_texture) override;

    VertexBuffer* acquire_transient_vertex_buffer(const void* data, size_t size) override;
    IndexBuffer* acquire_transient_index_buffer(const void* data, size_t size, IndexSize index_size) override;
    UniformBuffer* acquire_transient_uniform_buffer(const void* data, size_t size) override;

    Task* create_task() override;

    RenderApi get_api() const override;

    // A no-op if `is_debug_names_enabled` in `RenderDescriptor` was false.
    template <typename T>
    void set_debug_name(T* handle, const char* format, ...) const;

    // When resource destroy is queried, the renderer first waits (asynchronously) until all resource dependency
    // semaphores are signaled and only then actually destroys the resource.
    void add_resource_dependency(SharedPtr<TimelineSemaphore> timeline_semaphore);

    // Memory returned by these functions contains pure garbage.
    DeviceAllocation allocate_device_buffer_memory(uint64_t size, uint64_t alignment);
    void deallocate_device_buffer_memory(uint64_t data_index, uint64_t data_offset);
    DeviceAllocation allocate_device_texture_memory(uint64_t size, uint64_t alignment);
    void deallocate_device_texture_memory(uint64_t data_index, uint64_t data_offset);

    // For `UniformBufferVulkan` buffer is not defined explicitly.
    VkBuffer get_transient_buffer() const;

    MemoryResource& persistent_memory_resource;
    MemoryResource& transient_memory_resource;

    const VkAllocationCallbacks allocation_callbacks;
    const VkInstance instance;
    const VkPhysicalDevice physical_device;
    const VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    const VkPhysicalDeviceProperties physical_device_properties;
    
    // Some queue family indices may alias graphics queue family index, if corresponding queue family is not present.
    const uint32_t graphics_queue_family_index;
    const uint32_t compute_queue_family_index;
    const uint32_t transfer_queue_family_index;

    const VkDevice device;

    // Some queues may alias graphics queue, if corresponding queue is not present.
    const VkQueue graphics_queue;
    const VkQueue compute_queue;
    const VkQueue transfer_queue;

    // Submitting to a queue from multiple threads is not allowed. Surround `vkQueueSubmit` with these.
    // Some of the spinlocks may alias graphics spinklock, if corresponding queue is not present.
    const SharedPtr<Spinlock> graphics_queue_spinlock;
    const SharedPtr<Spinlock> compute_queue_spinlock;
    const SharedPtr<Spinlock> transfer_queue_spinlock;

    // Those who depend on resource creation must wait for this semaphore.
    SharedPtr<TimelineSemaphore> semaphore;

    const PFN_vkWaitSemaphoresKHR wait_semaphores;
    const PFN_vkGetSemaphoreCounterValueKHR get_semaphore_counter_value;

private:
    // An instance of this task is returned by `get_task` method.

    class FlushTask : public Task {
    public:
        FlushTask(RenderVulkan& render);

        void run() override;
        const char* get_name() const override;

    private:
        RenderVulkan& m_render;
    };

    // Vertex buffers, index buffers and textures are allocated by buddy allocator.

    struct DeviceData {
        VkDeviceMemory memory;
        void* memory_mapping; // Not `nullptr` if memory is accessible on host (valid for integrated devices).
        RenderBuddyAllocator allocator;
        uint32_t memory_type_index;
    };

    // Upload commands are not submitted on user request, but rather queued and submitted together in one batch.
    // When a resource from such batch is used in a draw call, the draw call submission waits for the batch to
    // complete on device.

    struct BufferUploadCommand {
        BufferVulkan* buffer;

        uint64_t staging_buffer_offset;
        uint64_t staging_buffer_size;

        uint64_t device_buffer_offset;
    };

    struct TextureUploadCommand {
        TextureVulkan* texture;

        uint64_t staging_buffer_offset;
        uint64_t staging_buffer_size;

        uint32_t base_mip_level;
        uint32_t mip_level_count;
        uint32_t base_array_layer;
        uint32_t array_layer_count;
        uint32_t x;
        uint32_t y;
        uint32_t z;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
    };

    // Resources are not destroyed on user request, because some of the frames that use them may still be in flight.
    // Instead, all semaphores and their host values are saved and until device returns the same semaphore values,
    // the resources are preserved.

    struct DestroyCommandDependency {
        WeakPtr<TimelineSemaphore> semaphore;
        uint64_t value;
    };

    struct BufferDestroyCommand {
        Vector<DestroyCommandDependency> dependencies;
        BufferVulkan* buffer;
    };

    struct TextureDestroyCommand {
        Vector<DestroyCommandDependency> dependencies;
        TextureVulkan* texture;
    };
    
    struct HostTextureDestroyCommand {
        Vector<DestroyCommandDependency> dependencies;
        HostTextureVulkan* host_texture;
    };

    struct ImageViewDestroyCommand {
        Vector<DestroyCommandDependency> dependencies;
        VkImageView image_view;
    };

    // When copy commands are submitted, the command buffer, current semaphore value and the end of staging data stack
    // are stored in a queue. Then renderer periodically checks whether the copy commands finished their execution on
    // device and destroys the command buffer and pops from staging data ring buffer.

    struct SubmitData {
        VkCommandBuffer transfer_command_buffer;
        VkCommandBuffer compute_command_buffer;
        VkCommandBuffer graphics_command_buffer;

        uint64_t semaphore_value;
        uint64_t staging_data_end;
    };

    VkInstance create_instance(const RenderDescriptor& descriptor);
    VkPhysicalDevice create_physical_device();

    VkPhysicalDeviceMemoryProperties get_physical_device_memory_properties();
    VkPhysicalDeviceProperties get_physical_device_properties();

    uint32_t get_graphics_queue_family_index();
    uint32_t get_compute_queue_family_index();
    uint32_t get_transfer_queue_family_index();

    VkDevice create_device(const RenderDescriptor& descriptor);

    VkQueue create_graphics_queue();
    VkQueue create_compute_queue();
    VkQueue create_transfer_queue();

    SharedPtr<Spinlock> create_graphics_queue_spinlock();
    SharedPtr<Spinlock> create_compute_queue_spinlock();
    SharedPtr<Spinlock> create_transfer_queue_spinlock();

    PFN_vkWaitSemaphoresKHR create_wait_semaphores();
    PFN_vkGetSemaphoreCounterValueKHR create_get_semaphore_counter_value();

    VkDebugUtilsMessengerEXT create_debug_messsenger(const RenderDescriptor& descriptor);
    PFN_vkSetDebugUtilsObjectNameEXT create_set_object_name(const RenderDescriptor& descriptor);

    VkCommandPool create_graphics_command_pool();
    VkCommandPool create_compute_command_pool();
    VkCommandPool create_transfer_command_pool();

    VkBuffer create_staging_buffer(const RenderDescriptor& render_descriptor);
    VkDeviceMemory allocate_staging_memory();

    // Allocate `size` bytes from staging memory. `size` must be not greater than `m_staging_buffer_size / 2`.
    // Smaller `size` leads to lower staging memory fragmentation. Might cause one or more synchrnonous flushes.
    uint64_t allocate_from_staging_memory(uint64_t size, uint64_t alignment);

    // Allocate from `min` to `max` bytes from staging memory. `min` must be not greater than `m_staging_buffer_size / 2`.
    // Smaller `min` leads to lower staging memory fragmentation. Might cause one or more synchrnonous flushes
    // unless `min` is 0. First value is the number of bytes allocated, second is the offset.
    std::pair<uint64_t, uint64_t> allocate_from_staging_memory(uint64_t min, uint64_t max, uint64_t alignment);

    // Wait until some more staging memory is available. Might cause a synchrnonous flush.
    void wait_for_staging_memory();

    void* map_memory(VkDeviceMemory memory);

    uint32_t compute_buffer_memory_index(VkMemoryPropertyFlags properties);
    uint32_t compute_texture_memory_index(VkMemoryPropertyFlags properties);

    VkBuffer create_transient_buffer(const RenderDescriptor& render_descriptor);
    VkDeviceMemory allocate_transient_memory();
    uint64_t allocate_from_transient_memory(uint64_t size, uint64_t alignment);

    // Returns UINT32_MAX on failure.
    uint32_t find_memory_type(uint32_t memory_type_mask, VkMemoryPropertyFlags property_mask);

    Vector<DestroyCommandDependency> get_destroy_command_dependencies();
    bool wait_for_dependencies(Vector<DestroyCommandDependency>& dependencies);

    // Check which transfer queue submits have completed and pop from staging ring buffer.
    // Check which pending destroy commands are safe to execute. Submit upload commands.
    void flush();

    void process_completed_submits();
    void destroy_queued_buffers();
    void destroy_queued_textures();
    void destroy_queued_host_textures();
    void destroy_queued_image_views();

    void submit_upload_commands();

    std::pair<VkCommandBuffer, uint64_t> upload_resources();
    uint64_t upload_buffers(VkCommandBuffer transfer_command_buffer);
    uint64_t upload_textures(VkCommandBuffer transfer_command_buffer);

    uint64_t staging_buffer_max(uint64_t lhs, uint64_t rhs) const;
    
    VkCommandBuffer acquire_graphics_ownership();
    void acquire_graphics_ownership_buffers(VkCommandBuffer graphics_command_buffer);
    void acquire_graphics_ownership_textures(VkCommandBuffer graphics_command_buffer);

    VkDebugUtilsMessengerEXT m_debug_messenger;
    PFN_vkSetDebugUtilsObjectNameEXT m_set_object_name;

    // Staging buffer is accessed on host and transfer queue.
    VkBuffer m_staging_buffer;
    VkDeviceMemory m_staging_memory;
    void* m_staging_memory_mapping;
    uint64_t m_staging_buffer_size;
    std::atomic_uint64_t m_staging_data_begin;
    std::atomic_uint64_t m_staging_data_end;

    // Vertex/index buffer ranges are first accessed on transfer queue
    // but later ownership is transfered to graphics queue.
    Vector<DeviceData> m_buffer_device_data;
    uint64_t m_buffer_allocation_size;
    uint64_t m_buffer_block_size;
    uint32_t m_buffer_memory_indices[2];
    std::mutex m_buffer_mutex;

    // Texture mip levels are first accessed on transfer queue
    // but later ownership is transfered to graphics queue.
    Vector<DeviceData> m_texture_device_data;
    uint64_t m_texture_allocation_size;
    uint64_t m_texture_block_size;
    uint32_t m_texture_memory_index;
    std::mutex m_texture_mutex;

    // Only memory index because each host texture has its own device memory.
    uint32_t m_host_texture_memory_index;

    // Transient buffer is accessed on host and graphics queue.
    VkBuffer m_transient_buffer;
    VkDeviceMemory m_transient_memory;
    void* m_transient_memory_mapping;
    uint64_t m_transient_buffer_size;
    std::atomic_uint64_t m_transient_data_end;

    // Dependencies we wait for before destroying any resource.
    Vector<WeakPtr<TimelineSemaphore>> m_resource_dependencies;
    std::mutex m_resource_dependency_mutex;

    // Queued upload commands that will be submitted together in one batch.
    Vector<BufferUploadCommand> m_buffer_upload_commands;
    std::mutex m_buffer_upload_command_mutex;
    Vector<TextureUploadCommand> m_texture_upload_commands;
    std::mutex m_texture_upload_command_mutex;

    // Queued resource destroy commands waiting for its dependencies to signal.
    Queue<BufferDestroyCommand> m_buffer_destroy_commands;
    std::mutex m_buffer_destroy_command_mutex;
    Queue<TextureDestroyCommand> m_texture_destroy_commands;
    std::mutex m_texture_destroy_command_mutex;
    Queue<HostTextureDestroyCommand> m_host_texture_destroy_commands;
    std::mutex m_host_texture_destroy_command_mutex;
    Queue<ImageViewDestroyCommand> m_image_view_destroy_commands;
    std::mutex m_image_view_destroy_command_mutex;

    // Submitted command buffers waiting for semaphore to signal.
    Queue<SubmitData> m_submit_data;
    std::recursive_mutex m_submit_data_mutex;

    // This semaphore is used to synchronize transfer/compute/graphics ownership transfer submits.
    UniquePtr<TimelineSemaphore> m_intermediate_semaphore;

    // Transfer command buffers are used to submit copy commands, command buffers from other queues are used for queue
    // ownership transfer. Some of the command pools may alias graphics command pool, if corresponding queue is not present.
    VkCommandPool m_graphics_command_pool;
    VkCommandPool m_compute_command_pool;
    VkCommandPool m_transfer_command_pool;
};

} // namespace kw
