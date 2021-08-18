#include "render/vulkan/render_vulkan.h"
#include "render/vulkan/timeline_semaphore.h"
#include "render/vulkan/vulkan_utils.h"

#include <core/debug/assert.h>
#include <core/debug/log.h>
#include <core/math/scalar.h>

#include <SDL2/SDL_vulkan.h>

#include <cstdio>

namespace kw {

static void* vk_alloc(void* userdata, size_t size, size_t alignment, VkSystemAllocationScope allocation_scope) {
    MemoryResource* memory_resource = static_cast<MemoryResource*>(userdata);
    KW_ASSERT(memory_resource != nullptr);

    return memory_resource->allocate(size, alignment);
}

static void* vk_realloc(void* userdata, void* memory, size_t size, size_t alignment, VkSystemAllocationScope allocation_scope) {
    MemoryResource* memory_resource = static_cast<MemoryResource*>(userdata);
    KW_ASSERT(memory_resource != nullptr);

    return memory_resource->reallocate(memory, size, alignment);
}

static void vk_free(void* userdata, void* memory) {
    MemoryResource* memory_resource = static_cast<MemoryResource*>(userdata);
    KW_ASSERT(memory_resource != nullptr);

    memory_resource->deallocate(memory);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* userdata) {
    Log::print("%s: %s", callback_data->pMessageIdName, callback_data->pMessage);
    return VK_FALSE;
}

VertexBufferVulkan::VertexBufferVulkan(size_t size_, VkBuffer buffer_, uint64_t device_data_index_, uint64_t device_data_offset_) {
    KW_ASSERT(size_ < (1ull << 63));

    m_size = size_;
    m_available_size = 0;
    m_is_transient = 0;
    
    buffer = buffer_;
    transfer_semaphore_value = 0;
    device_data_index = device_data_index_;
    device_data_offset = device_data_offset_;
}

VertexBufferVulkan::VertexBufferVulkan(size_t size_, VkBuffer buffer_, uint64_t transient_buffer_offset_) {
    KW_ASSERT(size_ < (1ull << 63));

    m_size = size_;
    m_available_size = 0;
    m_is_transient = 1;

    buffer = buffer_;
    transfer_semaphore_value = 0;
    transient_buffer_offset = transient_buffer_offset_;
}

IndexBufferVulkan::IndexBufferVulkan(size_t size_, IndexSize index_size_, VkBuffer buffer_, uint64_t device_data_index_, uint64_t device_data_offset_) {
    KW_ASSERT(size_ < (1ull << 63));
    KW_ASSERT(static_cast<uint64_t>(index_size_) < (1 << 1));

    m_size = size_;
    m_index_size = static_cast<uint64_t>(index_size_);
    m_available_size = 0;
    m_is_transient = 0;

    buffer = buffer_;
    transfer_semaphore_value = 0;
    device_data_index = device_data_index_;
    device_data_offset = device_data_offset_;
}

IndexBufferVulkan::IndexBufferVulkan(size_t size_, IndexSize index_size_, VkBuffer buffer_, uint64_t transient_buffer_offset_) {
    KW_ASSERT(size_ < (1ull << 63));
    KW_ASSERT(static_cast<uint64_t>(index_size_) < (1 << 1));

    m_size = size_;
    m_index_size = static_cast<uint64_t>(index_size_);
    m_available_size = 0;
    m_is_transient = 1;

    buffer = buffer_;
    transfer_semaphore_value = 0;
    transient_buffer_offset = transient_buffer_offset_;
}

UniformBufferVulkan::UniformBufferVulkan(size_t size_, uint64_t transient_buffer_offset_) {
    m_size = size_;
    transient_buffer_offset = transient_buffer_offset_;
}

TextureVulkan::TextureVulkan(TextureType type, TextureFormat format,
                             uint32_t mip_level_count, uint32_t array_layer_count, uint32_t width, uint32_t height, uint32_t depth,
                             VkImage image_, uint64_t device_data_index_, uint64_t device_data_offset_)
    : image(image_)
    , image_view(VK_NULL_HANDLE)
    , transfer_semaphore_value(0)
    , device_data_index(device_data_index_)
    , device_data_offset(device_data_offset_)
{
    KW_ASSERT(static_cast<uint32_t>(type) < (1 << 5));
    KW_ASSERT(static_cast<uint32_t>(format) < (1 << 6));
    KW_ASSERT(mip_level_count < (1 << 5));
    KW_ASSERT(array_layer_count < (1 << 11));

    m_type = type;
    m_format = format;
    m_mip_level_count = mip_level_count;
    m_array_layer_count = array_layer_count;
    m_available_mip_level_count = 0;
    m_width = width;
    m_height = height;
    m_depth = depth;
}


HostTextureVulkan::HostTextureVulkan(TextureFormat format, uint32_t width, uint32_t height,
                                     VkBuffer buffer_, VkDeviceMemory device_memory_, void* memory_mapping_, size_t size_)
    : buffer(buffer_)
    , device_memory(device_memory_)
    , memory_mapping(memory_mapping_)
    , size(size_)
{
    m_format = format;
    m_width = width;
    m_height = height;
}

RenderVulkan::FlushTask::FlushTask(RenderVulkan& render)
    : m_render(render)
{
}

void RenderVulkan::FlushTask::run() {
    m_render.flush();
}

const char* RenderVulkan::FlushTask::get_name() const {
    return "Render Flush";
}

#define DEFINE_SET_DEBUG_NAME(Type, Enum)                                                        \
template <>                                                                                      \
void RenderVulkan::set_debug_name(Type handle, const char* format, ...) const {                  \
    if (m_set_object_name != nullptr) {                                                          \
        char name_buffer[64];                                                                    \
                                                                                                 \
        va_list args;                                                                            \
        va_start(args, format);                                                                  \
        vsnprintf(name_buffer, sizeof(name_buffer), format, args);                               \
        va_end(args);                                                                            \
                                                                                                 \
        VkDebugUtilsObjectNameInfoEXT debug_utils_object_name_info{};                            \
        debug_utils_object_name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT; \
        debug_utils_object_name_info.objectType = Enum;                                          \
        debug_utils_object_name_info.objectHandle = reinterpret_cast<uint64_t>(handle);          \
        debug_utils_object_name_info.pObjectName = name_buffer;                                  \
                                                                                                 \
        VK_ERROR(m_set_object_name(device, &debug_utils_object_name_info));                      \
    }                                                                                            \
}

DEFINE_SET_DEBUG_NAME(VkQueue, VK_OBJECT_TYPE_QUEUE)
DEFINE_SET_DEBUG_NAME(VkSemaphore, VK_OBJECT_TYPE_SEMAPHORE)
DEFINE_SET_DEBUG_NAME(VkCommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER)
DEFINE_SET_DEBUG_NAME(VkFence, VK_OBJECT_TYPE_FENCE)
DEFINE_SET_DEBUG_NAME(VkDeviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY)
DEFINE_SET_DEBUG_NAME(VkBuffer, VK_OBJECT_TYPE_BUFFER)
DEFINE_SET_DEBUG_NAME(VkImage, VK_OBJECT_TYPE_IMAGE)
DEFINE_SET_DEBUG_NAME(VkEvent, VK_OBJECT_TYPE_EVENT)
DEFINE_SET_DEBUG_NAME(VkQueryPool, VK_OBJECT_TYPE_QUERY_POOL)
DEFINE_SET_DEBUG_NAME(VkBufferView, VK_OBJECT_TYPE_BUFFER_VIEW)
DEFINE_SET_DEBUG_NAME(VkImageView, VK_OBJECT_TYPE_IMAGE_VIEW)
DEFINE_SET_DEBUG_NAME(VkShaderModule, VK_OBJECT_TYPE_SHADER_MODULE)
DEFINE_SET_DEBUG_NAME(VkPipelineCache, VK_OBJECT_TYPE_PIPELINE_CACHE)
DEFINE_SET_DEBUG_NAME(VkPipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT)
DEFINE_SET_DEBUG_NAME(VkRenderPass, VK_OBJECT_TYPE_RENDER_PASS)
DEFINE_SET_DEBUG_NAME(VkPipeline, VK_OBJECT_TYPE_PIPELINE)
DEFINE_SET_DEBUG_NAME(VkDescriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT)
DEFINE_SET_DEBUG_NAME(VkSampler, VK_OBJECT_TYPE_SAMPLER)
DEFINE_SET_DEBUG_NAME(VkDescriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL)
DEFINE_SET_DEBUG_NAME(VkDescriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET)
DEFINE_SET_DEBUG_NAME(VkFramebuffer, VK_OBJECT_TYPE_FRAMEBUFFER)
DEFINE_SET_DEBUG_NAME(VkCommandPool, VK_OBJECT_TYPE_COMMAND_POOL)
DEFINE_SET_DEBUG_NAME(VkSwapchainKHR, VK_OBJECT_TYPE_SWAPCHAIN_KHR)

#undef DEFINE_SET_DEBUG_NAME

RenderVulkan::RenderVulkan(const RenderDescriptor& descriptor)
    : persistent_memory_resource(*descriptor.persistent_memory_resource)
    , transient_memory_resource(*descriptor.transient_memory_resource)
    , allocation_callbacks{ descriptor.persistent_memory_resource, &vk_alloc, &vk_realloc, &vk_free }
    , instance(create_instance(descriptor))
    , physical_device(create_physical_device())
    , physical_device_memory_properties(get_physical_device_memory_properties())
    , physical_device_properties(get_physical_device_properties())
    , graphics_queue_family_index(get_graphics_queue_family_index())
    , compute_queue_family_index(get_compute_queue_family_index())
    , transfer_queue_family_index(get_transfer_queue_family_index())
    , device(create_device(descriptor))
    , graphics_queue(create_graphics_queue())
    , compute_queue(create_compute_queue())
    , transfer_queue(create_transfer_queue())
    , graphics_queue_spinlock(create_graphics_queue_spinlock())
    , compute_queue_spinlock(create_compute_queue_spinlock())
    , transfer_queue_spinlock(create_transfer_queue_spinlock())
    , semaphore(allocate_shared<TimelineSemaphore>(persistent_memory_resource, this))
    , wait_semaphores(create_wait_semaphores())
    , get_semaphore_counter_value(create_get_semaphore_counter_value())
    , m_debug_messenger(create_debug_messsenger(descriptor))
    , m_set_object_name(create_set_object_name(descriptor))
    , m_staging_buffer(create_staging_buffer(descriptor))
    , m_staging_memory(allocate_staging_memory())
    , m_staging_memory_mapping(map_memory(m_staging_memory))
    , m_staging_buffer_size(descriptor.staging_buffer_size)
    , m_staging_data_begin(0)
    , m_staging_data_end(0)
    , m_buffer_device_data(persistent_memory_resource)
    , m_buffer_allocation_size(descriptor.buffer_allocation_size)
    , m_buffer_block_size(descriptor.buffer_block_size)
    , m_buffer_memory_indices{
        compute_buffer_memory_index(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
        compute_buffer_memory_index(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    }
    , m_texture_device_data(persistent_memory_resource)
    , m_texture_allocation_size(descriptor.texture_allocation_size)
    , m_texture_block_size(descriptor.texture_block_size)
    , m_texture_memory_index(compute_texture_memory_index(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
    , m_host_texture_memory_index(
        compute_buffer_memory_index(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
    )
    , m_transient_buffer(create_transient_buffer(descriptor))
    , m_transient_memory(allocate_transient_memory())
    , m_transient_memory_mapping(map_memory(m_transient_memory))
    , m_transient_buffer_size(descriptor.transient_buffer_size)
    , m_transient_data_end(0)
    , m_resource_dependencies(persistent_memory_resource)
    , m_buffer_destroy_commands(MemoryResourceAllocator<BufferDestroyCommand>(persistent_memory_resource))
    , m_texture_destroy_commands(MemoryResourceAllocator<TextureDestroyCommand>(persistent_memory_resource))
    , m_host_texture_destroy_commands(MemoryResourceAllocator<TextureDestroyCommand>(persistent_memory_resource))
    , m_image_view_destroy_commands(MemoryResourceAllocator<TextureDestroyCommand>(persistent_memory_resource))
    , m_buffer_upload_commands(persistent_memory_resource)
    , m_texture_upload_commands(persistent_memory_resource)
    , m_submit_data(MemoryResourceAllocator<SubmitData>(persistent_memory_resource))
    , m_intermediate_semaphore(allocate_unique<TimelineSemaphore>(persistent_memory_resource, this))
    , m_graphics_command_pool(create_graphics_command_pool())
    , m_compute_command_pool(create_compute_command_pool())
    , m_transfer_command_pool(create_transfer_command_pool())
{
    VK_NAME(*this, graphics_queue, "Graphics queue");

    if (compute_queue != graphics_queue) {
        VK_NAME(*this, compute_queue, "Async compute queue");
    }
     
    if (transfer_queue != graphics_queue) {
        VK_NAME(*this, transfer_queue, "Transfer queue");
    }

    VK_NAME(*this, semaphore->semaphore, "Transfer semaphore");

    VK_NAME(*this, m_intermediate_semaphore->semaphore, "Intermediate semaphore");

    m_buffer_device_data.reserve(4);
    m_texture_device_data.reserve(4);

    m_resource_dependencies.reserve(4);

    m_buffer_upload_commands.reserve(32);
    m_texture_upload_commands.reserve(32);
}

RenderVulkan::~RenderVulkan() {
    VK_ERROR(vkDeviceWaitIdle(device), "Failed to wait idle.");

    while (!m_submit_data.empty()) {
        SubmitData& submit_data = m_submit_data.front();
        
        if (submit_data.graphics_command_buffer != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(device, m_graphics_command_pool, 1, &submit_data.graphics_command_buffer);
        }

        if (submit_data.compute_command_buffer != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(device, m_compute_command_pool, 1, &submit_data.compute_command_buffer);
        }
        
        KW_ASSERT(submit_data.transfer_command_buffer != VK_NULL_HANDLE);
        vkFreeCommandBuffers(device, m_transfer_command_pool, 1, &submit_data.transfer_command_buffer);

        m_submit_data.pop();
    }

    m_intermediate_semaphore.reset();

    if (m_transfer_command_pool != m_graphics_command_pool) {
        vkDestroyCommandPool(device, m_transfer_command_pool, &allocation_callbacks);
    }

    if (m_compute_command_pool != m_graphics_command_pool) {
        vkDestroyCommandPool(device, m_compute_command_pool, &allocation_callbacks);
    }

    vkDestroyCommandPool(device, m_graphics_command_pool, &allocation_callbacks);

    while (!m_image_view_destroy_commands.empty()) {
        ImageViewDestroyCommand& image_view_destroy_command = m_image_view_destroy_commands.front();
        vkDestroyImageView(device, image_view_destroy_command.image_view, &allocation_callbacks);
        m_image_view_destroy_commands.pop();
    }

    while (!m_host_texture_destroy_commands.empty()) {
        HostTextureDestroyCommand& host_texture_destroy_command = m_host_texture_destroy_commands.front();
        vkUnmapMemory(device, host_texture_destroy_command.host_texture->device_memory);
        vkFreeMemory(device, host_texture_destroy_command.host_texture->device_memory, &allocation_callbacks);
        vkDestroyBuffer(device, host_texture_destroy_command.host_texture->buffer, &allocation_callbacks);
        persistent_memory_resource.deallocate(host_texture_destroy_command.host_texture);
        m_host_texture_destroy_commands.pop();
    }
    
    while (!m_texture_destroy_commands.empty()) {
        TextureDestroyCommand& texture_destroy_command = m_texture_destroy_commands.front();
        vkDestroyImageView(device, texture_destroy_command.texture->image_view, &allocation_callbacks);
        vkDestroyImage(device, texture_destroy_command.texture->image, &allocation_callbacks);
        deallocate_device_texture_memory(texture_destroy_command.texture->device_data_index, texture_destroy_command.texture->device_data_offset);
        persistent_memory_resource.deallocate(texture_destroy_command.texture);
        m_texture_destroy_commands.pop();
    }

    while (!m_buffer_destroy_commands.empty()) {
        BufferDestroyCommand& buffer_destroy_command = m_buffer_destroy_commands.front();
        
        vkDestroyBuffer(device, buffer_destroy_command.buffer->buffer, &allocation_callbacks);
        
        deallocate_device_buffer_memory(buffer_destroy_command.buffer->device_data_index, buffer_destroy_command.buffer->device_data_offset);

        // `VertexBufferVulkan` and `IndexBufferVulkan` have the same size and the same offset to `BufferVulkan`.
        persistent_memory_resource.deallocate(static_cast<VertexBufferVulkan*>(buffer_destroy_command.buffer));

        m_buffer_destroy_commands.pop();
    }

    m_resource_dependencies.clear();

    for (DeviceData& device_data : m_texture_device_data) {
        vkFreeMemory(device, device_data.memory, &allocation_callbacks);
    }

    vkUnmapMemory(device, m_transient_memory);

    vkDestroyBuffer(device, m_transient_buffer, &allocation_callbacks);
    vkFreeMemory(device, m_transient_memory, &allocation_callbacks);

    for (DeviceData& device_data : m_buffer_device_data) {
        if (device_data.memory_mapping != nullptr) {
            vkUnmapMemory(device, device_data.memory);
        }

        vkFreeMemory(device, device_data.memory, &allocation_callbacks);
    }

    vkUnmapMemory(device, m_staging_memory);

    vkDestroyBuffer(device, m_staging_buffer, &allocation_callbacks);
    vkFreeMemory(device, m_staging_memory, &allocation_callbacks);

    if (m_debug_messenger != nullptr) {
        auto vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        KW_ERROR(vkDestroyDebugUtilsMessengerEXT != nullptr, "Failed to get vkDestroyDebugUtilsMessengerEXT function.");

        vkDestroyDebugUtilsMessengerEXT(instance, m_debug_messenger, &allocation_callbacks);
    }

    semaphore.reset();

    vkDestroyDevice(device, &allocation_callbacks);

    vkDestroyInstance(instance, &allocation_callbacks);
}

VertexBuffer* RenderVulkan::create_vertex_buffer(const char* name, size_t size) {
    KW_ASSERT(name != nullptr, "Invalid vertex buffer name.");
    KW_ASSERT(size > 0, "Invalid vertex buffer data size.");

    //
    // Create device buffer and query its memory requirements.
    //

    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.flags = 0;
    buffer_create_info.size = static_cast<VkDeviceSize>(size);
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = 0;

    VkBuffer device_buffer;
    VK_ERROR(
        vkCreateBuffer(device, &buffer_create_info, &allocation_callbacks, &device_buffer),
        "Failed to create device vertex buffer \"%s\".", name
    );

    VK_NAME(*this, device_buffer, "Vertex buffer \"%s\"", name);

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, device_buffer, &memory_requirements);

    //
    // Find device memory range to store the buffer and bind the buffer to this range.
    //

    DeviceAllocation device_allocation = allocate_device_buffer_memory(memory_requirements.size, memory_requirements.alignment);
    KW_ASSERT(device_allocation.data_index < m_buffer_device_data.size());

    VK_ERROR(
        vkBindBufferMemory(device, device_buffer, device_allocation.memory, device_allocation.data_offset),
        "Failed to bind device vertex buffer \"%s\" to device memory.", name
    );

    //
    // Create buffer handle and return it.
    //

    return new (persistent_memory_resource.allocate<VertexBufferVulkan>()) VertexBufferVulkan(
        size, device_buffer, device_allocation.data_index, device_allocation.data_offset
    );
}

void RenderVulkan::upload_vertex_buffer(VertexBuffer* vertex_buffer, const void* data, size_t size) {
    KW_ASSERT(vertex_buffer != nullptr, "Invalid vertex buffer.");
    KW_ASSERT(!vertex_buffer->is_transient(), "Transient buffer upload is not allowed.");
    KW_ASSERT(vertex_buffer->get_available_size() + size <= vertex_buffer->get_size(), "Vertex buffer overflow.");
    KW_ASSERT(data != nullptr, "Invalid data.");
    KW_ASSERT(size > 0, "Invalid data size.");

    VertexBufferVulkan* vertex_buffer_vulkan = static_cast<VertexBufferVulkan*>(vertex_buffer);

    void* device_memory_mapping = m_buffer_device_data[vertex_buffer_vulkan->device_data_index].memory_mapping;
    if (device_memory_mapping != nullptr) {
        // Buffer memory is accessible on host, just memcpy to it.
        std::memcpy(static_cast<uint8_t*>(device_memory_mapping) + vertex_buffer_vulkan->device_data_offset, data, size);
    } else {
        uint64_t size_left = static_cast<uint64_t>(size);
        while (size_left > 0) {
            //
            // Synchronously change both the staging memory pointers and buffer upload commands.
            //

            std::lock_guard lock(m_buffer_upload_command_mutex);

            //
            // Find staging memory range to store the buffer data and upload the buffer data to this range.
            //

            auto [allocation_size, allocation_offset] = allocate_from_staging_memory(std::min(1024ull, size_left), size_left, 1);
            KW_ASSERT(allocation_size >= std::min(1024ull, size_left) && allocation_size <= size_left);
            KW_ASSERT(allocation_offset < m_staging_buffer_size);

            std::memcpy(static_cast<uint8_t*>(m_staging_memory_mapping) + allocation_offset, data, allocation_size);

            //
            // Enqueue upload command.
            //

            BufferUploadCommand buffer_upload_command{};
            buffer_upload_command.buffer = vertex_buffer_vulkan;
            buffer_upload_command.staging_buffer_offset = allocation_offset;
            buffer_upload_command.staging_buffer_size = allocation_size;
            buffer_upload_command.device_buffer_offset = vertex_buffer_vulkan->get_available_size();
            m_buffer_upload_commands.push_back(buffer_upload_command);

            //
            // All the vertex buffer users must wait until next transfer is complete.
            //

            KW_ASSERT(vertex_buffer_vulkan->transfer_semaphore_value <= semaphore->value + 1);
            vertex_buffer_vulkan->transfer_semaphore_value = semaphore->value + 1;

            //
            // Advance input buffer.
            //

            data = static_cast<const uint8_t*>(data) + allocation_size;
            size_left -= allocation_size;
        }
    }

    vertex_buffer_vulkan->set_available_size(vertex_buffer_vulkan->get_available_size() + size);
}

void RenderVulkan::destroy_vertex_buffer(VertexBuffer* vertex_buffer) {
    if (vertex_buffer != nullptr) {
        KW_ASSERT(!vertex_buffer->is_transient(), "Transient buffers must not be destroyed manually.");

        std::lock_guard lock(m_buffer_destroy_command_mutex);
        m_buffer_destroy_commands.push(BufferDestroyCommand{ get_destroy_command_dependencies(), static_cast<VertexBufferVulkan*>(vertex_buffer) });
    }
}

IndexBuffer* RenderVulkan::create_index_buffer(const char* name, size_t size, IndexSize index_size) {
    KW_ASSERT(name != nullptr, "Invalid index buffer name.");
    KW_ASSERT(size > 0, "Invalid index buffer data size.");

    //
    // Create device buffer and query its memory requirements.
    //

    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.flags = 0;
    buffer_create_info.size = static_cast<VkDeviceSize>(size);
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = 0;

    VkBuffer device_buffer;
    VK_ERROR(
        vkCreateBuffer(device, &buffer_create_info, &allocation_callbacks, &device_buffer),
        "Failed to create device index buffer \"%s\".", name
    );

    VK_NAME(*this, device_buffer, "Index buffer \"%s\"", name);

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, device_buffer, &memory_requirements);

    //
    // Find device memory range to store the buffer and bind the buffer to this range.
    //

    DeviceAllocation device_allocation = allocate_device_buffer_memory(memory_requirements.size, memory_requirements.alignment);
    KW_ASSERT(device_allocation.data_index < m_buffer_device_data.size());

    VK_ERROR(
        vkBindBufferMemory(device, device_buffer, device_allocation.memory, device_allocation.data_offset),
        "Failed to bind device index buffer \"%s\" to device memory.", name
    );

    //
    // Create buffer handle and return it.
    //

    return new (persistent_memory_resource.allocate<IndexBufferVulkan>()) IndexBufferVulkan(
        size, index_size, device_buffer, device_allocation.data_index, device_allocation.data_offset
    );
}

void RenderVulkan::upload_index_buffer(IndexBuffer* index_buffer, const void* data, size_t size) {
    KW_ASSERT(index_buffer != nullptr, "Invalid index buffer.");
    KW_ASSERT(!index_buffer->is_transient(), "Transient buffer upload is not allowed.");
    KW_ASSERT(index_buffer->get_available_size() + size <= index_buffer->get_size(), "Index buffer overflow.");
    KW_ASSERT(data != nullptr, "Invalid data.");
    KW_ASSERT(size > 0, "Invalid data size.");

    IndexBufferVulkan* index_buffer_vulkan = static_cast<IndexBufferVulkan*>(index_buffer);

    void* device_memory_mapping = m_buffer_device_data[index_buffer_vulkan->device_data_index].memory_mapping;
    if (device_memory_mapping != nullptr) {
        // Buffer memory is accessible on host, just memcpy to it.
        std::memcpy(static_cast<uint8_t*>(device_memory_mapping) + index_buffer_vulkan->device_data_offset, data, size);
    } else {
        uint64_t size_left = static_cast<uint64_t>(size);
        while (size_left > 0) {
            //
            // Synchronously change both the staging memory pointers and buffer upload commands.
            //

            std::lock_guard lock(m_buffer_upload_command_mutex);

            //
            // Find staging memory range to store the buffer data and upload the buffer data to this range.
            //

            auto [allocation_size, allocation_offset] = allocate_from_staging_memory(std::min(1024ull, size_left), size_left, 1);
            KW_ASSERT(allocation_size >= std::min(1024ull, size_left) && allocation_size <= size_left);
            KW_ASSERT(allocation_offset < m_staging_buffer_size);

            std::memcpy(static_cast<uint8_t*>(m_staging_memory_mapping) + allocation_offset, data, allocation_size);

            //
            // Enqueue upload command.
            //

            BufferUploadCommand buffer_upload_command{};
            buffer_upload_command.buffer = index_buffer_vulkan;
            buffer_upload_command.staging_buffer_offset = allocation_offset;
            buffer_upload_command.staging_buffer_size = allocation_size;
            buffer_upload_command.device_buffer_offset = index_buffer_vulkan->get_available_size();
            m_buffer_upload_commands.push_back(buffer_upload_command);

            //
            // All the index buffer users must wait until next transfer is complete.
            //

            KW_ASSERT(index_buffer_vulkan->transfer_semaphore_value <= semaphore->value + 1);
            index_buffer_vulkan->transfer_semaphore_value = semaphore->value + 1;

            //
            // Advance input buffer.
            //

            data = static_cast<const uint8_t*>(data) + allocation_size;
            size_left -= allocation_size;
        }
    }

    index_buffer_vulkan->set_available_size(index_buffer_vulkan->get_available_size() + size);
}

void RenderVulkan::destroy_index_buffer(IndexBuffer* index_buffer) {
    if (index_buffer != nullptr) {
        KW_ASSERT(!index_buffer->is_transient(), "Transient buffers must not be destroyed manually.");

        std::lock_guard lock(m_buffer_destroy_command_mutex);
        m_buffer_destroy_commands.push(BufferDestroyCommand{ get_destroy_command_dependencies(), static_cast<IndexBufferVulkan*>(index_buffer) });
    }
}

Texture* RenderVulkan::create_texture(const CreateTextureDescriptor& texture_descriptor) {
    KW_ASSERT(texture_descriptor.name != nullptr, "Invalid texture name.");

    //
    // Validation.
    //

    uint32_t max_side = std::max(texture_descriptor.width, std::max(texture_descriptor.height, texture_descriptor.depth));

    KW_ERROR(TextureFormatUtils::is_allowed_texture(texture_descriptor.format), "Invalid texture \"%s\" format.", texture_descriptor.name);
    KW_ERROR(texture_descriptor.mip_level_count <= log2(max_side) + 1, "Invalid texture \"%s\" mip levels.", texture_descriptor.name);
    KW_ERROR(texture_descriptor.width > 0 && is_pow2(texture_descriptor.width), "Invalid texture \"%s\" width.", texture_descriptor.name);
    KW_ERROR(texture_descriptor.height > 0 && is_pow2(texture_descriptor.height), "Invalid texture \"%s\" height.", texture_descriptor.name);
    KW_ERROR(texture_descriptor.depth == 0 || is_pow2(texture_descriptor.depth), "Invalid texture \"%s\" depth.", texture_descriptor.name);

    switch (texture_descriptor.type) {
    case TextureType::TEXTURE_2D:
        KW_ERROR(texture_descriptor.array_layer_count <= 1, "Invalid texture \"%s\" array size.", texture_descriptor.name);
        KW_ERROR(texture_descriptor.depth <= 1, "Invalid texture \"%s\" depth.", texture_descriptor.name);
        break;
    case TextureType::TEXTURE_CUBE:
        KW_ERROR(texture_descriptor.array_layer_count == 6, "Invalid texture \"%s\" array size.", texture_descriptor.name);
        KW_ERROR(texture_descriptor.mip_level_count <= 1, "Invalid texture \"%s\" mip levels.", texture_descriptor.name);
        KW_ERROR(texture_descriptor.width == texture_descriptor.height, "Invalid texture \"%s\" size.", texture_descriptor.name);
        KW_ERROR(texture_descriptor.depth <= 1, "Invalid texture \"%s\" depth.", texture_descriptor.name);
        break;
    case TextureType::TEXTURE_3D:
        KW_ERROR(texture_descriptor.array_layer_count <= 1, "Invalid texture \"%s\" array size.", texture_descriptor.name);
        break;
    case TextureType::TEXTURE_2D_ARRAY:
        KW_ERROR(texture_descriptor.depth <= 1, "Invalid texture \"%s\" depth.", texture_descriptor.name);
        break;
    case TextureType::TEXTURE_CUBE_ARRAY:
        KW_ERROR(texture_descriptor.array_layer_count % 6 == 0, "Invalid texture \"%s\" array size.", texture_descriptor.name);
        KW_ERROR(texture_descriptor.mip_level_count <= 1, "Invalid texture \"%s\" mip levels.", texture_descriptor.name);
        KW_ERROR(texture_descriptor.width == texture_descriptor.height, "Invalid texture \"%s\" size.", texture_descriptor.name);
        KW_ERROR(texture_descriptor.depth <= 1, "Invalid texture \"%s\" depth.", texture_descriptor.name);
        break;
    }

    //
    // Compute image type and flags.
    //

    VkImageType image_type;
    VkImageCreateFlags image_create_flags;

    switch (texture_descriptor.type) {
    case TextureType::TEXTURE_2D:
        image_type = VK_IMAGE_TYPE_2D;
        image_create_flags = 0;
        break;
    case TextureType::TEXTURE_CUBE:
        image_type = VK_IMAGE_TYPE_2D;
        image_create_flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        break;
    case TextureType::TEXTURE_3D:
        image_type = VK_IMAGE_TYPE_3D;
        image_create_flags = 0;
        break;
    case TextureType::TEXTURE_2D_ARRAY:
        image_type = VK_IMAGE_TYPE_2D;
        image_create_flags = 0;
        break;
    case TextureType::TEXTURE_CUBE_ARRAY:
        image_type = VK_IMAGE_TYPE_2D;
        image_create_flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        break;
    }

    //
    // Create image.
    //

    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.flags = image_create_flags;
    image_create_info.imageType = image_type;
    image_create_info.format = TextureFormatUtils::convert_format_vulkan(texture_descriptor.format);
    image_create_info.extent.width = texture_descriptor.width;
    image_create_info.extent.height = texture_descriptor.height;
    image_create_info.extent.depth = std::max(texture_descriptor.depth, 1U);
    image_create_info.mipLevels = std::max(texture_descriptor.mip_level_count, 1U);
    image_create_info.arrayLayers = std::max(texture_descriptor.array_layer_count, 1U);
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = nullptr;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage image;
    VK_ERROR(
        vkCreateImage(device, &image_create_info, &allocation_callbacks, &image),
        "Failed to create an image \"%s\".", texture_descriptor.name
    );
    VK_NAME(*this, image, "Texture \"%s\"", texture_descriptor.name);

    //
    // Find device memory range to store the texture and bind the texture to this range.
    //

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device, image, &memory_requirements);

    DeviceAllocation device_allocation = allocate_device_texture_memory(memory_requirements.size, memory_requirements.alignment);
    KW_ASSERT(device_allocation.data_index < m_texture_device_data.size());

    VK_ERROR(
        vkBindImageMemory(device, image, device_allocation.memory, device_allocation.data_offset),
        "Failed to bind texture \"%s\" to device memory.", texture_descriptor.name
    );

    //
    // Create texture handle and return it.
    //

    return new (persistent_memory_resource.allocate<TextureVulkan>()) TextureVulkan(
        texture_descriptor.type,
        texture_descriptor.format,
        std::max(texture_descriptor.mip_level_count, 1U),
        std::max(texture_descriptor.array_layer_count, 1U),
        texture_descriptor.width,
        texture_descriptor.height,
        std::max(texture_descriptor.depth, 1U),
        image,
        device_allocation.data_index,
        device_allocation.data_offset
    );
}

void RenderVulkan::upload_texture(const UploadTextureDescriptor& upload_texture_descriptor) {
    KW_ASSERT(upload_texture_descriptor.texture != nullptr, "Invalid texture.");
    KW_ASSERT(upload_texture_descriptor.data != nullptr, "Invalid data.");
    KW_ASSERT(upload_texture_descriptor.size > 0, "Invalid data size.");

    TextureVulkan* texture_vulkan = static_cast<TextureVulkan*>(upload_texture_descriptor.texture);
    KW_ASSERT(texture_vulkan != nullptr);

    TextureFormat format = texture_vulkan->get_format();
    bool is_compressed = TextureFormatUtils::is_compressed(format);
    uint64_t block_size = TextureFormatUtils::get_texel_size(format);

    uint32_t base_mip_width = std::max(texture_vulkan->get_width() >> upload_texture_descriptor.base_mip_level, 1U);
    uint32_t base_mip_height = std::max(texture_vulkan->get_height() >> upload_texture_descriptor.base_mip_level, 1U);
    uint32_t base_mip_depth = std::max(texture_vulkan->get_depth() >> upload_texture_descriptor.base_mip_level, 1U);

#ifdef KW_DEBUG
    //
    // Validation.
    //

    KW_ASSERT(
        upload_texture_descriptor.mip_level_count <= upload_texture_descriptor.base_mip_level + 1,
        "Invalid number of mip levels."
    );

    KW_ASSERT(
        upload_texture_descriptor.base_mip_level + 1 == texture_vulkan->get_mip_level_count() - texture_vulkan->get_available_mip_level_count(),
        "Mip levels must be uploaded and become available from smallest to largest."
    );

    KW_ASSERT(
        upload_texture_descriptor.base_array_layer + std::max(upload_texture_descriptor.array_layer_count, 1U) <= texture_vulkan->get_array_layer_count(),
        "Invalid number of array layers."
    );

    if (std::max(upload_texture_descriptor.mip_level_count, 1U) + std::max(upload_texture_descriptor.array_layer_count, 1U) == 2) {
        KW_ASSERT(
            upload_texture_descriptor.width > 0 && upload_texture_descriptor.x + upload_texture_descriptor.width <= base_mip_width &&
            upload_texture_descriptor.height > 0 && upload_texture_descriptor.y + upload_texture_descriptor.height <= base_mip_height &&
            upload_texture_descriptor.z + std::max(upload_texture_descriptor.depth, 1U) <= base_mip_depth,
            "Subregion is out of bounds."
        );

        KW_ASSERT(
            std::max(upload_texture_descriptor.depth, 1U) == 1 || upload_texture_descriptor.height == base_mip_height || upload_texture_descriptor.width == base_mip_width,
            "When depth is greater than one, all rows and columns must be specified."
        );

        if (is_compressed) {
            KW_ASSERT(
                upload_texture_descriptor.x % 4 == 0 && upload_texture_descriptor.y % 4 == 0 &&
                (upload_texture_descriptor.width % 4 == 0 || upload_texture_descriptor.x + upload_texture_descriptor.width == base_mip_width) &&
                (upload_texture_descriptor.height % 4 == 0 || upload_texture_descriptor.y + upload_texture_descriptor.height == base_mip_height),
                "Compressed texture subregion must be aligned by block size."
            );

            KW_ASSERT(
                upload_texture_descriptor.height <= 4 || upload_texture_descriptor.width == base_mip_width,
                "When height is greater than one block, all columns must be specified."
            );
        } else {
            KW_ASSERT(
                upload_texture_descriptor.height == 1 || upload_texture_descriptor.width == base_mip_width,
                "When height is greater than one, all columns must be specified."
            );
        }
    } else {
        KW_ASSERT(
            std::max(upload_texture_descriptor.mip_level_count, 1U) == 1 || std::max(upload_texture_descriptor.array_layer_count, 1U) == texture_vulkan->get_array_layer_count(),
            "To upload multiple mip levels, all array layers must be specified."
        );

        KW_ASSERT(
            upload_texture_descriptor.x == 0 &&
            upload_texture_descriptor.y == 0 &&
            upload_texture_descriptor.z == 0 &&
            upload_texture_descriptor.width == base_mip_width &&
            upload_texture_descriptor.height == base_mip_height &&
            std::max(upload_texture_descriptor.depth, 1U) == base_mip_depth,
            "Texture subregion upload for multiple mip levels/array layers is prohibited."
        );
    }

    {
        uint32_t texel_width = upload_texture_descriptor.width;
        uint32_t texel_height = upload_texture_descriptor.height;

        // Texel width/height for uncompressed texture types.
        uint32_t block_width = is_compressed ? (texel_width + 3) / 4 : texel_width;
        uint32_t block_height = is_compressed ? (texel_height + 3) / 4 : texel_height;

        uint32_t depth = std::max(upload_texture_descriptor.depth, 1U);

        uint32_t mip_level_index = upload_texture_descriptor.base_mip_level;
        uint32_t mip_level_count = std::max(upload_texture_descriptor.mip_level_count, 1U);

        uint32_t array_layer_count = std::max(upload_texture_descriptor.array_layer_count, 1U);

        uint64_t total_size = 0;

        while (mip_level_count > 0) {
            total_size += block_size * block_width * block_height * depth * array_layer_count;

            mip_level_index--;
            mip_level_count--;

            texel_width = std::max(texture_vulkan->get_width() >> mip_level_index, 1U);
            texel_height = std::max(texture_vulkan->get_height() >> mip_level_index, 1U);

            block_width = is_compressed ? (texel_width + 3) / 4 : texel_width;
            block_height = is_compressed ? (texel_height + 3) / 4 : texel_height;

            depth = std::max(texture_vulkan->get_depth() >> mip_level_index, 1U);
        }

        KW_ASSERT(upload_texture_descriptor.size == total_size, "Unexpected data size.");
    }
#endif // KW_DEBUG

    if (upload_texture_descriptor.size <= m_staging_buffer_size / 2) {
        TextureUploadCommand texture_upload_command{};

        {
            //
            // Synchronously change both the staging memory pointers and texture upload commands.
            //

            std::lock_guard lock(m_texture_upload_command_mutex);

            //
            // Find staging memory range to store the buffer data and upload the buffer data to this range.
            //

            uint64_t allocation_offset = allocate_from_staging_memory(upload_texture_descriptor.size, block_size);
            KW_ASSERT(allocation_offset < m_staging_buffer_size);

            std::memcpy(static_cast<uint8_t*>(m_staging_memory_mapping) + allocation_offset, upload_texture_descriptor.data, upload_texture_descriptor.size);

            //
            // Enqueue upload command and return.
            //

            texture_upload_command.texture = texture_vulkan;
            texture_upload_command.staging_buffer_offset = allocation_offset;
            texture_upload_command.staging_buffer_size = upload_texture_descriptor.size;
            texture_upload_command.base_mip_level = upload_texture_descriptor.base_mip_level;
            texture_upload_command.mip_level_count = std::max(upload_texture_descriptor.mip_level_count, 1U);
            texture_upload_command.base_array_layer = upload_texture_descriptor.base_array_layer;
            texture_upload_command.array_layer_count = std::max(upload_texture_descriptor.array_layer_count, 1U);
            texture_upload_command.x = upload_texture_descriptor.x;
            texture_upload_command.y = upload_texture_descriptor.y;
            texture_upload_command.z = upload_texture_descriptor.z;
            texture_upload_command.width = upload_texture_descriptor.width;
            texture_upload_command.height = upload_texture_descriptor.height;
            texture_upload_command.depth = std::max(upload_texture_descriptor.depth, 1U);
            m_texture_upload_commands.push_back(texture_upload_command);

            //
            // All the texture users must wait until next transfer is complete.
            //

            KW_ASSERT(texture_vulkan->transfer_semaphore_value <= semaphore->value + 1);
            texture_vulkan->transfer_semaphore_value = semaphore->value + 1;
        }

        //
        // If this upload completes a mip (or a few mips), create new image view and update available mip level count.
        //

        if (texture_upload_command.mip_level_count > 1 ||
            (texture_upload_command.x + texture_upload_command.width == base_mip_width &&
             texture_upload_command.y + texture_upload_command.height == base_mip_height &&
             texture_upload_command.z + texture_upload_command.depth == base_mip_depth &&
             texture_upload_command.base_array_layer + texture_upload_command.array_layer_count == texture_vulkan->get_array_layer_count()))
        {
            
            //
            // Create image view for uploaded mip levels.
            //

            VkImageAspectFlags aspect_mask;
            if (TextureFormatUtils::is_depth(format)) {
                // Sampled depth stencil textures provide access only to depth.
                aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
            } else {
                aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
            }

            VkImageViewType image_view_type;

            switch (texture_upload_command.texture->get_type()) {
            case TextureType::TEXTURE_2D:
                image_view_type = VK_IMAGE_VIEW_TYPE_2D;
                break;
            case TextureType::TEXTURE_CUBE:
                image_view_type = VK_IMAGE_VIEW_TYPE_CUBE;
                break;
            case TextureType::TEXTURE_3D:
                image_view_type = VK_IMAGE_VIEW_TYPE_3D;
                break;
            case TextureType::TEXTURE_2D_ARRAY:
                image_view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                break;
            case TextureType::TEXTURE_CUBE_ARRAY:
                image_view_type = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
                break;
            }

            VkImageSubresourceRange image_view_subresource_range{};
            image_view_subresource_range.aspectMask = aspect_mask;
            image_view_subresource_range.baseMipLevel = texture_upload_command.base_mip_level - texture_upload_command.mip_level_count + 1;
            image_view_subresource_range.levelCount = VK_REMAINING_MIP_LEVELS;
            image_view_subresource_range.baseArrayLayer = 0;
            image_view_subresource_range.layerCount = VK_REMAINING_ARRAY_LAYERS;

            VkImageViewCreateInfo image_view_create_info{};
            image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            image_view_create_info.flags = 0;
            image_view_create_info.image = texture_upload_command.texture->image;
            image_view_create_info.viewType = image_view_type;
            image_view_create_info.format = TextureFormatUtils::convert_format_vulkan(texture_upload_command.texture->get_format());
            image_view_create_info.subresourceRange = image_view_subresource_range;

            VkImageView image_view;
            VK_ERROR(
                vkCreateImageView(device, &image_view_create_info, &allocation_callbacks, &image_view),
                "Failed to create image view."
            );

            //
            // The previous image view must be destoyed once its no longer needed.
            //

            if (texture_upload_command.texture->image_view != VK_NULL_HANDLE) {
                std::lock_guard lock(m_image_view_destroy_command_mutex);
                m_image_view_destroy_commands.push(ImageViewDestroyCommand{ get_destroy_command_dependencies(), texture_upload_command.texture->image_view });
            }

            texture_upload_command.texture->image_view = image_view;

            //
            // Update the available mip level count.
            //

            KW_ASSERT(texture_vulkan->get_available_mip_level_count() <= texture_vulkan->get_mip_level_count() - (upload_texture_descriptor.base_mip_level + 1) + upload_texture_descriptor.mip_level_count);
            texture_vulkan->set_available_mip_level_count(texture_vulkan->get_mip_level_count() - (upload_texture_descriptor.base_mip_level + 1) + upload_texture_descriptor.mip_level_count);
        }
    } else {
        //
        // Figure out how to fit texture data into the staging buffer.
        //

        const uint8_t* data = static_cast<const uint8_t*>(upload_texture_descriptor.data);

        uint32_t texel_width = upload_texture_descriptor.width;
        uint32_t texel_height = upload_texture_descriptor.height;

        // Texel width/height for uncompressed texture types.
        uint32_t block_width = is_compressed ? (texel_width + 3) / 4 : texel_width;
        uint32_t block_height = is_compressed ? (texel_height + 3) / 4 : texel_height;

        uint32_t depth = std::max(upload_texture_descriptor.depth, 1U);

        uint32_t base_mip_level = upload_texture_descriptor.base_mip_level;
        uint32_t mip_level_count = std::max(upload_texture_descriptor.mip_level_count, 1U);

        uint32_t base_array_layer = upload_texture_descriptor.base_array_layer;
        uint32_t array_layer_count = std::max(upload_texture_descriptor.array_layer_count, 1U);

        uint32_t next_mip_level = base_mip_level;
        uint64_t next_mip_size = block_size * block_width * block_height * depth * array_layer_count;

        uint64_t total_mip_size = 0;

        while (base_mip_level - next_mip_level < mip_level_count && total_mip_size + next_mip_size <= m_staging_buffer_size / 2) {
            total_mip_size += next_mip_size;

            // This may overflow and it's ok. All the following variables will contain undefined values, but we won't use them.
            next_mip_level--;

            texel_width = std::max(texture_vulkan->get_width() >> next_mip_level, 1U);
            texel_height = std::max(texture_vulkan->get_height() >> next_mip_level, 1U);

            block_width = is_compressed ? (texel_width + 3) / 4 : texel_width;
            block_height = is_compressed ? (texel_height + 3) / 4 : texel_height;

            depth = std::max(texture_vulkan->get_depth() >> next_mip_level, 1U);

            next_mip_size = block_size * block_width * block_height * depth * array_layer_count;
        }

        if (base_mip_level - next_mip_level > 0) {
            //
            // We can upload some mip levels to staging buffers directly.
            //

            {
                UploadTextureDescriptor descriptor = upload_texture_descriptor;
                descriptor.data = data;
                descriptor.size = total_mip_size;
                descriptor.base_mip_level = base_mip_level;
                descriptor.mip_level_count = base_mip_level - next_mip_level;
                upload_texture(descriptor);
            }

            //
            // Other mip levels must be split and uploaded in the same manner.
            //

            if (base_mip_level - next_mip_level < mip_level_count) {
                UploadTextureDescriptor descriptor = upload_texture_descriptor;
                descriptor.data = data + total_mip_size;
                descriptor.size = upload_texture_descriptor.size - total_mip_size;
                descriptor.base_mip_level = next_mip_level;
                descriptor.mip_level_count = mip_level_count - (base_mip_level - next_mip_level);
                descriptor.width = texel_width;
                descriptor.height = texel_height;
                descriptor.depth = depth;
                upload_texture(descriptor);
            }
        } else {
            //
            // Find the number of array layers we can fit into the staging buffer.
            //

            uint32_t total_array_layer_count = array_layer_count;
            array_layer_count = (m_staging_buffer_size / 2) / (block_size * block_width * block_height * depth);
            KW_ASSERT(array_layer_count < total_array_layer_count);

            if (array_layer_count > 0) {
                //
                // We can upload next mip level by multiple uploads of array layers.
                //

                for (uint32_t i = 0; i < total_array_layer_count; i += array_layer_count) {
                    uint32_t count = std::min(array_layer_count, total_array_layer_count - i);

                    UploadTextureDescriptor descriptor = upload_texture_descriptor;
                    descriptor.data = data + block_size * block_width * block_height * depth * array_layer_count * i;
                    descriptor.size = block_size * block_width * block_height * depth * count;
                    descriptor.base_mip_level = base_mip_level;
                    descriptor.mip_level_count = 1;
                    descriptor.base_array_layer = base_array_layer + i;
                    descriptor.array_layer_count = count;
                    upload_texture(descriptor);
                }
            } else {
                //
                // Find the number of depth slices we can fit into the staging buffer.
                //

                uint32_t total_depth = depth;
                depth = (m_staging_buffer_size / 2) / (block_size * block_width * block_height);
                KW_ASSERT(depth < total_depth);

                if (depth > 0) {
                    //
                    // We can upload this mip level by multiple uploads of depth slices.
                    //

                    for (uint32_t i = 0; i < total_array_layer_count; i++) {
                        for (uint32_t j = 0; j < total_depth; j += depth) {
                            uint32_t count = std::min(depth, total_depth - j);

                            UploadTextureDescriptor descriptor = upload_texture_descriptor;
                            descriptor.data = data +
                                block_size * block_width * block_height * total_depth * i +
                                block_size * block_width * block_height * j;
                            descriptor.size = block_size * block_width * block_height * count;
                            descriptor.base_mip_level = base_mip_level;
                            descriptor.mip_level_count = 1;
                            descriptor.base_array_layer = base_array_layer + i;
                            descriptor.array_layer_count = 1;
                            descriptor.z = upload_texture_descriptor.z + j;
                            descriptor.depth = count;
                            upload_texture(descriptor);
                        }
                    }
                } else {
                    //
                    // Find the number of rows we can fit into the staging buffer.
                    //

                    uint32_t total_block_height = block_height;
                    block_height = (m_staging_buffer_size / 2) / (block_size * block_width);
                    KW_ASSERT(block_height < total_block_height);

                    if (block_height > 0) {
                        //
                        // We can upload this mip level by multiple uploads of rows.
                        //

                        for (uint32_t i = 0; i < total_array_layer_count; i++) {
                            for (uint32_t j = 0; j < total_depth; j++) {
                                for (uint32_t k = 0; k < total_block_height; k += block_height) {
                                    uint32_t block_count = std::min(block_height, total_block_height - k);

                                    // Invalid for uncompressed texture types.
                                    uint32_t texel_count = std::min(block_count * 4, texel_height - k * 4);

                                    UploadTextureDescriptor descriptor = upload_texture_descriptor;
                                    descriptor.data = data +
                                        block_size * block_width * total_block_height * total_depth * i +
                                        block_size * block_width * total_block_height * j +
                                        block_size * block_width * k;
                                    descriptor.size = block_size * block_width * block_count;
                                    descriptor.base_mip_level = base_mip_level;
                                    descriptor.mip_level_count = 1;
                                    descriptor.base_array_layer = base_array_layer + i;
                                    descriptor.array_layer_count = 1;
                                    descriptor.y = upload_texture_descriptor.y + (is_compressed ? k * 4 : k);
                                    descriptor.z = upload_texture_descriptor.z + j;
                                    descriptor.height = is_compressed ? texel_count : block_count;
                                    descriptor.depth = 1;
                                    upload_texture(descriptor);
                                }
                            }
                        }
                    } else {
                        //
                        // Find the number of columns we can fit into the staging buffer.
                        //

                        uint32_t total_block_width = block_width;
                        block_width = (m_staging_buffer_size / 2) / block_size;
                        KW_ASSERT(block_width > 0 && block_width < total_block_width);

                        //
                        // We can upload this mip level by multiple uploads of columns.
                        //

                        for (uint32_t i = 0; i < total_array_layer_count; i++) {
                            for (uint32_t j = 0; j < total_depth; j++) {
                                for (uint32_t k = 0; k < total_block_height; k++) {
                                    // Invalid for uncompressed texture types.
                                    uint32_t texel_y_count = std::min(4U, texel_height - k * 4);

                                    for (uint32_t l = 0; l < total_block_width; l += block_width) {
                                        uint32_t block_x_count = std::min(block_width, total_block_width - l);

                                        // Invalid for uncompressed texture types.
                                        uint32_t texel_x_count = std::min(block_x_count * 4, texel_width - k * 4);

                                        UploadTextureDescriptor descriptor = upload_texture_descriptor;
                                        descriptor.data = data +
                                            block_size * total_block_width * total_block_height * total_depth * i +
                                            block_size * total_block_width * total_block_height * j +
                                            block_size * total_block_width * k +
                                            block_size * l;
                                        descriptor.size = block_size * block_x_count;
                                        descriptor.base_mip_level = base_mip_level;
                                        descriptor.mip_level_count = 1;
                                        descriptor.base_array_layer = base_array_layer + i;
                                        descriptor.array_layer_count = 1;
                                        descriptor.x = upload_texture_descriptor.x + (is_compressed ? l * 4 : l);
                                        descriptor.y = upload_texture_descriptor.y + (is_compressed ? k * 4 : k);
                                        descriptor.z = upload_texture_descriptor.z + j;
                                        descriptor.width = is_compressed ? texel_x_count : block_x_count;
                                        descriptor.height = is_compressed ? texel_y_count : 1;
                                        descriptor.depth = 1;
                                        upload_texture(descriptor);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            //
            // Other mip levels must be split and uploaded in the same manner.
            //

            if (mip_level_count > 1) {
                UploadTextureDescriptor descriptor = upload_texture_descriptor;
                descriptor.data = data + next_mip_size;
                descriptor.size = upload_texture_descriptor.size - next_mip_size;
                descriptor.base_mip_level = base_mip_level - 1;
                descriptor.mip_level_count = mip_level_count - 1;
                descriptor.width = std::max(texture_vulkan->get_width() >> (base_mip_level - 1U), 1U);
                descriptor.height = std::max(texture_vulkan->get_height() >> (base_mip_level - 1U), 1U);
                descriptor.depth = std::max(texture_vulkan->get_depth() >> (base_mip_level - 1U), 1U);
                upload_texture(descriptor);
            }
        }
    }
}

void RenderVulkan::destroy_texture(Texture* texture) {
    if (texture != nullptr) {
        std::lock_guard lock(m_texture_destroy_command_mutex);
        m_texture_destroy_commands.push(TextureDestroyCommand{ get_destroy_command_dependencies(), static_cast<TextureVulkan*>(texture) });
    }
}

HostTexture* RenderVulkan::create_host_texture(const char* name, TextureFormat format, uint32_t width, uint32_t height) {
    KW_ASSERT(name != nullptr, "Invalid host texture name.");

    KW_ERROR(TextureFormatUtils::is_allowed_texture(format), "Invalid host texture \"%s\" format.", name);
    KW_ERROR(width > 0, "Invalid host texture \"%s\" width.", name);
    KW_ERROR(height > 0, "Invalid host texture \"%s\" height.", name);

    //
    // Create image to query its memory requirements.
    //

    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.flags = 0;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = TextureFormatUtils::convert_format_vulkan(format);
    image_create_info.extent.width = width;
    image_create_info.extent.height = height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = nullptr;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage image;
    VK_ERROR(
        vkCreateImage(device, &image_create_info, &allocation_callbacks, &image),
        "Failed to create a host image \"%s\".", name
    );
    VK_NAME(*this, image, "Host texture \"%s\"", name);

    //
    // Allocate device memory for the host texture buffer.
    //

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device, image, &memory_requirements);

    VkMemoryAllocateInfo memory_allocate_info{};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = m_host_texture_memory_index;

    VkDeviceMemory memory;
    VK_ERROR(
        vkAllocateMemory(device, &memory_allocate_info, &allocation_callbacks, &memory),
        "Failed to allocate memory for host image \"%s\".", name
    );
    VK_NAME(*this, memory, "Host texture device memory \"%s\"", name);

    //
    // Create host texture buffer.
    //
    
    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.flags = 0;
    buffer_create_info.size = memory_requirements.size;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = nullptr;

    VkBuffer buffer;
    VK_ERROR(
        vkCreateBuffer(device, &buffer_create_info, &allocation_callbacks, &buffer),
        "Failed to create host texture buffer \"%s\".", name
    );
    VK_NAME(*this, buffer, "Host texture buffer \"%s\"", name);

    //
    // Bind host texture buffer to memory.
    //

    VK_ERROR(
        vkBindBufferMemory(device, buffer, memory, 0),
        "Failed to bind host image \"%s\" to device memory.", name
    );

    //
    // Destroy the image.
    //

    vkDestroyImage(device, image, &allocation_callbacks);

    //
    // Create host texture handle and return it.
    //

    return new (persistent_memory_resource.allocate<HostTextureVulkan>()) HostTextureVulkan(
        format, width, height, buffer, memory, map_memory(memory), memory_requirements.size
    );
}

void RenderVulkan::read_host_texture(HostTexture* host_texture, void* buffer, size_t size) {
    HostTextureVulkan* host_texture_vulkan = static_cast<HostTextureVulkan*>(host_texture);
    KW_ASSERT(host_texture_vulkan != nullptr);

    size_t copy_size = std::max(size, host_texture_vulkan->size);
    std::memcpy(buffer, static_cast<uint8_t*>(host_texture_vulkan->memory_mapping), copy_size);
}

void RenderVulkan::destroy_host_texture(HostTexture* host_texture) {
    if (host_texture != nullptr) {
        std::lock_guard lock(m_host_texture_destroy_command_mutex);
        m_host_texture_destroy_commands.push(HostTextureDestroyCommand{ get_destroy_command_dependencies(), static_cast<HostTextureVulkan*>(host_texture) });
    }
}

VertexBuffer* RenderVulkan::acquire_transient_vertex_buffer(const void* data, size_t size) {
    KW_ASSERT(data != nullptr, "Invalid buffer data.");
    KW_ASSERT(size > 0, "Invalid buffer data size.");

    // Vertex data must be aligned by attribute type size. The largest supported attribute type at the moment is RGBA32F.
    uint64_t transient_buffer_offset = allocate_from_transient_memory(static_cast<uint64_t>(size), 16);

    // Memory is mapped persistently so it can be accessed from multiple threads simultaneously.
    std::memcpy(static_cast<uint8_t*>(m_transient_memory_mapping) + transient_buffer_offset, data, size);

    return new (transient_memory_resource.allocate<VertexBufferVulkan>()) VertexBufferVulkan(
        size, m_transient_buffer, transient_buffer_offset
    );
}

IndexBuffer* RenderVulkan::acquire_transient_index_buffer(const void* data, size_t size, IndexSize index_size) {
    KW_ASSERT(data != nullptr, "Invalid buffer data.");
    KW_ASSERT(size > 0, "Invalid buffer data size.");

    // Index data must be aligned by index type size.
    uint64_t transient_buffer_offset = allocate_from_transient_memory(static_cast<uint64_t>(size), index_size == IndexSize::UINT16 ? 2 : 4);

    // Memory is mapped persistently so it can be accessed from multiple threads simultaneously.
    std::memcpy(static_cast<uint8_t*>(m_transient_memory_mapping) + transient_buffer_offset, data, size);

    return new (transient_memory_resource.allocate<IndexBufferVulkan>()) IndexBufferVulkan(
        size, index_size, m_transient_buffer, transient_buffer_offset
    );
}

UniformBuffer* RenderVulkan::acquire_transient_uniform_buffer(const void* data, size_t size) {
    KW_ASSERT(data != nullptr, "Invalid buffer data.");
    KW_ASSERT(size > 0, "Invalid buffer data size.");

    // Uniform offset must be aligned by some limits constant.
    uint64_t transient_buffer_offset = allocate_from_transient_memory(static_cast<uint64_t>(size), static_cast<uint64_t>(physical_device_properties.limits.minUniformBufferOffsetAlignment));

    // Memory is mapped persistently so it can be accessed from multiple threads simultaneously.
    std::memcpy(static_cast<uint8_t*>(m_transient_memory_mapping) + transient_buffer_offset, data, size);

    return new (transient_memory_resource.allocate<UniformBufferVulkan>()) UniformBufferVulkan(
        size, transient_buffer_offset
    );
}

Task* RenderVulkan::create_task() {
    return new (transient_memory_resource.allocate<FlushTask>()) FlushTask(*this);
}

RenderApi RenderVulkan::get_api() const {
    return RenderApi::VULKAN;
}

void RenderVulkan::add_resource_dependency(SharedPtr<TimelineSemaphore> timeline_semaphore) {
    std::lock_guard lock(m_resource_dependency_mutex);

    m_resource_dependencies.push_back(timeline_semaphore);
}

RenderVulkan::DeviceAllocation RenderVulkan::allocate_device_buffer_memory(uint64_t size, uint64_t alignment) {
    KW_ASSERT(size > 0, "Invalid memory allocation size.");
    KW_ASSERT(alignment > 0 && is_pow2(alignment), "Invalid memory allocation alignment.");

    std::lock_guard lock(m_buffer_mutex);

    KW_ERROR(
        alignment <= m_buffer_block_size,
        "Invalid buffer alignment. Requested %llu, allowed %llu.", alignment, m_buffer_block_size
    );

    //
    // Try to sub-allocate buffer memory from existing allocations.
    //

    for (size_t device_data_index = 0; device_data_index < m_buffer_device_data.size(); device_data_index++) {
        uint64_t offset = m_buffer_device_data[device_data_index].allocator.allocate(size, alignment);
        if (offset != RenderBuddyAllocator::INVALID_ALLOCATION) {
            return { m_buffer_device_data[device_data_index].memory, static_cast<uint64_t>(device_data_index), offset };
        }
    }

    //
    // Create new allocation to sub-allocate from. First try device local, but when out of device memory, try host visible.
    //

    for (uint32_t buffer_memory_index : m_buffer_memory_indices) {
        for (uint64_t allocation_size = m_buffer_allocation_size; allocation_size >= m_buffer_block_size && allocation_size >= size; allocation_size /= 2) {
            VkMemoryAllocateInfo memory_allocate_info{};
            memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memory_allocate_info.allocationSize = allocation_size;
            memory_allocate_info.memoryTypeIndex = buffer_memory_index;

            VkDeviceMemory memory;
            VkResult result = vkAllocateMemory(device, &memory_allocate_info, &allocation_callbacks, &memory);
            if (result == VK_SUCCESS) {
                size_t device_data_index = m_buffer_device_data.size();

                void* memory_mapping = nullptr;
                if (buffer_memory_index == m_buffer_memory_indices[1]) {
                    // Persistently map host visible & host coherent memory.
                    VK_ERROR(
                        vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &memory_mapping),
                        "Failed to map host visible memory."
                    );
                }

                m_buffer_device_data.push_back(DeviceData{ memory, memory_mapping, RenderBuddyAllocator(persistent_memory_resource, log2(allocation_size), log2(m_buffer_block_size)), buffer_memory_index });

                uint64_t offset = m_buffer_device_data.back().allocator.allocate(size, alignment);
                KW_ASSERT(offset != RenderBuddyAllocator::INVALID_ALLOCATION);

                VK_NAME(*this, memory, "Buffer device memory %zu", device_data_index);

                return { memory, static_cast<uint64_t>(device_data_index), offset };
            }

            KW_ERROR(
                result == VK_ERROR_OUT_OF_HOST_MEMORY || result == VK_ERROR_OUT_OF_DEVICE_MEMORY,
                "Failed to allocate device buffer."
            );
        }
    }

    KW_ERROR(
        false,
        "Not enough video memory to allocate %llu bytes for a device buffer.", size
    );
}

void RenderVulkan::deallocate_device_buffer_memory(uint64_t data_index, uint64_t data_offset) {
    std::lock_guard lock(m_buffer_mutex);

    KW_ASSERT(data_index < m_buffer_device_data.size());
    m_buffer_device_data[data_index].allocator.deallocate(data_offset);
}

RenderVulkan::DeviceAllocation RenderVulkan::allocate_device_texture_memory(uint64_t size, uint64_t alignment) {
    KW_ASSERT(size > 0, "Invalid memory allocation size.");
    KW_ASSERT(alignment > 0 && is_pow2(alignment), "Invalid memory allocation alignment.");

    std::lock_guard lock(m_texture_mutex);

    KW_ERROR(
        alignment <= m_texture_block_size && m_texture_block_size % alignment == 0,
        "Invalid texture alignment. Requested %llu, allowed %llu.", alignment, m_texture_block_size
    );

    //
    // Try to sub-allocate buffer memory from existing allocations.
    //

    for (size_t device_data_index = 0; device_data_index < m_texture_device_data.size(); device_data_index++) {
        uint64_t offset = m_texture_device_data[device_data_index].allocator.allocate(size, alignment);
        if (offset != RenderBuddyAllocator::INVALID_ALLOCATION) {
            return { m_texture_device_data[device_data_index].memory, static_cast<uint64_t>(device_data_index), offset };
        }
    }

    //
    // Create new allocation to sub-allocate from. First try device local, but when out of device memory, try host visible.
    //
    
    for (uint64_t allocation_size = m_texture_allocation_size; allocation_size >= m_texture_block_size && allocation_size >= size; allocation_size /= 2) {
        VkMemoryAllocateInfo memory_allocate_info{};
        memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_allocate_info.allocationSize = allocation_size;
        memory_allocate_info.memoryTypeIndex = m_texture_memory_index;

        VkDeviceMemory memory;
        VkResult result = vkAllocateMemory(device, &memory_allocate_info, &allocation_callbacks, &memory);
        if (result == VK_SUCCESS) {
            size_t device_data_index = m_texture_device_data.size();

            // We won't ever map texture memory, because we can't simply memcpy to textures.
            m_texture_device_data.push_back(DeviceData{ memory, nullptr, RenderBuddyAllocator(persistent_memory_resource, log2(allocation_size), log2(m_texture_block_size)), m_texture_memory_index });

            uint64_t offset = m_texture_device_data.back().allocator.allocate(size, alignment);
            KW_ASSERT(offset != RenderBuddyAllocator::INVALID_ALLOCATION);

            VK_NAME(*this, memory, "Texture device memory %zu", device_data_index);

            return { memory, static_cast<uint64_t>(device_data_index), offset };
        }

        KW_ERROR(
            result == VK_ERROR_OUT_OF_HOST_MEMORY || result == VK_ERROR_OUT_OF_DEVICE_MEMORY,
            "Failed to allocate texture device buffer."
        );
    }

    KW_ERROR(
        false,
        "Not enough video memory to allocate %llu bytes for texture device buffer.", size
    );
}

void RenderVulkan::deallocate_device_texture_memory(uint64_t data_index, uint64_t data_offset) {
    std::lock_guard lock(m_texture_mutex);

    KW_ASSERT(data_index < m_texture_device_data.size());
    m_texture_device_data[data_index].allocator.deallocate(data_offset);
}

VkBuffer RenderVulkan::get_transient_buffer() const {
    return m_transient_buffer;
}

VkInstance RenderVulkan::create_instance(const RenderDescriptor& descriptor) {
    VkApplicationInfo application_info{};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName = "KURWA";
    application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info.pEngineName = "KURWA";
    application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info.apiVersion = VK_API_VERSION_1_0;

    Vector<const char*> instance_layers(transient_memory_resource);
    if (descriptor.is_validation_enabled) {
        instance_layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    uint32_t instance_extensions_count;
    SDL_ERROR(
        SDL_Vulkan_GetInstanceExtensions(nullptr, &instance_extensions_count, nullptr),
        "Failed to get instance extension count."
    );

    Vector<const char*> instance_extensions(transient_memory_resource);
    instance_extensions.reserve(instance_extensions_count + 2);
    instance_extensions.resize(instance_extensions_count);

    SDL_ERROR(
        SDL_Vulkan_GetInstanceExtensions(nullptr, &instance_extensions_count, instance_extensions.data()),
        "Failed to get instance extensions."
    );

    instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    if (descriptor.is_validation_enabled || descriptor.is_debug_names_enabled) {
        instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkInstanceCreateInfo instance_create_info{};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &application_info;
    instance_create_info.enabledLayerCount = static_cast<uint32_t>(instance_layers.size());
    instance_create_info.ppEnabledLayerNames = instance_layers.data();
    instance_create_info.enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size());
    instance_create_info.ppEnabledExtensionNames = instance_extensions.data();

    VkInstance instance;
    VK_ERROR(
        vkCreateInstance(&instance_create_info, &allocation_callbacks, &instance),
        "Failed to create an instance."
    );

    return instance;
}

VkPhysicalDevice RenderVulkan::create_physical_device() {
    uint32_t physical_device_count;
    VK_ERROR(
        vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr),
        "Failed to query physical device count."
    );

    Vector<VkPhysicalDevice> physical_devices(physical_device_count, transient_memory_resource);
    VK_ERROR(
        vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data()),
        "Failed to query physical devices."
    );

    VkPhysicalDevice best_physical_device = VK_NULL_HANDLE;
    bool best_physical_device_is_discrete = false;
    VkDeviceSize best_physical_device_size = 0;

    for (uint32_t physical_device_index = 0; physical_device_index < physical_device_count; physical_device_index++) {
        VkPhysicalDevice physical_device = physical_devices[physical_device_index];

        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(physical_device, &device_properties);

        bool is_discrete = device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

        VkPhysicalDeviceMemoryProperties memory_properties;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

        VkDeviceSize device_size = 0;
        for (uint32_t memory_heap_index = 0; memory_heap_index < memory_properties.memoryHeapCount; memory_heap_index++) {
            device_size += memory_properties.memoryHeaps[memory_heap_index].size;
        }

        if (best_physical_device == VK_NULL_HANDLE ||
            (!best_physical_device_is_discrete && is_discrete) ||
            (best_physical_device_is_discrete == is_discrete && device_size > best_physical_device_size)) {
            uint32_t queue_family_count;
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

            Vector<VkQueueFamilyProperties> queue_families(queue_family_count, transient_memory_resource);
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

            for (uint32_t queue_family_index = 0; queue_family_index < queue_family_count; queue_family_index++) {
                if ((queue_families[queue_family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT) {
                    best_physical_device = physical_device;
                    best_physical_device_is_discrete = is_discrete;
                    best_physical_device_size = device_size;
                }
            }
        }
    }

    KW_ERROR(best_physical_device != VK_NULL_HANDLE, "Failed to find any suitable physical device.");

    return best_physical_device;
}

VkPhysicalDeviceMemoryProperties RenderVulkan::get_physical_device_memory_properties() {
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
    return memory_properties;
}

VkPhysicalDeviceProperties RenderVulkan::get_physical_device_properties() {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical_device, &properties);
    return properties;
}

uint32_t RenderVulkan::get_graphics_queue_family_index() {
    uint32_t queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

    Vector<VkQueueFamilyProperties> queue_families(queue_family_count, transient_memory_resource);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

    for (uint32_t queue_family_index = 0; queue_family_index < queue_family_count; queue_family_index++) {
        if ((queue_families[queue_family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT) {
            return queue_family_index;
        }
    }

    // Never happens, this queue is required by physical device.
    return 0;
}

uint32_t RenderVulkan::get_compute_queue_family_index() {
    uint32_t queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

    Vector<VkQueueFamilyProperties> queue_families(queue_family_count, transient_memory_resource);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

    for (uint32_t queue_family_index = 0; queue_family_index < queue_family_count; queue_family_index++) {
        if ((queue_families[queue_family_index].queueFlags & VK_QUEUE_COMPUTE_BIT) == VK_QUEUE_COMPUTE_BIT &&
            (queue_families[queue_family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != VK_QUEUE_GRAPHICS_BIT) {
            return queue_family_index;
        }
    }

    // Failed to find async compute queue family, fallback to graphics queue family.
    return graphics_queue_family_index;
}

uint32_t RenderVulkan::get_transfer_queue_family_index() {
    uint32_t queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

    Vector<VkQueueFamilyProperties> queue_families(queue_family_count, transient_memory_resource);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

    for (uint32_t queue_family_index = 0; queue_family_index < queue_family_count; queue_family_index++) {
        if ((queue_families[queue_family_index].queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT &&
            (queue_families[queue_family_index].queueFlags & VK_QUEUE_COMPUTE_BIT) != VK_QUEUE_COMPUTE_BIT) {
            return queue_family_index;
        }
    }

    // Failed to find transfer queue family, fallback to graphics queue family.
    return graphics_queue_family_index;
}

VkDevice RenderVulkan::create_device(const RenderDescriptor& descriptor) {
    Vector<VkDeviceQueueCreateInfo> device_queue_create_infos(transient_memory_resource);
    device_queue_create_infos.reserve(3);

    float queue_priority = 1.f;

    VkDeviceQueueCreateInfo graphics_queue_create_info{};
    graphics_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphics_queue_create_info.queueFamilyIndex = graphics_queue_family_index;
    graphics_queue_create_info.queueCount = 1;
    graphics_queue_create_info.pQueuePriorities = &queue_priority;
    device_queue_create_infos.push_back(graphics_queue_create_info);

    if (compute_queue_family_index != graphics_queue_family_index) {
        VkDeviceQueueCreateInfo compute_queue_create_info{};
        compute_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        compute_queue_create_info.queueFamilyIndex = compute_queue_family_index;
        compute_queue_create_info.queueCount = 1;
        compute_queue_create_info.pQueuePriorities = &queue_priority;
        device_queue_create_infos.push_back(compute_queue_create_info);
    }

    if (transfer_queue_family_index != graphics_queue_family_index) {
        VkDeviceQueueCreateInfo transfer_queue_create_info{};
        transfer_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        transfer_queue_create_info.queueFamilyIndex = transfer_queue_family_index;
        transfer_queue_create_info.queueCount = 1;
        transfer_queue_create_info.pQueuePriorities = &queue_priority;
        device_queue_create_infos.push_back(transfer_queue_create_info);
    }

    uint32_t device_layer_count = descriptor.is_validation_enabled ? 1 : 0;
    const char* device_layer = "VK_LAYER_KHRONOS_validation";

    const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME
    };

    VkPhysicalDeviceFeatures physical_device_features{};
    physical_device_features.depthBiasClamp = VK_TRUE;
    physical_device_features.fillModeNonSolid = VK_TRUE;
    physical_device_features.independentBlend = VK_TRUE;
    physical_device_features.samplerAnisotropy = VK_TRUE;
    physical_device_features.textureCompressionBC = VK_TRUE;

    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VkStructureType::VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(device_queue_create_infos.size());
    device_create_info.pQueueCreateInfos = device_queue_create_infos.data();
    device_create_info.enabledLayerCount = device_layer_count;
    device_create_info.ppEnabledLayerNames = &device_layer;
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(std::size(device_extensions));
    device_create_info.ppEnabledExtensionNames = device_extensions;
    device_create_info.pEnabledFeatures = &physical_device_features;

    VkDevice device;
    VK_ERROR(
        vkCreateDevice(physical_device, &device_create_info, &allocation_callbacks, &device),
        "Failed to create a device."
    );

    return device;
}

VkQueue RenderVulkan::create_graphics_queue() {
    VkQueue queue;
    vkGetDeviceQueue(device, graphics_queue_family_index, 0, &queue);
    return queue;
}

VkQueue RenderVulkan::create_compute_queue() {
    if (compute_queue_family_index != graphics_queue_family_index) {
        VkQueue queue;
        vkGetDeviceQueue(device, compute_queue_family_index, 0, &queue);
        return queue;
    }
    return graphics_queue;
}

VkQueue RenderVulkan::create_transfer_queue() {
    if (transfer_queue_family_index != graphics_queue_family_index) {
        VkQueue queue;
        vkGetDeviceQueue(device, transfer_queue_family_index, 0, &queue);
        return queue;
    }
    return graphics_queue;
}

SharedPtr<Spinlock> RenderVulkan::create_graphics_queue_spinlock() {
    return allocate_shared<Spinlock>(persistent_memory_resource);
}

SharedPtr<Spinlock> RenderVulkan::create_compute_queue_spinlock() {
    return compute_queue != graphics_queue ? allocate_shared<Spinlock>(persistent_memory_resource) : graphics_queue_spinlock;
}

SharedPtr<Spinlock> RenderVulkan::create_transfer_queue_spinlock() {
    return transfer_queue != graphics_queue ? allocate_shared<Spinlock>(persistent_memory_resource) : graphics_queue_spinlock;
}

PFN_vkWaitSemaphoresKHR RenderVulkan::create_wait_semaphores() {
    PFN_vkWaitSemaphoresKHR wait_semaphores = reinterpret_cast<PFN_vkWaitSemaphoresKHR>(vkGetDeviceProcAddr(device, "vkWaitSemaphoresKHR"));
    KW_ERROR(
        wait_semaphores != nullptr,
        "Failed to get vkWaitSemaphoresKHR function."
    );
    return wait_semaphores;
}

PFN_vkGetSemaphoreCounterValueKHR RenderVulkan::create_get_semaphore_counter_value() {
    PFN_vkGetSemaphoreCounterValueKHR get_semaphore_counter_value_ = reinterpret_cast<PFN_vkGetSemaphoreCounterValueKHR>(vkGetDeviceProcAddr(device, "vkGetSemaphoreCounterValueKHR"));
    KW_ERROR(
        get_semaphore_counter_value_ != nullptr,
        "Failed to get vkGetSemaphoreCounterValueKHR function."
    );
    return get_semaphore_counter_value_;
}

VkDebugUtilsMessengerEXT RenderVulkan::create_debug_messsenger(const RenderDescriptor& descriptor) {
    if (descriptor.is_validation_enabled) {
        auto vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
        KW_ERROR(
            vkCreateDebugUtilsMessengerEXT != nullptr,
            "Failed to get vkCreateDebugUtilsMessengerEXT function."
        );

        VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info{};
        debug_utils_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_utils_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_utils_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_utils_messenger_create_info.pfnUserCallback = debug_callback;
        debug_utils_messenger_create_info.pUserData = nullptr;

        VkDebugUtilsMessengerEXT debug_messenger;
        VK_ERROR(
            vkCreateDebugUtilsMessengerEXT(instance, &debug_utils_messenger_create_info, &allocation_callbacks, &debug_messenger),
            "Failed to create debug messenger."
        );

        return debug_messenger;
    }

    return VK_NULL_HANDLE;
}

PFN_vkSetDebugUtilsObjectNameEXT RenderVulkan::create_set_object_name(const RenderDescriptor& descriptor) {
    if (descriptor.is_debug_names_enabled) {
        PFN_vkSetDebugUtilsObjectNameEXT set_object_name = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT"));
        KW_ERROR(
            set_object_name != nullptr,
            "Failed to get vkSetDebugUtilsObjectNameEXT function."
        );

        return set_object_name;
    }

    return nullptr;
}

VkCommandPool RenderVulkan::create_graphics_command_pool() {
    VkCommandPoolCreateInfo command_pool_create_info{};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = graphics_queue_family_index;

    VkCommandPool command_pool;

    VK_ERROR(
        vkCreateCommandPool(device, &command_pool_create_info, &allocation_callbacks, &command_pool),
        "Failed to create command pool."
    );

    VK_NAME(*this, command_pool, "Graphics command pool");

    return command_pool;
}

VkCommandPool RenderVulkan::create_compute_command_pool() {
    if (compute_queue_family_index != graphics_queue_family_index) {
        VkCommandPoolCreateInfo command_pool_create_info{};
        command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        command_pool_create_info.queueFamilyIndex = compute_queue_family_index;

        VkCommandPool command_pool;

        VK_ERROR(
            vkCreateCommandPool(device, &command_pool_create_info, &allocation_callbacks, &command_pool),
            "Failed to create command pool."
        );

        VK_NAME(*this, command_pool, "Compute command pool");

        return command_pool;
    }

    return m_graphics_command_pool;
}

VkCommandPool RenderVulkan::create_transfer_command_pool() {
    if (transfer_queue_family_index != graphics_queue_family_index) {
        VkCommandPoolCreateInfo command_pool_create_info{};
        command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        command_pool_create_info.queueFamilyIndex = transfer_queue_family_index;

        VkCommandPool command_pool;

        VK_ERROR(
            vkCreateCommandPool(device, &command_pool_create_info, &allocation_callbacks, &command_pool),
            "Failed to create command pool."
        );

        VK_NAME(*this, command_pool, "Transfer command pool");

        return command_pool;
    }

    return m_graphics_command_pool;
}

VkBuffer RenderVulkan::create_staging_buffer(const RenderDescriptor& render_descriptor) {
    VkBufferCreateInfo staging_buffer_create_info{};
    staging_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    staging_buffer_create_info.flags = 0;
    staging_buffer_create_info.size = render_descriptor.staging_buffer_size;
    staging_buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    staging_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    staging_buffer_create_info.queueFamilyIndexCount = 0;
    staging_buffer_create_info.pQueueFamilyIndices = 0;

    VkBuffer buffer;

    VK_ERROR(
        vkCreateBuffer(device, &staging_buffer_create_info, &allocation_callbacks, &buffer),
        "Failed to create staging buffer."
    );

    VK_NAME(*this, buffer, "Staging buffer");

    return buffer;
}

VkDeviceMemory RenderVulkan::allocate_staging_memory() {
    //
    // Query buffer memory requirements and find suitable memory type.
    //

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, m_staging_buffer, &memory_requirements);

    uint32_t memory_type_index = find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    KW_ERROR(
        memory_type_index != UINT32_MAX,
        "Failed to find memory type for staging buffer memory allocation."
    );

    //
    // Allocate memory for the buffer.
    //

    VkMemoryAllocateInfo memory_allocate_info{};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = memory_type_index;

    VkDeviceMemory memory;

    VK_ERROR(
        vkAllocateMemory(device, &memory_allocate_info, &allocation_callbacks, &memory),
        "Failed to allocate %llu bytes for staging buffer.", memory_requirements.size
    );

    VK_NAME(*this, memory, "Staging memory");

    VK_ERROR(
        vkBindBufferMemory(device, m_staging_buffer, memory, 0),
        "Failed to bind staging buffer to memory."
    );

    return memory;
}

uint64_t RenderVulkan::allocate_from_staging_memory(uint64_t size, uint64_t alignment) {
    KW_ASSERT(size > 0, "Invalid allocation size.");

    return allocate_from_staging_memory(size, size, alignment).second;
}

Pair<uint64_t, uint64_t> RenderVulkan::allocate_from_staging_memory(uint64_t min, uint64_t max, uint64_t alignment) {
    KW_ASSERT(max >= min, "Max must be greater than min.");
    KW_ASSERT(alignment > 0 && is_pow2(alignment), "Alignment must be power of 2.");
    KW_ASSERT(
        min <= m_staging_buffer_size / 2,
        "Staging allocation is too big. Requested %llu, allowed %llu.", min, m_staging_buffer_size / 2
    );

    uint64_t staging_data_end = m_staging_data_end.load(std::memory_order_relaxed);
    while (true) {
        uint64_t staging_data_begin = m_staging_data_begin.load(std::memory_order_acquire);
        uint64_t alignment_offset = (alignment - staging_data_end) & (alignment - 1);
        if (staging_data_end >= staging_data_begin) {
            if (min + alignment_offset <= m_staging_buffer_size - staging_data_end) {
                uint64_t size = std::min(m_staging_buffer_size - staging_data_end, max + alignment_offset);
                if (m_staging_data_end.compare_exchange_weak(staging_data_end, staging_data_end + size, std::memory_order_release, std::memory_order_relaxed)) {
                    // Allocate in the end without fragmentation.
                    return { size - alignment_offset, staging_data_end + alignment_offset };
                }
            } else if (min < staging_data_begin) {
                uint64_t size = std::min(staging_data_begin, max);
                if (m_staging_data_end.compare_exchange_weak(staging_data_end, size, std::memory_order_release, std::memory_order_relaxed)) {
                    // Allocate in the beginning with fragmentation.
                    return { size, 0 };
                }
            } else if (min > 0) {
                // Synchronous wait.
                wait_for_staging_memory();
            } else {
                // Wait-free return.
                return { 0, 0 };
            }
        } else {
            if (min + alignment_offset < staging_data_begin - staging_data_end) {
                uint64_t size = std::min(staging_data_begin - staging_data_end, max + alignment_offset);
                if (m_staging_data_end.compare_exchange_weak(staging_data_end, staging_data_end + size, std::memory_order_release, std::memory_order_relaxed)) {
                    // Allocate in the middle without fragmentation.
                    return { size - alignment_offset, staging_data_end + alignment_offset };
                }
            } else if (min > 0) {
                // Synchronous wait.
                wait_for_staging_memory();
            } else {
                // Wait-free return.
                return { 0, 0 };
            }
        }
    }
}

void RenderVulkan::wait_for_staging_memory() {
    if (m_submit_data_mutex.try_lock()) {
        if (m_submit_data.empty()) {
            // We're out of staging buffer and we don't have any submitted upload commands, which means that
            // we allocated all memory but not flushed. Flush and then wait for commands to finish execution.

            flush();

            // Flush must produce some flush data.
            KW_ASSERT(!m_submit_data.empty());
        }

        // Wait until submitted transfer commands finish execution.
        SubmitData& submit_data = m_submit_data.front();

        VkSemaphoreWaitInfo semaphore_wait_info{};
        semaphore_wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        semaphore_wait_info.flags = 0;
        semaphore_wait_info.semaphoreCount = 1;
        semaphore_wait_info.pSemaphores = &semaphore->semaphore;
        semaphore_wait_info.pValues = &submit_data.semaphore_value;

        // When this semaphore is signaled, staging data from `staging_data_begin` to
        // `submit_data.staging_data_end` becomes available for allocation.
        VK_ERROR(
            wait_semaphores(device, &semaphore_wait_info, UINT64_MAX),
            "Failed to wait for a transfer semaphore."
        );

        if (submit_data.graphics_command_buffer != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(device, m_graphics_command_pool, 1, &submit_data.graphics_command_buffer);
        }

        if (submit_data.compute_command_buffer != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(device, m_compute_command_pool, 1, &submit_data.compute_command_buffer);
        }

        if (submit_data.transfer_command_buffer != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(device, m_transfer_command_pool, 1, &submit_data.transfer_command_buffer);
        }

        m_staging_data_begin.store(submit_data.staging_data_end, std::memory_order_release);

        m_submit_data.pop();

        m_submit_data_mutex.unlock();
    } else {
        // Seems like some other thread is flushing memory for us. It can take a while. Wait for it.
        std::lock_guard lock(m_submit_data_mutex);
    }
}

uint32_t RenderVulkan::compute_buffer_memory_index(VkMemoryPropertyFlags properties) {
    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.flags = 0;
    buffer_create_info.size = 4;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = 0;

    VkBuffer buffer;
    VK_ERROR(
        vkCreateBuffer(device, &buffer_create_info, &allocation_callbacks, &buffer),
        "Failed to create a dummy buffer to query memory type mask."
    );
    VK_NAME(*this, buffer, "Dummy buffer");

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

    vkDestroyBuffer(device, buffer, &allocation_callbacks);

    uint32_t buffer_memory_index = find_memory_type(memory_requirements.memoryTypeBits, properties);
    KW_ERROR(buffer_memory_index != UINT32_MAX, "Failed to find memory type for buffer allocation.");

    return buffer_memory_index;
}

uint32_t RenderVulkan::compute_texture_memory_index(VkMemoryPropertyFlags properties) {
    uint32_t memory_type_mask = UINT32_MAX;

    for (size_t i = 0; i < TEXTURE_FORMAT_COUNT; i++) {
        TextureFormat format = static_cast<TextureFormat>(i);

        if (!TextureFormatUtils::is_allowed_texture(format)) {
            continue;
        }

        VkImageCreateInfo image_create_info{};
        image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.flags = 0;
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.format = TextureFormatUtils::convert_format_vulkan(format);
        image_create_info.extent.width = 4;
        image_create_info.extent.height = 4;
        image_create_info.extent.depth = 1;
        image_create_info.mipLevels = 1;
        image_create_info.arrayLayers = 1;
        image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_create_info.queueFamilyIndexCount = 0;
        image_create_info.pQueueFamilyIndices = nullptr;
        image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkImage image;
        VK_ERROR(
            vkCreateImage(device, &image_create_info, &allocation_callbacks, &image),
            "Failed to create dummy image to query memory type mask."
        );
        VK_NAME(*this, image, "Dummy image %zu", i);

        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(device, image, &memory_requirements);

        vkDestroyImage(device, image, &allocation_callbacks);

        memory_type_mask &= memory_requirements.memoryTypeBits;
    }

    uint32_t texture_memory_index = find_memory_type(memory_type_mask, properties);
    KW_ERROR(texture_memory_index != UINT32_MAX, "Failed to find texture memory type.");

    return texture_memory_index;
}

void* RenderVulkan::map_memory(VkDeviceMemory memory) {
    void* result;

    VK_ERROR(
        vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &result),
        "Failed to map memory."
    );

    return result;
}

VkBuffer RenderVulkan::create_transient_buffer(const RenderDescriptor& render_descriptor) {
    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.flags = 0;
    buffer_create_info.size = render_descriptor.transient_buffer_size;
    buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = 0;

    VkBuffer transient_buffer;

    VK_ERROR(
        vkCreateBuffer(device, &buffer_create_info, &allocation_callbacks, &transient_buffer),
        "Failed to create transient buffer."
    );

    VK_NAME(*this, transient_buffer, "Transient buffer");

    return transient_buffer;
}

VkDeviceMemory RenderVulkan::allocate_transient_memory() {
    //
    // If this device supports local & host visible memory type, then use it for transient buffer.
    // If it doesn't or there's not enough space in it, stick to old good host-visible memory type.
    //

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, m_transient_buffer, &memory_requirements);

    VkMemoryPropertyFlags property_masks[] = {
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };

    for (VkMemoryPropertyFlags property_mask : property_masks) {
        uint32_t memory_type_index = find_memory_type(memory_requirements.memoryTypeBits, property_mask);
        if (memory_type_index != UINT32_MAX) {
            VkMemoryAllocateInfo memory_allocate_info{};
            memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memory_allocate_info.allocationSize = memory_requirements.size;
            memory_allocate_info.memoryTypeIndex = memory_type_index;

            VkDeviceMemory device_memory;
            if (vkAllocateMemory(device, &memory_allocate_info, &allocation_callbacks, &device_memory) == VK_SUCCESS) {
                VK_NAME(*this, device_memory, "Transient memory");

                VK_ERROR(
                    vkBindBufferMemory(device, m_transient_buffer, device_memory, 0),
                    "Failed to bind transient buffer to memory."
                );

                return device_memory;
            }
        }
    }

    KW_ERROR(false, "Failed to allocate %llu bytes for transient buffer.", memory_requirements.size);
}

uint64_t RenderVulkan::allocate_from_transient_memory(uint64_t size, uint64_t alignment) {
    KW_ASSERT(
        size <= m_transient_buffer_size,
        "Transient allocation is too big. Requested %llu, allowed %llu.", size, m_transient_buffer_size
    );

    uint64_t transient_data_end = m_transient_data_end.load(std::memory_order_relaxed);

    while (true) {
        uint64_t aligned_transient_data_end = align_up(transient_data_end, alignment);
        uint64_t new_transient_data_end = aligned_transient_data_end + size;

        if (new_transient_data_end <= m_transient_buffer_size) {
            if (m_transient_data_end.compare_exchange_weak(transient_data_end, new_transient_data_end, std::memory_order_release, std::memory_order_relaxed)) {
                return aligned_transient_data_end;
            }
        } else {
            if (m_transient_data_end.compare_exchange_weak(transient_data_end, size, std::memory_order_release, std::memory_order_relaxed)) {
                return 0;
            }
        }
    }
}

uint32_t RenderVulkan::find_memory_type(uint32_t memory_type_mask, VkMemoryPropertyFlags property_mask) {
    for (uint32_t memory_type_index = 0; memory_type_index < physical_device_memory_properties.memoryTypeCount; memory_type_index++) {
        uint32_t memory_type_bit = 1 << memory_type_index;
        if ((memory_type_mask & memory_type_bit) == memory_type_bit) {
            if ((physical_device_memory_properties.memoryTypes[memory_type_index].propertyFlags & property_mask) == property_mask) {
                return memory_type_index;
            }
        }
    }

    return UINT32_MAX;
}

Vector<RenderVulkan::DestroyCommandDependency> RenderVulkan::get_destroy_command_dependencies() {
    std::lock_guard lock(m_resource_dependency_mutex);

    Vector<DestroyCommandDependency> result(persistent_memory_resource);
    result.reserve(m_resource_dependencies.size() + 1);

    // In case of "create, destroy, flush" rather than "create, flush, destroy" we'd want to postpone destroy
    // until the first flush. Otherwise resource will stay in a creation queue after destruction and cause invalid
    // memory access. That could be avoided by checking in destroy whether the resource is in a creation queue
    // and removing it from there. However the "create, destroy, flush" case is kind of irrational, so postponing
    // destroy seems like a good compromise.
    result.push_back(DestroyCommandDependency{ semaphore, semaphore->value + 1 });

    for (size_t i = 0; i < m_resource_dependencies.size(); ) {
        if (SharedPtr<TimelineSemaphore> shared_timeline_semaphore = m_resource_dependencies[i].lock()) {
            result.push_back(DestroyCommandDependency{ shared_timeline_semaphore, shared_timeline_semaphore->value });
            i++;
        } else {
            std::swap(m_resource_dependencies[i], m_resource_dependencies.back());
            m_resource_dependencies.pop_back();
        }
    }

    return result;
}

bool RenderVulkan::wait_for_dependencies(Vector<DestroyCommandDependency>& dependencies) {
    Vector<SharedPtr<TimelineSemaphore>> timeline_semaphores(transient_memory_resource);
    timeline_semaphores.reserve(dependencies.size());

    Vector<VkSemaphore> semaphores(transient_memory_resource);
    semaphores.reserve(dependencies.size());

    Vector<uint64_t> values(transient_memory_resource);
    values.reserve(dependencies.size());

    for (DestroyCommandDependency& destroy_command_dependency : dependencies) {
        if (SharedPtr<TimelineSemaphore> timeline_semaphore = destroy_command_dependency.semaphore.lock()) {
            values.push_back(destroy_command_dependency.value);
            semaphores.push_back(timeline_semaphore->semaphore);
            timeline_semaphores.push_back(std::move(timeline_semaphore));
        }
    }

    VkSemaphoreWaitInfo semaphore_wait_info{};
    semaphore_wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    semaphore_wait_info.flags = 0;
    semaphore_wait_info.semaphoreCount = static_cast<uint32_t>(timeline_semaphores.size());
    semaphore_wait_info.pSemaphores = semaphores.data();
    semaphore_wait_info.pValues = values.data();

    return wait_semaphores(device, &semaphore_wait_info, 0) == VK_SUCCESS;
}

void RenderVulkan::flush() {
    process_completed_submits();
    destroy_queued_buffers();
    destroy_queued_textures();
    destroy_queued_host_textures();
    destroy_queued_image_views();
    submit_upload_commands();
}

void RenderVulkan::process_completed_submits() {
    std::lock_guard lock(m_submit_data_mutex);

    while (!m_submit_data.empty()) {
        SubmitData& submit_data = m_submit_data.front();

        VkSemaphoreWaitInfo semaphore_wait_info{};
        semaphore_wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        semaphore_wait_info.flags = 0;
        semaphore_wait_info.semaphoreCount = 1;
        semaphore_wait_info.pSemaphores = &semaphore->semaphore;
        semaphore_wait_info.pValues = &submit_data.semaphore_value;

        if (wait_semaphores(device, &semaphore_wait_info, 0) == VK_SUCCESS) {
            if (submit_data.graphics_command_buffer != VK_NULL_HANDLE) {
                vkFreeCommandBuffers(device, m_graphics_command_pool, 1, &submit_data.graphics_command_buffer);
            }

            if (submit_data.compute_command_buffer != VK_NULL_HANDLE) {
                vkFreeCommandBuffers(device, m_compute_command_pool, 1, &submit_data.compute_command_buffer);
            }

            KW_ASSERT(submit_data.transfer_command_buffer != VK_NULL_HANDLE);
            vkFreeCommandBuffers(device, m_transfer_command_pool, 1, &submit_data.transfer_command_buffer);

            m_staging_data_begin.store(submit_data.staging_data_end, std::memory_order_release);

            m_submit_data.pop();
        } else {
            // The following submits in a queue have greater semaphore values.
            break;
        }
    }
}

void RenderVulkan::destroy_queued_buffers() {
    std::lock_guard lock(m_buffer_destroy_command_mutex);

    while (!m_buffer_destroy_commands.empty()) {
        BufferDestroyCommand& buffer_destroy_command = m_buffer_destroy_commands.front();

        if (wait_for_dependencies(buffer_destroy_command.dependencies)) {
            // Transfer semaphore is also a destroy dependency. If it just signaled, we need to destroy command buffer
            // before destroying the buffer, because the former may have dependency on the latter.
            process_completed_submits();

            vkDestroyBuffer(device, buffer_destroy_command.buffer->buffer, &allocation_callbacks);

            deallocate_device_buffer_memory(buffer_destroy_command.buffer->device_data_index, buffer_destroy_command.buffer->device_data_offset);
            
            // `VertexBufferVulkan` and `IndexBufferVulkan` have the same size and the same offset to `BufferVulkan`.
            persistent_memory_resource.deallocate(static_cast<VertexBufferVulkan*>(buffer_destroy_command.buffer));

            m_buffer_destroy_commands.pop();
        } else {
            // The following resources in a queue have greater or equal semaphore values.
            break;
        }
    }
}

void RenderVulkan::destroy_queued_textures() {
    std::lock_guard lock(m_texture_destroy_command_mutex);

    while (!m_texture_destroy_commands.empty()) {
        TextureDestroyCommand& texture_destroy_command = m_texture_destroy_commands.front();

        if (wait_for_dependencies(texture_destroy_command.dependencies)) {
            // Transfer semaphore is also a destroy dependency. If it just signaled, we need to destroy command buffer
            // before destroying the texture, because the former may have dependency on the latter.
            process_completed_submits();

            vkDestroyImageView(device, texture_destroy_command.texture->image_view, &allocation_callbacks);
            vkDestroyImage(device, texture_destroy_command.texture->image, &allocation_callbacks);

            deallocate_device_texture_memory(texture_destroy_command.texture->device_data_index, texture_destroy_command.texture->device_data_offset);

            persistent_memory_resource.deallocate(texture_destroy_command.texture);

            m_texture_destroy_commands.pop();
        } else {
            // The following resources in a queue have greater or equal semaphore values.
            break;
        }
    }
}

void RenderVulkan::destroy_queued_host_textures() {
    std::lock_guard lock(m_host_texture_destroy_command_mutex);

    while (!m_host_texture_destroy_commands.empty()) {
        HostTextureDestroyCommand& host_texture_destroy_command = m_host_texture_destroy_commands.front();
        if (wait_for_dependencies(host_texture_destroy_command.dependencies)) {
            vkUnmapMemory(device, host_texture_destroy_command.host_texture->device_memory);
            vkFreeMemory(device, host_texture_destroy_command.host_texture->device_memory, &allocation_callbacks);
            vkDestroyBuffer(device, host_texture_destroy_command.host_texture->buffer, &allocation_callbacks);
            persistent_memory_resource.deallocate(host_texture_destroy_command.host_texture);
            m_texture_destroy_commands.pop();
        } else {
            // The following resources in a queue have greater or equal semaphore values.
            break;
        }
    }
}

void RenderVulkan::destroy_queued_image_views() {
    std::lock_guard lock(m_image_view_destroy_command_mutex);

    while (!m_image_view_destroy_commands.empty()) {
        ImageViewDestroyCommand& image_view_destroy_command = m_image_view_destroy_commands.front();
        if (wait_for_dependencies(image_view_destroy_command.dependencies)) {
            vkDestroyImageView(device, image_view_destroy_command.image_view, &allocation_callbacks);
            m_image_view_destroy_commands.pop();
        } else {
            // The following resources in a queue have greater or equal semaphore values.
            break;
        }
    }
}

void RenderVulkan::submit_upload_commands() {
    std::scoped_lock buffer_texture_lock(m_buffer_upload_command_mutex, m_texture_upload_command_mutex);

    if (!m_buffer_upload_commands.empty() || !m_texture_upload_commands.empty()) {
        std::lock_guard submit_data_lock(m_submit_data_mutex);

        //
        // Upload resources to device.
        //

        auto [transfer_command_buffer, staging_data_end] = upload_resources();

        //
        // If transfer and graphics queue are from different queue families, queue ownership transfer must be performed.
        //

        VkCommandBuffer graphics_command_buffer = VK_NULL_HANDLE;
        if (transfer_queue_family_index != graphics_queue_family_index) {
            graphics_command_buffer = acquire_graphics_ownership();
        }

        //
        // These were used for both upload and graphics acquire.
        //

        m_buffer_upload_commands.clear();
        m_texture_upload_commands.clear();

        //
        // Destroy command buffer(s) when copy commands completed on device.
        //

        m_submit_data.push(SubmitData{ transfer_command_buffer, VK_NULL_HANDLE, graphics_command_buffer, semaphore->value, staging_data_end });
    }
}

Pair<VkCommandBuffer, uint64_t> RenderVulkan::upload_resources() {
    KW_ASSERT(!m_buffer_upload_commands.empty() || !m_texture_upload_commands.empty());

    //
    // Create new transfer command buffer.
    //

    VkCommandBufferAllocateInfo transfer_command_buffer_allocate_info{};
    transfer_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    transfer_command_buffer_allocate_info.commandPool = m_transfer_command_pool;
    transfer_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    transfer_command_buffer_allocate_info.commandBufferCount = 1;

    VkCommandBuffer transfer_command_buffer;
    VK_ERROR(
        vkAllocateCommandBuffers(device, &transfer_command_buffer_allocate_info, &transfer_command_buffer),
        "Failed to allocate a transfer command buffer."
    );
    VK_NAME(*this, transfer_command_buffer, "Transfer command buffer");

    //
    // Begin transfer command buffer.
    //

    VkCommandBufferBeginInfo transfer_command_buffer_begin_info{};
    transfer_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    transfer_command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_ERROR(
        vkBeginCommandBuffer(transfer_command_buffer, &transfer_command_buffer_begin_info),
        "Failed to begin a transfer command buffer."
    );

    //
    // Upload resources.
    //

    uint64_t staging_data_end = staging_buffer_max(
        upload_buffers(transfer_command_buffer),
        upload_textures(transfer_command_buffer)
    );

    //
    // If transfer and graphics queue are from different queue families, the copy commands are submitted to a transfer
    // queue and ownership acquire commands are submitted to a graphics queue. Resource users wait for the latter,
    // so public semaphore must be signaled from graphics queue.
    //

    VkSemaphore transfer_semaphore;
    uint64_t transfer_signal_value;

    if (transfer_queue_family_index != graphics_queue_family_index) {
        transfer_semaphore = m_intermediate_semaphore->semaphore;
        transfer_signal_value = ++m_intermediate_semaphore->value;
    } else {
        transfer_semaphore = semaphore->semaphore;
        transfer_signal_value = ++semaphore->value;
    }

    //
    // End transfer command buffer and submit to a transfer queue.
    //

    VK_ERROR(
        vkEndCommandBuffer(transfer_command_buffer),
        "Failed to end a transfer command buffer."
    );

    VkTimelineSemaphoreSubmitInfo transfer_timeline_semaphore_submit_info{};
    transfer_timeline_semaphore_submit_info.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    transfer_timeline_semaphore_submit_info.waitSemaphoreValueCount = 0;
    transfer_timeline_semaphore_submit_info.pWaitSemaphoreValues = nullptr;
    transfer_timeline_semaphore_submit_info.signalSemaphoreValueCount = 1;
    transfer_timeline_semaphore_submit_info.pSignalSemaphoreValues = &transfer_signal_value;

    VkSubmitInfo transfer_submit_info{};
    transfer_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    transfer_submit_info.pNext = &transfer_timeline_semaphore_submit_info;
    transfer_submit_info.waitSemaphoreCount = 0;
    transfer_submit_info.pWaitSemaphores = nullptr;
    transfer_submit_info.pWaitDstStageMask = nullptr;
    transfer_submit_info.commandBufferCount = 1;
    transfer_submit_info.pCommandBuffers = &transfer_command_buffer;
    transfer_submit_info.signalSemaphoreCount = 1;
    transfer_submit_info.pSignalSemaphores = &transfer_semaphore;

    {
        std::lock_guard lock(*transfer_queue_spinlock);

        VK_ERROR(
            vkQueueSubmit(transfer_queue, 1, &transfer_submit_info, VK_NULL_HANDLE),
            "Failed to submit copy commands to a transfer queue."
        );
    }

    return { transfer_command_buffer, staging_data_end };
}

uint64_t RenderVulkan::upload_buffers(VkCommandBuffer transfer_command_buffer) {
    // Staging data end equal to `m_staging_data_begin` is the latest. The one after it is the earliest.
    uint64_t staging_data_end = m_staging_data_begin + 1;

    for (BufferUploadCommand& buffer_upload_command : m_buffer_upload_commands) {
        VkBufferCopy buffer_copy{};
        buffer_copy.srcOffset = buffer_upload_command.staging_buffer_offset;
        buffer_copy.dstOffset = buffer_upload_command.device_buffer_offset;
        buffer_copy.size = buffer_upload_command.staging_buffer_size;

        vkCmdCopyBuffer(transfer_command_buffer, m_staging_buffer, buffer_upload_command.buffer->buffer, 1, &buffer_copy);

        // If transfer queue and graphics queue are from different queue families, ownership transfer is reuqired.
        if (transfer_queue_family_index != graphics_queue_family_index) {
            VkBufferMemoryBarrier release_barrier{};
            release_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            release_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            release_barrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
            release_barrier.srcQueueFamilyIndex = transfer_queue_family_index;
            release_barrier.dstQueueFamilyIndex = graphics_queue_family_index;
            release_barrier.buffer = buffer_upload_command.buffer->buffer;
            release_barrier.offset = buffer_upload_command.device_buffer_offset;
            release_barrier.size = buffer_upload_command.staging_buffer_size;

            vkCmdPipelineBarrier(
                transfer_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
                0, nullptr, 1, &release_barrier, 0, nullptr
            );
        }

        staging_data_end = staging_buffer_max(staging_data_end, buffer_upload_command.staging_buffer_offset + buffer_upload_command.staging_buffer_size);
    }

    // Keep `m_buffer_copy_commands` items for ownership transfer.

    return staging_data_end;
}

uint64_t RenderVulkan::upload_textures(VkCommandBuffer transfer_command_buffer) {
    // Staging data end equal to `m_staging_data_begin` is the latest. The one after it is the earliest.
    uint64_t staging_data_end = m_staging_data_begin + 1;

    for (TextureUploadCommand& texture_upload_command : m_texture_upload_commands) {
        TextureFormat format = texture_upload_command.texture->get_format();
        bool is_compressed = TextureFormatUtils::is_compressed(format);
        uint64_t block_size = TextureFormatUtils::get_texel_size(format);

        VkImageAspectFlags aspect_mask;
        if (TextureFormatUtils::is_depth(format)) {
            // Sampled depth stencil textures provide access only to depth.
            aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
        } else {
            aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        uint32_t mip_level_count = texture_upload_command.texture->get_mip_level_count();
        uint32_t array_layer_count = texture_upload_command.texture->get_array_layer_count();

        uint32_t base_mip_width = std::max(texture_upload_command.texture->get_width() >> texture_upload_command.base_mip_level, 1U);
        uint32_t base_mip_height = std::max(texture_upload_command.texture->get_height() >> texture_upload_command.base_mip_level, 1U);
        uint32_t base_mip_depth = std::max(texture_upload_command.texture->get_depth() >> texture_upload_command.base_mip_level, 1U);

        //
        // The first upload transitions image from undefined image layout to transfer destination layout.
        //

        if (texture_upload_command.base_mip_level + 1 == mip_level_count) {
            KW_ASSERT(
                texture_upload_command.mip_level_count > 1 ||
                (texture_upload_command.base_mip_level + 1 == mip_level_count &&
                 texture_upload_command.mip_level_count == 1 &&
                 texture_upload_command.base_array_layer == 0 &&
                 texture_upload_command.array_layer_count == array_layer_count &&
                 texture_upload_command.x == 0 &&
                 texture_upload_command.y == 0 &&
                 texture_upload_command.z == 0 &&
                 texture_upload_command.width == base_mip_width &&
                 texture_upload_command.height == base_mip_height &&
                 texture_upload_command.depth == base_mip_depth),
                "The first upload must make at least the smallest mip level available."
            );

            VkImageSubresourceRange image_subresource_range{};
            image_subresource_range.aspectMask = aspect_mask;
            image_subresource_range.baseMipLevel = 0;
            image_subresource_range.levelCount = VK_REMAINING_MIP_LEVELS;
            image_subresource_range.baseArrayLayer = 0;
            image_subresource_range.layerCount = VK_REMAINING_ARRAY_LAYERS;

            VkImageMemoryBarrier acquire_barrier{};
            acquire_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            acquire_barrier.srcAccessMask = 0;
            acquire_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            acquire_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            acquire_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            acquire_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            acquire_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            acquire_barrier.image = texture_upload_command.texture->image;
            acquire_barrier.subresourceRange = image_subresource_range;

            vkCmdPipelineBarrier(
                transfer_command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr, 0, nullptr, 1, &acquire_barrier
            );
        }

        uint64_t staging_buffer_offset = texture_upload_command.staging_buffer_offset;

        uint32_t current_mip_width = texture_upload_command.width;
        uint32_t current_mip_height = texture_upload_command.height;
        uint32_t current_mip_depth = texture_upload_command.depth;

        for (uint32_t i = 0; i < texture_upload_command.mip_level_count; i++) {
            VkImageSubresourceLayers image_subresource_layers{};
            image_subresource_layers.aspectMask = aspect_mask;
            image_subresource_layers.mipLevel = texture_upload_command.base_mip_level - i;
            image_subresource_layers.baseArrayLayer = texture_upload_command.base_array_layer;
            image_subresource_layers.layerCount = texture_upload_command.array_layer_count;

            VkOffset3D offset{};
            offset.x = texture_upload_command.x;
            offset.y = texture_upload_command.y;
            offset.z = texture_upload_command.z;

            VkExtent3D extent{};
            extent.width = current_mip_width;
            extent.height = current_mip_height;
            extent.depth = current_mip_depth;

            VkBufferImageCopy buffer_image_copy{};
            buffer_image_copy.bufferOffset = staging_buffer_offset;
            buffer_image_copy.bufferRowLength = 0;
            buffer_image_copy.bufferImageHeight = 0;
            buffer_image_copy.imageSubresource = image_subresource_layers;
            buffer_image_copy.imageOffset = offset;
            buffer_image_copy.imageExtent = extent;

            vkCmdCopyBufferToImage(
                transfer_command_buffer, m_staging_buffer, texture_upload_command.texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &buffer_image_copy
            );

            staging_buffer_offset += is_compressed ?
                block_size * ((current_mip_width + 3) / 4) * ((current_mip_height + 3) / 4) * current_mip_depth * texture_upload_command.array_layer_count :
                block_size * current_mip_width * current_mip_height * current_mip_depth * texture_upload_command.array_layer_count;
            
            // This may overflow and it's ok. The following extent will contain undefined values, but we won't use it.
            uint32_t next_mip_level = texture_upload_command.base_mip_level - i - 1;

            // Update extent for larger mip levels.
            current_mip_width = std::max(texture_upload_command.texture->get_width() >> next_mip_level, 1U);
            current_mip_height = std::max(texture_upload_command.texture->get_height() >> next_mip_level, 1U);
            current_mip_depth = std::max(texture_upload_command.texture->get_depth() >> next_mip_level, 1U);
        }

        //
        // If transfer queue and graphics queue are from different queue families, ownership transfer is reuqired.
        //

        if (transfer_queue_family_index != graphics_queue_family_index &&
            (texture_upload_command.mip_level_count > 1 ||
             (texture_upload_command.x + texture_upload_command.width == base_mip_width &&
              texture_upload_command.y + texture_upload_command.height == base_mip_height &&
              texture_upload_command.z + texture_upload_command.depth == base_mip_depth &&
              texture_upload_command.base_array_layer + texture_upload_command.array_layer_count == array_layer_count)))
        {
            VkImageSubresourceRange image_subresource_range{};
            image_subresource_range.aspectMask = aspect_mask;
            image_subresource_range.baseMipLevel = texture_upload_command.base_mip_level - texture_upload_command.mip_level_count + 1;
            image_subresource_range.levelCount = texture_upload_command.mip_level_count;
            image_subresource_range.baseArrayLayer = texture_upload_command.base_array_layer;
            image_subresource_range.layerCount = texture_upload_command.array_layer_count;

            VkImageMemoryBarrier release_barrier{};
            release_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            release_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            release_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            release_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            release_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            release_barrier.srcQueueFamilyIndex = transfer_queue_family_index;
            release_barrier.dstQueueFamilyIndex = graphics_queue_family_index;
            release_barrier.image = texture_upload_command.texture->image;
            release_barrier.subresourceRange = image_subresource_range;

            vkCmdPipelineBarrier(
                transfer_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
                0, nullptr, 0, nullptr, 1, &release_barrier
            );
        }

        staging_data_end = staging_buffer_max(staging_data_end, texture_upload_command.staging_buffer_offset + texture_upload_command.staging_buffer_size);
    }

    // Keep `m_texture_update_commands` items for ownership transfer.

    return staging_data_end;
}

uint64_t RenderVulkan::staging_buffer_max(uint64_t lhs, uint64_t rhs) const {
    uint32_t begin = m_staging_data_begin.load(std::memory_order_acquire);
    if (lhs <= begin) {
        if (rhs <= begin) {
            return std::max(lhs, rhs);
        } else {
            return lhs;
        }
    } else {
        if (rhs <= begin) {
            return rhs;
        } else {
            return std::max(lhs, rhs);
        }
    }
}

VkCommandBuffer RenderVulkan::acquire_graphics_ownership() {
    KW_ASSERT(transfer_queue_family_index != graphics_queue_family_index);
    KW_ASSERT(!m_buffer_upload_commands.empty() || !m_texture_upload_commands.empty());

    //
    // Create new graphics command buffer.
    //

    VkCommandBufferAllocateInfo graphics_command_buffer_allocate_info{};
    graphics_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    graphics_command_buffer_allocate_info.commandPool = m_graphics_command_pool;
    graphics_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    graphics_command_buffer_allocate_info.commandBufferCount = 1;

    VkCommandBuffer graphics_command_buffer;
    VK_ERROR(
        vkAllocateCommandBuffers(device, &graphics_command_buffer_allocate_info, &graphics_command_buffer),
        "Failed to allocate a graphics command buffer."
    );
    VK_NAME(*this, graphics_command_buffer, "Graphics command buffer");

    //
    // Begin graphics command buffer.
    //

    VkCommandBufferBeginInfo graphics_command_buffer_begin_info{};
    graphics_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    graphics_command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_ERROR(
        vkBeginCommandBuffer(graphics_command_buffer, &graphics_command_buffer_begin_info),
        "Failed to begin a graphics command buffer."
    );

    //
    // Acquire all resources on graphics queue.
    //
    
    acquire_graphics_ownership_buffers(graphics_command_buffer);
    acquire_graphics_ownership_textures(graphics_command_buffer);

    //
    // End graphics command buffer and submit it to a graphics queue.
    //

    VK_ERROR(
        vkEndCommandBuffer(graphics_command_buffer),
        "Failed to end a graphics command buffer."
    );

    uint64_t transfer_signal_value = m_intermediate_semaphore->value;
    uint64_t graphics_signal_value = ++semaphore->value;

    VkTimelineSemaphoreSubmitInfo graphics_timeline_semaphore_submit_info{};
    graphics_timeline_semaphore_submit_info.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    graphics_timeline_semaphore_submit_info.waitSemaphoreValueCount = 1;
    graphics_timeline_semaphore_submit_info.pWaitSemaphoreValues = &transfer_signal_value;
    graphics_timeline_semaphore_submit_info.signalSemaphoreValueCount = 1;
    graphics_timeline_semaphore_submit_info.pSignalSemaphoreValues = &graphics_signal_value;

    VkPipelineStageFlags graphics_wait_stage_masks = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    VkSubmitInfo graphics_submit_info{};
    graphics_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    graphics_submit_info.pNext = &graphics_timeline_semaphore_submit_info;
    graphics_submit_info.waitSemaphoreCount = 1;
    graphics_submit_info.pWaitSemaphores = &m_intermediate_semaphore->semaphore;
    graphics_submit_info.pWaitDstStageMask = &graphics_wait_stage_masks;
    graphics_submit_info.commandBufferCount = 1;
    graphics_submit_info.pCommandBuffers = &graphics_command_buffer;
    graphics_submit_info.signalSemaphoreCount = 1;
    graphics_submit_info.pSignalSemaphores = &semaphore->semaphore;

    {
        std::lock_guard lock(*graphics_queue_spinlock);

        VK_ERROR(
            vkQueueSubmit(graphics_queue, 1, &graphics_submit_info, VK_NULL_HANDLE),
            "Failed to submit acquire ownership commands to a graphics queue."
        );
    }

    return graphics_command_buffer;
}

void RenderVulkan::acquire_graphics_ownership_buffers(VkCommandBuffer graphics_command_buffer) {
    for (BufferUploadCommand& buffer_upload_command : m_buffer_upload_commands) {
        VkBufferMemoryBarrier acquire_barrier{};
        acquire_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        acquire_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        acquire_barrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        acquire_barrier.srcQueueFamilyIndex = transfer_queue_family_index;
        acquire_barrier.dstQueueFamilyIndex = graphics_queue_family_index;
        acquire_barrier.buffer = buffer_upload_command.buffer->buffer;
        acquire_barrier.offset = buffer_upload_command.device_buffer_offset;
        acquire_barrier.size = buffer_upload_command.staging_buffer_size;

        vkCmdPipelineBarrier(
            graphics_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
            0, nullptr, 1, &acquire_barrier, 0, nullptr
        );
    }
}

void RenderVulkan::acquire_graphics_ownership_textures(VkCommandBuffer graphics_command_buffer) {
    for (TextureUploadCommand& texture_upload_command : m_texture_upload_commands) {
        uint32_t array_layer_count = texture_upload_command.texture->get_array_layer_count();

        uint32_t base_mip_width = std::max(texture_upload_command.texture->get_width() >> texture_upload_command.base_mip_level, 1U);
        uint32_t base_mip_height = std::max(texture_upload_command.texture->get_height() >> texture_upload_command.base_mip_level, 1U);
        uint32_t base_mip_depth = std::max(texture_upload_command.texture->get_depth() >> texture_upload_command.base_mip_level, 1U);
        
        // Mip level is acquired on graphics queue only when finished.
        if (texture_upload_command.mip_level_count > 1 ||
            (texture_upload_command.x + texture_upload_command.width == base_mip_width &&
             texture_upload_command.y + texture_upload_command.height == base_mip_height &&
             texture_upload_command.z + texture_upload_command.depth == base_mip_depth &&
             texture_upload_command.base_array_layer + texture_upload_command.array_layer_count == array_layer_count))
        {
            TextureFormat format = texture_upload_command.texture->get_format();

            VkImageAspectFlags aspect_mask;
            if (TextureFormatUtils::is_depth_stencil(format)) {
                // Sampled depth stencil textures provide access only to depth.
                aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
            } else {
                aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
            }

            VkImageSubresourceRange image_subresource_range{};
            image_subresource_range.aspectMask = aspect_mask;
            image_subresource_range.baseMipLevel = texture_upload_command.base_mip_level - texture_upload_command.mip_level_count + 1;
            image_subresource_range.levelCount = texture_upload_command.mip_level_count;
            image_subresource_range.baseArrayLayer = texture_upload_command.base_array_layer;
            image_subresource_range.layerCount = texture_upload_command.array_layer_count;

            VkImageMemoryBarrier acquire_barrier{};
            acquire_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            acquire_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            acquire_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            acquire_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            acquire_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            acquire_barrier.srcQueueFamilyIndex = transfer_queue_family_index;
            acquire_barrier.dstQueueFamilyIndex = graphics_queue_family_index;
            acquire_barrier.image = texture_upload_command.texture->image;
            acquire_barrier.subresourceRange = image_subresource_range;

            vkCmdPipelineBarrier(
                graphics_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
                0, nullptr, 0, nullptr, 1, &acquire_barrier
            );
        }
    }
}

} // namespace kw
