#pragma once

#include "render/buddy_allocator.h"
#include "render/render.h"

#include <core/enum.h>
#include <core/queue.h>
#include <core/vector.h>

#include <concurrency/spinlock.h>

#include <vulkan/vulkan.h>

#include <mutex>

namespace kw {

class TimelineSemaphore;

enum class BufferFlagsVulkan {
    NONE      = 0,

    // Size in bits of each index.
    INDEX16   = 1 << 0,
    INDEX32   = 1 << 1,

    // Buffer can't and shouldn't be destroyed.
    // `device_data_index` is unused.
    // `device_data_offset` is the offset within `buffer`.
    TRANSIENT = 1 << 2,
};

KW_DEFINE_ENUM_BITMASK(BufferFlagsVulkan);

class BufferVulkan {
public:
    VkBuffer buffer;
    BufferFlagsVulkan buffer_flags;
    uint64_t transfer_semaphore_value;
    uint64_t device_data_index : 16;
    uint64_t device_data_offset : 48;
};

static_assert(sizeof(BufferVulkan) == 32);

class TextureVulkan {
public:
    VkImage image;
    VkImageView image_view;
    uint64_t transfer_semaphore_value;
    uint64_t device_data_index : 16;
    uint64_t device_data_offset : 48;
};

static_assert(sizeof(TextureVulkan) == 32);

class RenderVulkan : public Render {
public:
    struct DeviceAllocation {
        VkDeviceMemory memory;
        uint64_t data_index : 16;
        uint64_t data_offset : 48;
    };

    RenderVulkan(const RenderDescriptor& descriptor);
    ~RenderVulkan() override;

    VertexBuffer create_vertex_buffer(const BufferDescriptor& buffer_descriptor) override;
    void destroy_vertex_buffer(VertexBuffer vertex_buffer) override;

    IndexBuffer create_index_buffer(const BufferDescriptor& buffer_descriptor) override;
    void destroy_index_buffer(IndexBuffer index_buffer) override;

    VertexBuffer acquire_transient_vertex_buffer(const void* data, size_t size) override;
    IndexBuffer acquire_transient_index_buffer(const void* data, size_t size, IndexSize index_size) override;
    UniformBuffer acquire_transient_uniform_buffer(const void* data, size_t size) override;

    Texture create_texture(const TextureDescriptor& texture_descriptor) override;
    void destroy_texture(Texture texture) override;

    void flush() override;

    RenderApi get_api() const override;

    // A no-op if `is_debug_names_enabled` in `RenderDescriptor` was false.
    template <typename T>
    void set_debug_name(T* handle, const char* format, ...) const;

    // Returns `UINT32_MAX` if memory type is not found.
    uint32_t find_memory_type(uint32_t memory_type_mask, VkMemoryPropertyFlags property_mask);

    // When resource destroy is queried, the renderer first waits until all resource dependency semaphores are signaled
    // and only then actually destroys the resource.
    void add_resource_dependency(std::shared_ptr<TimelineSemaphore> timeline_semaphore);

    // Memory returned by these functions contains pure garbage.
    DeviceAllocation allocate_device_buffer_memory(uint64_t size, uint64_t alignment);
    void deallocate_device_buffer_memory(uint64_t data_index, uint64_t data_offset);
    DeviceAllocation allocate_device_texture_memory(uint64_t size, uint64_t alignment);
    void deallocate_device_texture_memory(uint64_t data_index, uint64_t data_offset);

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
    const std::shared_ptr<Spinlock> graphics_queue_spinlock;
    const std::shared_ptr<Spinlock> compute_queue_spinlock;
    const std::shared_ptr<Spinlock> transfer_queue_spinlock;

    // Those who depend on resource creation must wait for this semaphore.
    std::shared_ptr<TimelineSemaphore> semaphore;

private:
    // Vertex buffers, index buffers and textures are allocated by buddy allocator.

    struct DeviceData {
        VkDeviceMemory memory;
        RenderBuddyAllocator allocator;
        uint32_t memory_type_index;
    };

    // Copy commands are not submitted on user request, but rather queued and submitted together in one batch.
    // When such batch is submitted, all frame graphs wait for its execution on device, so no synchronization from
    // user is needed.

    struct BufferCopyCommand {
        uint64_t staging_buffer_offset;
        uint64_t staging_buffer_size;

        VkBuffer buffer;
    };

    struct TextureCopyCommand {
        uint64_t staging_buffer_offset;
        uint64_t staging_buffer_size;

        VkImage image;
        VkImageAspectFlags aspect_mask;
        uint32_t array_size;
        uint32_t mip_levels;
        uint32_t width;
        uint32_t height;
        uint32_t depth;

        // size_t offsets[array_size][mip_levels];
        size_t* offsets;
    };

    // Resources are not destroyed on user request, because some of the frames that use them may still be in flight.
    // Instead, all semaphores and their host values are saved and until device returns the same semaphore values,
    // the resources are preserved.

    struct DestroyCommandDependency {
        std::weak_ptr<TimelineSemaphore> semaphore;
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

