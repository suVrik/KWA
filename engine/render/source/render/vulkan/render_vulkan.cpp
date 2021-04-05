#include "render/vulkan/render_vulkan.h"
#include "render/vulkan/vulkan_utils.h"

#include <core/vector.h>

#include <debug/assert.h>
#include <debug/log.h>

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
    : allocation_callbacks{ descriptor.memory_resource, &vk_alloc, &vk_realloc, &vk_free }
    , instance(create_instance(descriptor))
    , physical_device(create_physical_device(descriptor))
    , physical_device_memory_properties(get_physical_device_memory_properties(descriptor))
    , physical_device_properties(get_physical_device_properties(descriptor))
    , queue_family_index(get_queue_family_index(descriptor))
    , device(create_device(descriptor))
    , queue(create_queue(descriptor))
{
    create_debug_utils(descriptor);
    VK_NAME(this, queue, "Queue");
}

RenderVulkan::~RenderVulkan() {
    vkDeviceWaitIdle(device);

    vkDestroyDevice(device, &allocation_callbacks);

    if (m_debug_messenger != nullptr) {
        auto vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        KW_ERROR(vkDestroyDebugUtilsMessengerEXT != nullptr, "Failed to get vkDestroyDebugUtilsMessengerEXT function.");
        
        vkDestroyDebugUtilsMessengerEXT(instance, m_debug_messenger, &allocation_callbacks);
    }

    vkDestroyInstance(instance, &allocation_callbacks);
}

RenderApi RenderVulkan::get_api() const {
    return RenderApi::VULKAN;
}

VkInstance RenderVulkan::create_instance(const RenderDescriptor& descriptor) {
    VkApplicationInfo application_info{};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName = "KURWA";
    application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info.pEngineName = "KURWA";
    application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info.apiVersion = VK_API_VERSION_1_0;

    uint32_t instance_layer_count = 0;
    const char* instance_layers[1]{};

    if (descriptor.is_validation_enabled) {
        instance_layers[instance_layer_count++] = "VK_LAYER_KHRONOS_validation";
    }

    uint32_t instance_extensions_count;
    SDL_ERROR(
        SDL_Vulkan_GetInstanceExtensions(nullptr, &instance_extensions_count, nullptr),
        "Failed to get instance extension count."
    );

    VectorLinear<const char*> instance_extensions(instance_extensions_count + 1, descriptor.memory_resource);
    SDL_ERROR(
        SDL_Vulkan_GetInstanceExtensions(nullptr, &instance_extensions_count, instance_extensions.data()),
        "Failed to get instance extensions."
    );

    if (descriptor.is_validation_enabled || descriptor.is_debug_names_enabled) {
        instance_extensions[instance_extensions_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }

    VkInstanceCreateInfo instance_create_info{};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &application_info;
    instance_create_info.enabledLayerCount = instance_layer_count;
    instance_create_info.ppEnabledLayerNames = instance_layers;
    instance_create_info.enabledExtensionCount = instance_extensions_count;
    instance_create_info.ppEnabledExtensionNames = instance_extensions.data();

    VkInstance instance;
    VK_ERROR(
        vkCreateInstance(&instance_create_info, &allocation_callbacks, &instance),
        "Failed to create an instance."
    );

    return instance;
}

void RenderVulkan::create_debug_utils(const RenderDescriptor& descriptor) {
    if (descriptor.is_debug_names_enabled) {
        m_set_object_name = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT"));
        KW_ERROR(
            m_set_object_name != nullptr,
            "Failed to get vkSetDebugUtilsObjectNameEXT function."
        );
    }

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

        VK_ERROR(
            vkCreateDebugUtilsMessengerEXT(instance, &debug_utils_messenger_create_info, &allocation_callbacks, &m_debug_messenger),
            "Failed to create debug messenger."
        );
    }
}

VkPhysicalDevice RenderVulkan::create_physical_device(const RenderDescriptor& descriptor) {
    uint32_t physical_device_count;
    VK_ERROR(
        vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr),
        "Failed to query physical device count."
    );

    VectorLinear<VkPhysicalDevice> physical_devices(physical_device_count, descriptor.memory_resource);
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

            VectorLinear<VkQueueFamilyProperties> queue_families(queue_family_count, descriptor.memory_resource);
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

            for (uint32_t queue_family_index = 0; queue_family_index < queue_family_count; queue_family_index++) {
                VkQueueFlags required_queue_mask = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
                if ((queue_families[queue_family_index].queueFlags & required_queue_mask) == required_queue_mask) {
                    best_physical_device = physical_device;
                    best_physical_device_is_discrete = is_discrete;
                    best_physical_device_size = device_size;
                }
            }
        }
    }

    KW_ERROR(best_physical_device != VK_NULL_HANDLE, "Failed to find any physical device.");

    return best_physical_device;
}

VkPhysicalDeviceMemoryProperties RenderVulkan::get_physical_device_memory_properties(const RenderDescriptor& descriptor) {
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
    return memory_properties;
}

VkPhysicalDeviceProperties RenderVulkan::get_physical_device_properties(const RenderDescriptor& descriptor) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical_device, &properties);
    return properties;
}

uint32_t RenderVulkan::get_queue_family_index(const RenderDescriptor& descriptor) {
    uint32_t queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

    VectorLinear<VkQueueFamilyProperties> queue_families(queue_family_count, descriptor.memory_resource);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

    for (uint32_t queue_family_index = 0; queue_family_index < queue_family_count; queue_family_index++) {
        VkQueueFlags required_queue_mask = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
        if ((queue_families[queue_family_index].queueFlags & required_queue_mask) == required_queue_mask) {
            return queue_family_index;
        }
    }

    KW_ASSERT(false);
    return 0;
}

VkDevice RenderVulkan::create_device(const RenderDescriptor& descriptor) {
    float queue_priority = 1.f;

    VkDeviceQueueCreateInfo device_queue_create_info{};
    device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_queue_create_info.queueFamilyIndex = queue_family_index;
    device_queue_create_info.queueCount = 1;
    device_queue_create_info.pQueuePriorities = &queue_priority;

    uint32_t device_layer_count = descriptor.is_validation_enabled ? 1 : 0;
    const char* device_layer = "VK_LAYER_KHRONOS_validation";
    const char* device_extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

    VkPhysicalDeviceFeatures physical_device_features{};
    physical_device_features.independentBlend = VK_TRUE;
    physical_device_features.depthBiasClamp = VK_TRUE;
    physical_device_features.fillModeNonSolid = VK_TRUE;
    physical_device_features.textureCompressionBC = VK_TRUE;

    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VkStructureType::VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &device_queue_create_info;
    device_create_info.enabledLayerCount = device_layer_count;
    device_create_info.ppEnabledLayerNames = &device_layer;
    device_create_info.enabledExtensionCount = 1;
    device_create_info.ppEnabledExtensionNames = &device_extension;
    device_create_info.pEnabledFeatures = &physical_device_features;

    VkDevice device;
    VK_ERROR(
        vkCreateDevice(physical_device, &device_create_info, &allocation_callbacks, &device),
        "Failed to create a device."
    );

    return device;
}

VkQueue RenderVulkan::create_queue(const RenderDescriptor& descriptor) {
    VkQueue queue;
    vkGetDeviceQueue(device, queue_family_index, 0, &queue);
    return queue;
}

} // namespace kw
