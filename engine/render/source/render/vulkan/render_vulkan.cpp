#include "render/vulkan/render_vulkan.h"
#include "render/vulkan/timeline_semaphore.h"
#include "render/vulkan/vulkan_utils.h"

#include <core/debug/assert.h>
#include <core/debug/log.h>
#include <core/math.h>

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
    static char buffer[1024];
    sprintf_s(buffer, sizeof(buffer), "%s: %s", callback_data->pMessageIdName, callback_data->pMessage);
    Log::print(buffer);
    return VK_FALSE;
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
    , semaphore(std::make_shared<TimelineSemaphore>(this))
    , m_wait_semaphores(create_wait_semaphores())
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
    , m_transient_buffer(create_transient_buffer(descriptor))
    , m_transient_memory(allocate_transient_memory())
    , m_transient_memory_mapping(map_memory(m_transient_memory))
    , m_transient_buffer_size(descriptor.transient_buffer_size)
    , m_transient_data_end(0)
    , m_texture_device_data(persistent_memory_resource)
    , m_texture_allocation_size(descriptor.texture_allocation_size)
    , m_texture_block_size(descriptor.texture_block_size)
    , m_texture_memory_indices{
        compute_texture_memory_index(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
        compute_texture_memory_index(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    }
    , m_resource_dependencies(persistent_memory_resource)
    , m_buffer_destroy_commands(MemoryResourceAllocator<BufferDestroyCommand>(persistent_memory_resource))
    , m_texture_destroy_commands(MemoryResourceAllocator<TextureDestroyCommand>(persistent_memory_resource))
    , m_buffer_copy_commands(persistent_memory_resource)
    , m_texture_copy_commands(persistent_memory_resource)
    , m_submit_data(MemoryResourceAllocator<SubmitData>(persistent_memory_resource))
    , m_intermediate_semaphore(std::make_unique<TimelineSemaphore>(this))
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

    m_buffer_copy_commands.reserve(32);
    m_texture_copy_commands.reserve(32);
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
        persistent_memory_resource.deallocate(buffer_destroy_command.buffer);
        m_buffer_destroy_commands.pop();
    }

    for (TextureCopyCommand& texture_copy_command : m_texture_copy_commands) {
        persistent_memory_resource.deallocate(texture_copy_command.offsets);
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

VertexBuffer RenderVulkan::create_vertex_buffer(const BufferDescriptor& buffer_descriptor) {
    return reinterpret_cast<VertexBuffer>(create_buffer_vulkan(buffer_descriptor, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT));
}

void RenderVulkan::destroy_vertex_buffer(VertexBuffer vertex_buffer) {
    if (vertex_buffer != nullptr) {
        destroy_buffer_vulkan(const_cast<BufferVulkan*>(reinterpret_cast<const BufferVulkan*>(vertex_buffer)));
    }
}

IndexBuffer RenderVulkan::create_index_buffer(const BufferDescriptor& buffer_descriptor) {
    return reinterpret_cast<IndexBuffer>(create_buffer_vulkan(buffer_descriptor, VK_BUFFER_USAGE_INDEX_BUFFER_BIT));
}

void RenderVulkan::destroy_index_buffer(IndexBuffer index_buffer) {
    if (index_buffer != nullptr) {
        destroy_buffer_vulkan(const_cast<BufferVulkan*>(reinterpret_cast<const BufferVulkan*>(index_buffer)));
    }
}

VertexBuffer RenderVulkan::acquire_transient_vertex_buffer(const void* data, size_t size) {
    return reinterpret_cast<VertexBuffer>(acquire_transient_buffer_vulkan(data, size, 16, BufferFlagsVulkan::NONE));
}

IndexBuffer RenderVulkan::acquire_transient_index_buffer(const void* data, size_t size, IndexSize index_size) {
    if (index_size == IndexSize::UINT16) {
        return reinterpret_cast<IndexBuffer>(acquire_transient_buffer_vulkan(data, size, 2, BufferFlagsVulkan::INDEX16));
    } else {
        return reinterpret_cast<IndexBuffer>(acquire_transient_buffer_vulkan(data, size, 4, BufferFlagsVulkan::INDEX32));
    }
}

UniformBuffer RenderVulkan::acquire_transient_uniform_buffer(const void* data, size_t size) {
    size_t alignment = static_cast<size_t>(physical_device_properties.limits.minUniformBufferOffsetAlignment);
    return reinterpret_cast<UniformBuffer>(acquire_transient_buffer_vulkan(data, size, alignment, BufferFlagsVulkan::NONE));
}

Texture RenderVulkan::create_texture(const TextureDescriptor& texture_descriptor) {
    return reinterpret_cast<Texture>(create_texture_vulkan(texture_descriptor));
}

void RenderVulkan::destroy_texture(Texture texture) {
    if (texture != nullptr) {
        destroy_texture_vulkan(const_cast<TextureVulkan*>(reinterpret_cast<const TextureVulkan*>(texture)));
    }
}

void RenderVulkan::flush() {
    process_completed_submits();
    destroy_queued_buffers();
    destroy_queued_textures();
    submit_copy_commands();
}

RenderApi RenderVulkan::get_api() const {
    return RenderApi::VULKAN;
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

void RenderVulkan::add_resource_dependency(std::shared_ptr<TimelineSemaphore> timeline_semaphore) {
    std::lock_guard<std::mutex> lock(m_resource_dependency_mutex);

    for (size_t i = 0; i < m_resource_dependencies.size(); ) {
        if (!m_resource_dependencies[i].lock()) {
            std::swap(m_resource_dependencies[i], m_resource_dependencies.back());
            m_resource_dependencies.pop_back();
        } else {
            i++;
        }
    }

    m_resource_dependencies.push_back(timeline_semaphore);
}

RenderVulkan::DeviceAllocation RenderVulkan::allocate_device_buffer_memory(uint64_t size, uint64_t alignment) {
    std::lock_guard<std::mutex> lock(m_buffer_mutex);

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
    std::lock_guard<std::mutex> lock(m_buffer_mutex);

    KW_ASSERT(data_index < m_buffer_device_data.size());
    m_buffer_device_data[data_index].allocator.deallocate(data_offset);
}

RenderVulkan::DeviceAllocation RenderVulkan::allocate_device_texture_memory(uint64_t size, uint64_t alignment) {
    KW_ASSERT(size > 0);
    KW_ASSERT(alignment > 0 && is_pow2(alignment));

    std::lock_guard<std::mutex> lock(m_texture_mutex);

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

    for (uint32_t texture_memory_index : m_texture_memory_indices) {
        for (uint64_t allocation_size = m_texture_allocation_size; allocation_size >= m_texture_block_size && allocation_size >= size; allocation_size /= 2) {
            VkMemoryAllocateInfo memory_allocate_info{};
            memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memory_allocate_info.allocationSize = allocation_size;
            memory_allocate_info.memoryTypeIndex = texture_memory_index;

            VkDeviceMemory memory;
            VkResult result = vkAllocateMemory(device, &memory_allocate_info, &allocation_callbacks, &memory);
            if (result == VK_SUCCESS) {
                size_t device_data_index = m_texture_device_data.size();

                // We won't ever map texture memory, because we can't simply memcpy to textures.
                m_texture_device_data.push_back(DeviceData{ memory, nullptr, RenderBuddyAllocator(persistent_memory_resource, log2(allocation_size), log2(m_texture_block_size)), texture_memory_index });

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
    }

    KW_ERROR(
        false,
        "Not enough video memory to allocate %llu bytes for texture device buffer.", size
    );
}

void RenderVulkan::deallocate_device_texture_memory(uint64_t data_index, uint64_t data_offset) {
    std::lock_guard<std::mutex> lock(m_texture_mutex);

    KW_ASSERT(data_index < m_texture_device_data.size());
    m_texture_device_data[data_index].allocator.deallocate(data_offset);
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

    // Failed to find async compute queue family, fallback to graphics family.
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

    // Failed to find transfer queue family, fallback to graphics family.
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
    physical_device_features.independentBlend = VK_TRUE;
    physical_device_features.depthBiasClamp = VK_TRUE;
    physical_device_features.fillModeNonSolid = VK_TRUE;
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

std::shared_ptr<Spinlock> RenderVulkan::create_graphics_queue_spinlock() {
    return std::make_shared<Spinlock>();
}

std::shared_ptr<Spinlock> RenderVulkan::create_compute_queue_spinlock() {
    return compute_queue != graphics_queue ? std::make_shared<Spinlock>() : graphics_queue_spinlock;
}

std::shared_ptr<Spinlock> RenderVulkan::create_transfer_queue_spinlock() {
    return transfer_queue != graphics_queue ? std::make_shared<Spinlock>() : graphics_queue_spinlock;
}

PFN_vkWaitSemaphoresKHR RenderVulkan::create_wait_semaphores() {
    PFN_vkWaitSemaphoresKHR wait_semaphores = reinterpret_cast<PFN_vkWaitSemaphoresKHR>(vkGetDeviceProcAddr(device, "vkWaitSemaphoresKHR"));
    KW_ERROR(
        wait_semaphores != nullptr,
        "Failed to get vkWaitSemaphoresKHR function."
    );
    return wait_semaphores;
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
    KW_ASSERT(size > 0);
    KW_ASSERT(alignment > 0 && is_pow2(alignment));

    KW_ERROR(
        alignment + size - 1 <= m_staging_buffer_size / 2,
        "Staging allocation is too big. Requested %llu, allowed %llu.", alignment + size - 1, m_staging_buffer_size / 2
    );

    uint64_t staging_data_end = m_staging_data_end.load(std::memory_order_relaxed);
    while (true) {
        // Acquire because we don't want to mess with the flush data if another thread recently did it for us.
        uint64_t staging_data_begin = m_staging_data_begin.load(std::memory_order_acquire);
        
        uint64_t aligned_staging_data_end = align_up(staging_data_end, alignment);
        uint64_t new_staging_data_end = aligned_staging_data_end + size;

        if ((staging_data_end >= staging_data_begin && new_staging_data_end <= m_staging_buffer_size) ||
            (staging_data_end < staging_data_begin && new_staging_data_end < staging_data_begin)) {
            if (m_staging_data_end.compare_exchange_weak(staging_data_end, new_staging_data_end, std::memory_order_release, std::memory_order_relaxed)) {
                return aligned_staging_data_end;
            }
        } else {
            if (staging_data_end >= staging_data_begin && size < staging_data_begin) {
                if (m_staging_data_end.compare_exchange_weak(staging_data_end, size, std::memory_order_release, std::memory_order_relaxed)) {
                    return 0;
                }
            } else {
                if (m_submit_data_mutex.try_lock()) {
                    if (m_submit_data.empty()) {
                        // We're out of staging buffer and we don't have any submitted transfer commands, which means that
                        // we allocated all memory but not flushed. Flush and then wait for commands to finish execution.

                        flush();

                        // Flush must produce some flush data.
                        KW_ASSERT(!m_submit_data.empty());
                    }

                    // We're out of staging buffer, wait until submitted transfer commands finish execution.
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
                        m_wait_semaphores(device, &semaphore_wait_info, UINT64_MAX),
                        "Failed to wait for a transfer semaphore."
                    );

                    if (submit_data.graphics_command_buffer != VK_NULL_HANDLE) {
                        vkFreeCommandBuffers(device, m_graphics_command_pool, 1, &submit_data.graphics_command_buffer);
                    }

                    if (submit_data.compute_command_buffer != VK_NULL_HANDLE) {
                        vkFreeCommandBuffers(device, m_compute_command_pool, 1, &submit_data.compute_command_buffer);
                    }

                    KW_ASSERT(submit_data.transfer_command_buffer != VK_NULL_HANDLE);
                    vkFreeCommandBuffers(device, m_transfer_command_pool, 1, &submit_data.transfer_command_buffer);

                    KW_ASSERT(
                        (submit_data.staging_data_end >= staging_data_begin && submit_data.staging_data_end <= staging_data_end) ||
                        (submit_data.staging_data_end >= staging_data_end && submit_data.staging_data_end <= staging_data_begin)
                    );

                    m_staging_data_begin.store(submit_data.staging_data_end, std::memory_order_release);

                    m_submit_data.pop();

                    m_submit_data_mutex.unlock();
                } else {
                    // Seems like some other thread is flushing memory for us. It can take a while. Wait for it.
                    std::lock_guard<std::recursive_mutex> lock(m_submit_data_mutex);
                }
            }
        }
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

BufferVulkan* RenderVulkan::create_buffer_vulkan(const BufferDescriptor& buffer_descriptor, VkBufferUsageFlags usage) {
    KW_ASSERT(buffer_descriptor.name != nullptr, "Invalid buffer name.");
    KW_ASSERT(buffer_descriptor.data != nullptr, "Invalid buffer data.");
    KW_ASSERT(buffer_descriptor.size > 0, "Invalid buffer data size.");

    //
    // Create device buffer and query its memory requirements.
    //

    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.flags = 0;
    buffer_create_info.size = static_cast<VkDeviceSize>(buffer_descriptor.size);
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = 0;

    VkBuffer device_buffer;
    VK_ERROR(
        vkCreateBuffer(device, &buffer_create_info, &allocation_callbacks, &device_buffer),
        "Failed to create device buffer \"%s\".", buffer_descriptor.name
    );

    if ((usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) == VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) {
        VK_NAME(*this, device_buffer, "Vertex buffer \"%s\"", buffer_descriptor.name);
    } else {
        VK_NAME(*this, device_buffer, "Index buffer \"%s\"", buffer_descriptor.name);
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, device_buffer, &memory_requirements);

    //
    // Find device memory range to store the buffer and bind the buffer to this range.
    //

    DeviceAllocation device_allocation = allocate_device_buffer_memory(memory_requirements.size, memory_requirements.alignment);
    KW_ASSERT(device_allocation.data_index < m_buffer_device_data.size());

    VK_ERROR(
        vkBindBufferMemory(device, device_buffer, device_allocation.memory, device_allocation.data_offset),
        "Failed to bind device buffer \"%s\" to device memory.", buffer_descriptor.name
    );

    //
    // If device allocation is host visible, we can simply memcpy our buffer there. Otherwise staging buffer is required.
    //

    if (m_buffer_device_data[device_allocation.data_index].memory_mapping != nullptr) {
        // Memory is mapped persistently so it can be accessed from multiple threads simultaneously.
        std::memcpy(static_cast<uint8_t*>(m_buffer_device_data[device_allocation.data_index].memory_mapping) + device_allocation.data_offset, buffer_descriptor.data, buffer_descriptor.size);

        BufferVulkan* result = persistent_memory_resource.allocate<BufferVulkan>();
        result->buffer = device_buffer;
        result->buffer_flags = buffer_descriptor.index_size == IndexSize::UINT16 ? BufferFlagsVulkan::INDEX16 : BufferFlagsVulkan::INDEX32;
        result->transfer_semaphore_value = 0; // Don't wait for transfer queue.
        result->device_data_index = device_allocation.data_index;
        result->device_data_offset = device_allocation.data_offset;
        return result;
    } else {
        //
        // Find staging memory range to store the buffer data and upload the buffer data to this range.
        //

        uint64_t staging_buffer_offset = allocate_from_staging_memory(static_cast<uint64_t>(buffer_descriptor.size), 1);

        // Memory is mapped persistently so it can be accessed from multiple threads simultaneously.
        std::memcpy(static_cast<uint8_t*>(m_staging_memory_mapping) + staging_buffer_offset, buffer_descriptor.data, buffer_descriptor.size);

        //
        // Enqueue copy command and return.
        //

        BufferCopyCommand buffer_copy_command{};
        buffer_copy_command.buffer = device_buffer;
        buffer_copy_command.staging_buffer_offset = staging_buffer_offset;
        buffer_copy_command.staging_buffer_size = buffer_descriptor.size;

        // If this resource is used in a draw call or dispatch, the following submit must wait for this semaphore value.
        uint64_t semaphore_value;

        {
            std::lock_guard<std::mutex> lock(m_buffer_copy_command_mutex);

            m_buffer_copy_commands.push_back(buffer_copy_command);

            // The fact that we locked `m_buffer_copy_command_mutex` means there's no `submit_copy_commands` in parallel
            // and therefore semaphore value will be increased only after new buffer copy command is processed.
            semaphore_value = semaphore->value + 1;
        }

        BufferVulkan* result = persistent_memory_resource.allocate<BufferVulkan>();
        result->buffer = device_buffer;
        result->buffer_flags = buffer_descriptor.index_size == IndexSize::UINT16 ? BufferFlagsVulkan::INDEX16 : BufferFlagsVulkan::INDEX32;
        result->transfer_semaphore_value = semaphore_value;
        result->device_data_index = device_allocation.data_index;
        result->device_data_offset = device_allocation.data_offset;
        return result;
    }
}

void RenderVulkan::destroy_buffer_vulkan(BufferVulkan* buffer) {
    KW_ASSERT(
        (buffer->buffer_flags & BufferFlagsVulkan::TRANSIENT) == BufferFlagsVulkan::NONE,
        "Transient buffers must not be destroyed manually."
    );

    std::lock_guard<std::mutex> lock(m_buffer_destroy_command_mutex);
    m_buffer_destroy_commands.push(BufferDestroyCommand{ get_destroy_command_dependencies(), buffer });
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

void* RenderVulkan::map_memory(VkDeviceMemory memory) {
    void* result;
    
    VK_ERROR(
        vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &result),
        "Failed to map memory."
    );

    return result;
}

uint64_t RenderVulkan::allocate_from_transient_memory(uint64_t size, uint64_t alignment) {
    KW_ASSERT(
        size + alignment - 1 <= m_transient_buffer_size,
        "Transient allocation is too big. Requested %llu, allowed %llu.", size + alignment - 1, m_transient_buffer_size
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

BufferVulkan* RenderVulkan::acquire_transient_buffer_vulkan(const void* data, size_t size, size_t alignment, BufferFlagsVulkan flags) {
    KW_ASSERT(data != nullptr, "Invalid buffer data.");
    KW_ASSERT(size > 0, "Invalid buffer data size.");

    uint64_t transient_buffer_offset = allocate_from_transient_memory(static_cast<uint64_t>(size), static_cast<uint64_t>(alignment));

    // Memory is mapped persistently so it can be accessed from multiple threads simultaneously.
    std::memcpy(static_cast<uint8_t*>(m_transient_memory_mapping) + transient_buffer_offset, data, size);

    BufferVulkan* result = transient_memory_resource.allocate<BufferVulkan>();
    result->buffer = m_transient_buffer;
    result->buffer_flags = flags | BufferFlagsVulkan::TRANSIENT;
    result->transfer_semaphore_value = 0; // Don't wait for transfer queue.
    result->device_data_index = UINT16_MAX;
    result->device_data_offset = transient_buffer_offset;
    return result;
}

uint32_t RenderVulkan::compute_texture_memory_index(VkMemoryPropertyFlags properties) {
    uint32_t memory_type_mask = UINT32_MAX;

    for (size_t i = 1; i < TEXTURE_FORMAT_COUNT; i++) {
        TextureFormat format = static_cast<TextureFormat>(i);

        if (format == TextureFormat::RGB32_FLOAT || format == TextureFormat::RGB32_SINT || format == TextureFormat::RGB32_UINT) {
            // TODO: Perhaps some mapping?
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

    uint32_t texture_memory_index = find_memory_type(memory_type_mask, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    KW_ERROR(texture_memory_index != UINT32_MAX, "Failed to find texture memory type.");

    return texture_memory_index;
}

TextureVulkan* RenderVulkan::create_texture_vulkan(const TextureDescriptor& texture_descriptor) {
    KW_ASSERT(texture_descriptor.name != nullptr, "Invalid texture name.");
    KW_ASSERT(texture_descriptor.data != nullptr, "Invalid texture data.");
    KW_ASSERT(texture_descriptor.size > 0, "Invalid texture data size.");

    //
    // Validation.
    //

    uint32_t max_side = std::max(texture_descriptor.width, std::max(texture_descriptor.height, texture_descriptor.depth));

    KW_ERROR(texture_descriptor.format != TextureFormat::UNKNOWN, "Invalid texture \"%s\" format.", texture_descriptor.name);
    KW_ERROR(texture_descriptor.mip_levels <= log2(max_side) + 1, "Invalid texture \"%s\" mip levels.", texture_descriptor.name);
    KW_ERROR(texture_descriptor.width > 0 && is_pow2(texture_descriptor.width), "Invalid texture \"%s\" width.", texture_descriptor.name);
    KW_ERROR(texture_descriptor.height > 0 && is_pow2(texture_descriptor.height), "Invalid texture \"%s\" height.", texture_descriptor.name);
    KW_ERROR(texture_descriptor.depth == 0 || is_pow2(texture_descriptor.depth), "Invalid texture \"%s\" depth.", texture_descriptor.name);

    switch (texture_descriptor.type) {
    case TextureType::TEXTURE_2D:
        KW_ERROR(texture_descriptor.array_size <= 1, "Invalid texture \"%s\" array size.", texture_descriptor.name);
        KW_ERROR(texture_descriptor.depth <= 1, "Invalid texture \"%s\" depth.", texture_descriptor.name);
        break;
    case TextureType::TEXTURE_CUBE:
        KW_ERROR(texture_descriptor.array_size == 6, "Invalid texture \"%s\" array size.", texture_descriptor.name);
        KW_ERROR(texture_descriptor.mip_levels <= 1, "Invalid texture \"%s\" mip levels.", texture_descriptor.name);
        KW_ERROR(texture_descriptor.width == texture_descriptor.height, "Invalid texture \"%s\" size.", texture_descriptor.name);
        KW_ERROR(texture_descriptor.depth <= 1, "Invalid texture \"%s\" depth.", texture_descriptor.name);
        break;
    case TextureType::TEXTURE_3D:
        KW_ERROR(texture_descriptor.array_size <= 1, "Invalid texture \"%s\" array size.", texture_descriptor.name);
        break;
    case TextureType::TEXTURE_2D_ARRAY:
        KW_ERROR(texture_descriptor.depth <= 1, "Invalid texture \"%s\" depth.", texture_descriptor.name);
        break;
    case TextureType::TEXTURE_CUBE_ARRAY:
        KW_ERROR(texture_descriptor.array_size % 6 == 0, "Invalid texture \"%s\" array size.", texture_descriptor.name);
        KW_ERROR(texture_descriptor.mip_levels <= 1, "Invalid texture \"%s\" mip levels.", texture_descriptor.name);
        KW_ERROR(texture_descriptor.width == texture_descriptor.height, "Invalid texture \"%s\" size.", texture_descriptor.name);
        KW_ERROR(texture_descriptor.depth <= 1, "Invalid texture \"%s\" depth.", texture_descriptor.name);
        break;
    }

    //
    // Compute image types and flags.
    //

    VkImageType image_type;
    VkImageCreateFlags image_create_flags;
    VkImageViewType image_view_type;

    switch (texture_descriptor.type) {
    case TextureType::TEXTURE_2D:
        image_type = VK_IMAGE_TYPE_2D;
        image_create_flags = 0;
        image_view_type = VK_IMAGE_VIEW_TYPE_2D;
        break;
    case TextureType::TEXTURE_CUBE:
        image_type = VK_IMAGE_TYPE_2D;
        image_create_flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        image_view_type = VK_IMAGE_VIEW_TYPE_CUBE;
        break;
    case TextureType::TEXTURE_3D:
        image_type = VK_IMAGE_TYPE_3D;
        image_create_flags = 0;
        image_view_type = VK_IMAGE_VIEW_TYPE_3D;
        break;
    case TextureType::TEXTURE_2D_ARRAY:
        image_type = VK_IMAGE_TYPE_2D;
        image_create_flags = 0;
        image_view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        break;
    case TextureType::TEXTURE_CUBE_ARRAY:
        image_type = VK_IMAGE_TYPE_2D;
        image_create_flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        image_view_type = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
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
    image_create_info.mipLevels = std::max(texture_descriptor.mip_levels, 1U);
    image_create_info.arrayLayers = std::max(texture_descriptor.array_size, 1U);
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
    // Create image view.
    //

    VkImageAspectFlags aspect_mask;
    if (TextureFormatUtils::is_depth_stencil(texture_descriptor.format)) {
        // Sampled depth stencil textures provide access only to depth.
        aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
    } else {
        aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkImageSubresourceRange image_subresource_range{};
    image_subresource_range.aspectMask = aspect_mask;
    image_subresource_range.baseMipLevel = 0;
    image_subresource_range.levelCount = VK_REMAINING_MIP_LEVELS;
    image_subresource_range.baseArrayLayer = 0;
    image_subresource_range.layerCount = VK_REMAINING_ARRAY_LAYERS;

    VkImageViewCreateInfo image_view_create_info{};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.flags = 0;
    image_view_create_info.image = image;
    image_view_create_info.viewType = image_view_type;
    image_view_create_info.format = image_create_info.format;
    image_view_create_info.subresourceRange = image_subresource_range;

    VkImageView image_view;
    VK_ERROR(
        vkCreateImageView(device, &image_view_create_info, &allocation_callbacks, &image_view),
        "Failed to create image view \"%s\".", texture_descriptor.name
    );
    VK_NAME(*this, image_view, "Texture view \"%s\"", texture_descriptor.name);

    //
    // Find staging memory range to store the texture data and upload the texture data to this range.
    //

    uint64_t staging_buffer_offset = allocate_from_staging_memory(static_cast<uint64_t>(texture_descriptor.size), 16);

    // Memory is mapped persistently so it can be accessed from multiple threads simultaneously.
    std::memcpy(static_cast<uint8_t*>(m_staging_memory_mapping) + staging_buffer_offset, texture_descriptor.data, texture_descriptor.size);

    //
    // Enqueue copy command and return.
    //

    size_t* offsets = persistent_memory_resource.allocate<size_t>(image_create_info.arrayLayers * image_create_info.mipLevels);
    std::memcpy(offsets, texture_descriptor.offsets, sizeof(size_t) * image_create_info.arrayLayers * image_create_info.mipLevels);

    TextureCopyCommand texture_copy_command{};
    texture_copy_command.staging_buffer_offset = staging_buffer_offset;
    texture_copy_command.staging_buffer_size = static_cast<uint64_t>(texture_descriptor.size);
    texture_copy_command.image = image;
    texture_copy_command.aspect_mask = aspect_mask;
    texture_copy_command.array_size = image_create_info.arrayLayers;
    texture_copy_command.mip_levels = image_create_info.mipLevels;
    texture_copy_command.width = image_create_info.extent.width;
    texture_copy_command.height = image_create_info.extent.height;
    texture_copy_command.depth = image_create_info.extent.depth;
    texture_copy_command.offsets = offsets;

    // If this resource is used in a draw call or dispatch, the following submit must wait for this semaphore value.
    uint64_t semaphore_value;

    {
        std::lock_guard<std::mutex> lock(m_texture_copy_command_mutex);

        m_texture_copy_commands.push_back(texture_copy_command);

        // The fact that we locked `m_texture_copy_command_mutex` means there's no `submit_copy_commands` in parallel
        // and therefore semaphore value will be increased only after new texture copy command is processed.
        semaphore_value = semaphore->value + 1;
    }

    TextureVulkan* result = persistent_memory_resource.allocate<TextureVulkan>();
    result->image = image;
    result->image_view = image_view;
    result->transfer_semaphore_value = semaphore_value;
    result->device_data_index = device_allocation.data_index;
    result->device_data_offset = device_allocation.data_offset;
    return result;
}

void RenderVulkan::destroy_texture_vulkan(TextureVulkan* texture) {
    std::lock_guard<std::mutex> lock(m_texture_destroy_command_mutex);
    m_texture_destroy_commands.push(TextureDestroyCommand{ get_destroy_command_dependencies(), texture });
}

Vector<RenderVulkan::DestroyCommandDependency> RenderVulkan::get_destroy_command_dependencies() {
    std::lock_guard<std::mutex> lock(m_resource_dependency_mutex);

    Vector<DestroyCommandDependency> result(persistent_memory_resource);
    result.reserve(m_resource_dependencies.size() + 1);

    // In case of "create, destroy, flush" rather than "create, flush, destroy" we'd want to postpone destroy
    // until the first flush. Otherwise resource will stay in a creation queue after destruction and cause invalid
    // memory access. That could be avoided by checking in destroy whether the resource is in a creation queue
    // and removing it from there. However the "create, destroy, flush" case is kind of irrational, so postponing
    // destroy seems like a good compromise.
    result.push_back(DestroyCommandDependency{ semaphore, semaphore->value + 1 });

    for (size_t i = 0; i < m_resource_dependencies.size(); ) {
        if (std::shared_ptr<TimelineSemaphore> shared_timeline_semaphore = m_resource_dependencies[i].lock()) {
            result.push_back(DestroyCommandDependency{ shared_timeline_semaphore, shared_timeline_semaphore->value });
            i++;
        } else {
            std::swap(m_resource_dependencies[i], m_resource_dependencies.back());
            m_resource_dependencies.pop_back();
        }
    }

    return result;
}

void RenderVulkan::process_completed_submits() {
    std::lock_guard<std::recursive_mutex> lock(m_submit_data_mutex);

    while (!m_submit_data.empty()) {
        SubmitData& submit_data = m_submit_data.front();

        VkSemaphoreWaitInfo semaphore_wait_info{};
        semaphore_wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        semaphore_wait_info.flags = 0;
        semaphore_wait_info.semaphoreCount = 1;
        semaphore_wait_info.pSemaphores = &semaphore->semaphore;
        semaphore_wait_info.pValues = &submit_data.semaphore_value;

        if (m_wait_semaphores(device, &semaphore_wait_info, 0) == VK_SUCCESS) {
            if (submit_data.graphics_command_buffer != VK_NULL_HANDLE) {
                vkFreeCommandBuffers(device, m_graphics_command_pool, 1, &submit_data.graphics_command_buffer);
            }

            if (submit_data.compute_command_buffer != VK_NULL_HANDLE) {
                vkFreeCommandBuffers(device, m_compute_command_pool, 1, &submit_data.compute_command_buffer);
            }

            KW_ASSERT(submit_data.transfer_command_buffer != VK_NULL_HANDLE);
            vkFreeCommandBuffers(device, m_transfer_command_pool, 1, &submit_data.transfer_command_buffer);

            KW_ASSERT(
                (submit_data.staging_data_end >= m_staging_data_begin && submit_data.staging_data_end <= m_staging_data_end) ||
                (submit_data.staging_data_end >= m_staging_data_end && submit_data.staging_data_end <= m_staging_data_begin)
            );

            m_staging_data_begin.store(submit_data.staging_data_end, std::memory_order_release);

            m_submit_data.pop();
        } else {
            // The following submits in a queue have greater semaphore values.
            break;
        }
    }
}

void RenderVulkan::destroy_queued_buffers() {
    std::lock_guard<std::mutex> lock(m_buffer_destroy_command_mutex);

    while (!m_buffer_destroy_commands.empty()) {
        BufferDestroyCommand& buffer_destroy_command = m_buffer_destroy_commands.front();

        Vector<std::shared_ptr<TimelineSemaphore>> timeline_semaphores(transient_memory_resource);
        timeline_semaphores.reserve(buffer_destroy_command.dependencies.size());

        Vector<VkSemaphore> semaphores(transient_memory_resource);
        semaphores.reserve(buffer_destroy_command.dependencies.size());

        Vector<uint64_t> values(transient_memory_resource);
        values.reserve(buffer_destroy_command.dependencies.size());

        for (DestroyCommandDependency& destroy_command_dependency : buffer_destroy_command.dependencies) {
            if (std::shared_ptr<TimelineSemaphore> timeline_semaphore = destroy_command_dependency.semaphore.lock()) {
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

        if (m_wait_semaphores(device, &semaphore_wait_info, 0) == VK_SUCCESS) {
            // Transfer semaphore is also a destroy dependency. If it just signaled, we need to destroy command buffer
            // before destroying the buffer, because the former may have dependency on the latter.
            process_completed_submits();

            vkDestroyBuffer(device, buffer_destroy_command.buffer->buffer, &allocation_callbacks);
            deallocate_device_buffer_memory(buffer_destroy_command.buffer->device_data_index, buffer_destroy_command.buffer->device_data_offset);
            persistent_memory_resource.deallocate(buffer_destroy_command.buffer);

            m_buffer_destroy_commands.pop();
        } else {
            // The following resources in a queue have greater or equal semaphore values.
            break;
        }
    }
}

void RenderVulkan::destroy_queued_textures() {
    std::lock_guard<std::mutex> lock(m_texture_destroy_command_mutex);

    while (!m_texture_destroy_commands.empty()) {
        TextureDestroyCommand& texture_destroy_command = m_texture_destroy_commands.front();

        Vector<std::shared_ptr<TimelineSemaphore>> timeline_semaphores(transient_memory_resource);
        timeline_semaphores.reserve(texture_destroy_command.dependencies.size());

        Vector<VkSemaphore> semaphores(transient_memory_resource);
        semaphores.reserve(texture_destroy_command.dependencies.size());

        Vector<uint64_t> values(transient_memory_resource);
        values.reserve(texture_destroy_command.dependencies.size());

        for (DestroyCommandDependency& destroy_command_dependency : texture_destroy_command.dependencies) {
            if (std::shared_ptr<TimelineSemaphore> timeline_semaphore = destroy_command_dependency.semaphore.lock()) {
                values.push_back(destroy_command_dependency.value);
                semaphores.push_back(timeline_semaphore->semaphore);
                timeline_semaphores.push_back(std::move(timeline_semaphore));
            }
        }

        VkSemaphoreWaitInfo semaphore_wait_info{};
        semaphore_wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        semaphore_wait_info.flags = 0;
        semaphore_wait_info.semaphoreCount = static_cast<uint32_t>(semaphores.size());
        semaphore_wait_info.pSemaphores = semaphores.data();
        semaphore_wait_info.pValues = values.data();

        if (m_wait_semaphores(device, &semaphore_wait_info, 0) == VK_SUCCESS) {
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

void RenderVulkan::submit_copy_commands() {
    std::scoped_lock buffer_texture_lock(m_buffer_copy_command_mutex, m_texture_copy_command_mutex);

    if (!m_buffer_copy_commands.empty() || !m_texture_copy_commands.empty()) {
        std::lock_guard<std::recursive_mutex> submit_data_lock(m_submit_data_mutex);

        //
        // Create new command buffer.
        //

        VkCommandBufferAllocateInfo transfer_command_buffer_allocate_info{};
        transfer_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        transfer_command_buffer_allocate_info.commandPool = m_transfer_command_pool;
        transfer_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        transfer_command_buffer_allocate_info.commandBufferCount = 1;

        VkCommandBuffer transfer_command_buffer;
        VK_ERROR(
            vkAllocateCommandBuffers(device, &transfer_command_buffer_allocate_info, &transfer_command_buffer),
            "Failed to allocate transfer command buffer."
        );
        VK_NAME(*this, transfer_command_buffer, "Transfer command buffer");

        //
        // Begin command buffer.
        //

        VkCommandBufferBeginInfo transfer_command_buffer_begin_info{};
        transfer_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        transfer_command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_ERROR(
            vkBeginCommandBuffer(transfer_command_buffer, &transfer_command_buffer_begin_info),
            "Failed to begin a transfer command buffer."
        );

        //
        // Copy buffers.
        //

        uint64_t staging_data_end = 0;

        for (BufferCopyCommand& buffer_copy_command : m_buffer_copy_commands) {
            VkBufferCopy buffer_copy{};
            buffer_copy.srcOffset = buffer_copy_command.staging_buffer_offset;
            buffer_copy.dstOffset = 0;
            buffer_copy.size = buffer_copy_command.staging_buffer_size;

            vkCmdCopyBuffer(transfer_command_buffer, m_staging_buffer, buffer_copy_command.buffer, 1, &buffer_copy);

            if (transfer_queue_family_index != graphics_queue_family_index) {
                VkBufferMemoryBarrier release_barrier{};
                release_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                release_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                release_barrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
                release_barrier.srcQueueFamilyIndex = transfer_queue_family_index;
                release_barrier.dstQueueFamilyIndex = graphics_queue_family_index;
                release_barrier.buffer = buffer_copy_command.buffer;
                release_barrier.offset = 0;
                release_barrier.size = VK_WHOLE_SIZE;

                vkCmdPipelineBarrier(
                    transfer_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
                    0, nullptr, 1, &release_barrier, 0, nullptr
                );
            }

            staging_data_end = std::max(staging_data_end, buffer_copy_command.staging_buffer_offset + buffer_copy_command.staging_buffer_size);
        }

        // Keep `m_buffer_copy_commands` items for ownership transfer.

        //
        // Copy textures.
        //

        for (TextureCopyCommand& texture_copy_command : m_texture_copy_commands) {
            VkImageSubresourceRange image_subresource_range{};
            image_subresource_range.aspectMask = texture_copy_command.aspect_mask;
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
            acquire_barrier.image = texture_copy_command.image;
            acquire_barrier.subresourceRange = image_subresource_range;

            vkCmdPipelineBarrier(
                transfer_command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr, 0, nullptr, 1, &acquire_barrier
            );

            Vector<VkBufferImageCopy> buffer_image_copies(transient_memory_resource);
            buffer_image_copies.reserve(texture_copy_command.array_size * texture_copy_command.mip_levels);

            for (uint32_t array_index = 0; array_index < texture_copy_command.array_size; array_index++) {
                uint32_t width = texture_copy_command.width;
                uint32_t height = texture_copy_command.height;
                uint32_t depth = texture_copy_command.depth;

                for (uint32_t mip_index = 0; mip_index < texture_copy_command.mip_levels; mip_index++) {
                    size_t array_mip_offset = texture_copy_command.offsets[array_index * texture_copy_command.mip_levels + mip_index];

                    VkImageSubresourceLayers image_subresource_layers{};
                    image_subresource_layers.aspectMask = texture_copy_command.aspect_mask;
                    image_subresource_layers.mipLevel = mip_index;
                    image_subresource_layers.baseArrayLayer = array_index;
                    image_subresource_layers.layerCount = 1;

                    VkOffset3D offset{};
                    offset.x = 0;
                    offset.y = 0;
                    offset.z = 0;

                    VkExtent3D extent{};
                    extent.width = width;
                    extent.height = height;
                    extent.depth = depth;

                    VkBufferImageCopy buffer_image_copy{};
                    buffer_image_copy.bufferOffset = texture_copy_command.staging_buffer_offset + static_cast<VkDeviceSize>(array_mip_offset);
                    buffer_image_copy.bufferRowLength = 0;
                    buffer_image_copy.bufferImageHeight = 0;
                    buffer_image_copy.imageSubresource = image_subresource_layers;
                    buffer_image_copy.imageOffset = offset;
                    buffer_image_copy.imageExtent = extent;

                    buffer_image_copies.push_back(buffer_image_copy);

                    width = std::max(width / 2, 1U);
                    height = std::max(height / 2, 1U);
                    depth = std::max(depth / 2, 1U);
                }
            }

            vkCmdCopyBufferToImage(
                transfer_command_buffer, m_staging_buffer, texture_copy_command.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                static_cast<uint32_t>(buffer_image_copies.size()), buffer_image_copies.data()
            );

            VkImageMemoryBarrier release_barrier{};
            release_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            release_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            release_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            release_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            release_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            release_barrier.srcQueueFamilyIndex = transfer_queue_family_index;
            release_barrier.dstQueueFamilyIndex = graphics_queue_family_index;
            release_barrier.image = texture_copy_command.image;
            release_barrier.subresourceRange = image_subresource_range;

            vkCmdPipelineBarrier(
                transfer_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
                0, nullptr, 0, nullptr, 1, &release_barrier
            );

            persistent_memory_resource.deallocate(texture_copy_command.offsets);

            staging_data_end = std::max(staging_data_end, texture_copy_command.staging_buffer_offset + texture_copy_command.staging_buffer_size);
        }

        // Keep `m_texture_copy_commands` items for ownership transfer.

        //
        // If transfer and graphics queue are from different families, the copy commands are submitted to a transfer
        // queue and ownership transfer commands are submitted to a graphics queue. Resource users wait for the latter,
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
            std::lock_guard<Spinlock> lock(*transfer_queue_spinlock);

            VK_ERROR(
                vkQueueSubmit(transfer_queue, 1, &transfer_submit_info, VK_NULL_HANDLE),
                "Failed to submit copy commands to a transfer queue."
            );
        }

        //
        // If transfer and graphics queue are from different families, queue ownership transfer must be performed.
        //

        VkCommandBuffer graphics_command_buffer = VK_NULL_HANDLE;

        if (transfer_queue_family_index != graphics_queue_family_index) {
            //
            // Create new command buffer.
            //

            VkCommandBufferAllocateInfo graphics_command_buffer_allocate_info{};
            graphics_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            graphics_command_buffer_allocate_info.commandPool = m_graphics_command_pool;
            graphics_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            graphics_command_buffer_allocate_info.commandBufferCount = 1;

            VK_ERROR(
                vkAllocateCommandBuffers(device, &graphics_command_buffer_allocate_info, &graphics_command_buffer),
                "Failed to allocate graphics command buffer."
            );
            VK_NAME(*this, graphics_command_buffer, "Graphics command buffer");

            //
            // Begin command buffer.
            //

            VkCommandBufferBeginInfo graphics_command_buffer_begin_info{};
            graphics_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            graphics_command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            VK_ERROR(
                vkBeginCommandBuffer(graphics_command_buffer, &graphics_command_buffer_begin_info),
                "Failed to begin a graphics command buffer."
            );

            //
            // Transfer buffer ownership.
            //

            for (BufferCopyCommand& buffer_copy_command : m_buffer_copy_commands) {
                VkBufferMemoryBarrier acquire_barrier{};
                acquire_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                acquire_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                acquire_barrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
                acquire_barrier.srcQueueFamilyIndex = transfer_queue_family_index;
                acquire_barrier.dstQueueFamilyIndex = graphics_queue_family_index;
                acquire_barrier.buffer = buffer_copy_command.buffer;
                acquire_barrier.offset = 0;
                acquire_barrier.size = VK_WHOLE_SIZE;

                vkCmdPipelineBarrier(
                    graphics_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
                    0, nullptr, 1, &acquire_barrier, 0, nullptr
                );
            }

            //
            // Transfer texture ownership.
            //

            for (TextureCopyCommand& texture_copy_command : m_texture_copy_commands) {
                VkImageSubresourceRange image_subresource_range{};
                image_subresource_range.aspectMask = texture_copy_command.aspect_mask;
                image_subresource_range.baseMipLevel = 0;
                image_subresource_range.levelCount = VK_REMAINING_MIP_LEVELS;
                image_subresource_range.baseArrayLayer = 0;
                image_subresource_range.layerCount = VK_REMAINING_ARRAY_LAYERS;

                VkImageMemoryBarrier acquire_barrier{};
                acquire_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                acquire_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                acquire_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                acquire_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                acquire_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                acquire_barrier.srcQueueFamilyIndex = transfer_queue_family_index;
                acquire_barrier.dstQueueFamilyIndex = graphics_queue_family_index;
                acquire_barrier.image = texture_copy_command.image;
                acquire_barrier.subresourceRange = image_subresource_range;

                vkCmdPipelineBarrier(
                    graphics_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
                    0, nullptr, 0, nullptr, 1, &acquire_barrier
                );
            }

            //
            // End graphics command buffer and submit it to a graphics queue.
            //

            VK_ERROR(
                vkEndCommandBuffer(graphics_command_buffer),
                "Failed to end a transfer command buffer."
            );

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
                std::lock_guard<Spinlock> lock(*graphics_queue_spinlock);

                VK_ERROR(
                    vkQueueSubmit(graphics_queue, 1, &graphics_submit_info, VK_NULL_HANDLE),
                    "Failed to submit ownership transfer commands to a graphics queue."
                );
            }
        }

        m_buffer_copy_commands.clear();
        m_texture_copy_commands.clear();

        //
        // Destroy command buffer when copy commands completed on device.
        //

        m_submit_data.push(SubmitData{ transfer_command_buffer, VK_NULL_HANDLE, graphics_command_buffer, semaphore->value, staging_data_end });
    }
}

} // namespace kw