    // When copy commands are submitted, the command buffer, current semaphore value and the end of staging data stack
    // are stored in a queue. Then renderer periodically checks whether the copy commands finished their execution on
    // device and destroys the command buffer and pops from staging data stack.

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

    std::shared_ptr<Spinlock> create_graphics_queue_spinlock();
    std::shared_ptr<Spinlock> create_compute_queue_spinlock();
    std::shared_ptr<Spinlock> create_transfer_queue_spinlock();

    PFN_vkWaitSemaphoresKHR create_wait_semaphores();

    VkDebugUtilsMessengerEXT create_debug_messsenger(const RenderDescriptor& descriptor);
    PFN_vkSetDebugUtilsObjectNameEXT create_set_object_name(const RenderDescriptor& descriptor);

    VkCommandPool create_graphics_command_pool();
    VkCommandPool create_compute_command_pool();
    VkCommandPool create_transfer_command_pool();

    VkBuffer create_staging_buffer(const RenderDescriptor& render_descriptor);
    VkDeviceMemory allocate_staging_memory();
    uint64_t allocate_from_staging_memory(uint64_t size, uint64_t alignment);

    uint32_t compute_buffer_memory_index(VkMemoryPropertyFlags properties);

    BufferVulkan* create_buffer_vulkan(const BufferDescriptor& buffer_descriptor, VkBufferUsageFlags usage);
    void destroy_buffer_vulkan(BufferVulkan* buffer);

    VkBuffer create_transient_buffer(const RenderDescriptor& render_descriptor);
    VkDeviceMemory allocate_transient_memory();
    void* map_transient_memory();
    uint64_t allocate_from_transient_memory(uint64_t size, uint64_t alignment);

    BufferVulkan* acquire_transient_buffer_vulkan(const void* data, size_t size, size_t alignment, BufferFlagsVulkan flags);

    uint32_t compute_texture_memory_index(VkMemoryPropertyFlags properties);

    TextureVulkan* create_texture_vulkan(const TextureDescriptor& texture_descriptor);
    void destroy_texture_vulkan(TextureVulkan* texture);

    Vector<DestroyCommandDependency> get_destroy_command_dependencies();

    void process_completed_submits();
    void destroy_queued_buffers();
    void destroy_queued_textures();
    void submit_copy_commands();

    PFN_vkWaitSemaphoresKHR m_wait_semaphores;

    VkDebugUtilsMessengerEXT m_debug_messenger;
    PFN_vkSetDebugUtilsObjectNameEXT m_set_object_name;

    // Staging buffer is accessed on host and transfer queue.
    VkBuffer m_staging_buffer;
    VkDeviceMemory m_staging_memory;
    uint64_t m_staging_buffer_size;
    std::atomic_uint64_t m_staging_data_begin;
    std::atomic_uint64_t m_staging_data_end;

    // Vertex/index buffers are first accessed on transfer queue
    // but later ownership is transfered to graphics queue.
    Vector<DeviceData> m_buffer_device_data;
    uint64_t m_buffer_allocation_size;
    uint64_t m_buffer_block_size;
    uint32_t m_buffer_memory_indices[2];
    std::mutex m_buffer_mutex;

    // Transient buffer is accessed on host and graphics queue.
    VkBuffer m_transient_buffer;
    VkDeviceMemory m_transient_memory;
    void* m_transient_memory_mapping;
    uint64_t m_transient_buffer_size;
    std::atomic_uint64_t m_transient_data_end;

    // Textures are first accessed on transfer queue
    // but later ownership is transfered to graphics queue.
    Vector<DeviceData> m_texture_device_data;
    uint64_t m_texture_allocation_size;
    uint64_t m_texture_block_size;
    uint32_t m_texture_memory_indices[2];
    std::mutex m_texture_mutex;

    // Dependencies we wait for before destroying any resource.
    Vector<std::weak_ptr<TimelineSemaphore>> m_resource_dependencies;
    std::mutex m_resource_dependency_mutex;

    // Queued copy commands that will be submitted together in one batch.
    Vector<BufferCopyCommand> m_buffer_copy_commands;
    std::mutex m_buffer_copy_command_mutex;
    Vector<TextureCopyCommand> m_texture_copy_commands;
    std::mutex m_texture_copy_command_mutex;

    // Queued destroy commands each waiting for its dependencies to signal.
    Queue<BufferDestroyCommand> m_buffer_destroy_commands;
    std::mutex m_buffer_destroy_command_mutex;
    Queue<TextureDestroyCommand> m_texture_destroy_commands;
    std::mutex m_texture_destroy_command_mutex;

    // Submitted command buffers waiting for semaphore to signal.
    Queue<SubmitData> m_submit_data;
    std::recursive_mutex m_submit_data_mutex;

    // This semaphore is used to synchronize transfer/compute/graphics ownership transfer submits.
    std::unique_ptr<TimelineSemaphore> m_intermediate_semaphore;

    // Transfer command buffers are used to submit copy commands, command buffers from other queues are used for queue
    // ownership transfer. Some of the command pools may alias graphics command pool, if corresponding queue is not present.
    VkCommandPool m_graphics_command_pool;
    VkCommandPool m_compute_command_pool;
    VkCommandPool m_transfer_command_pool;
};

} // namespace kw
