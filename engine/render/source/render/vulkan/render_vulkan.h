#pragma once

#include "render/render.h"

#include <vulkan/vulkan.h>

namespace kw {

class RenderVulkan : public Render {
public:
    RenderVulkan(const RenderDescriptor& descriptor);
    ~RenderVulkan() override;

    RenderApi get_api() const override;

    template <typename T>
    void set_debug_name(T* handle, const char* format, ...) const;

    const VkAllocationCallbacks allocation_callbacks;
    const VkInstance instance;
    const VkPhysicalDevice physical_device;
    const VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    const VkPhysicalDeviceProperties physical_device_properties;
    const uint32_t queue_family_index;
    const VkDevice device;
    const VkQueue queue;

private:
    VkInstance create_instance(const RenderDescriptor& descriptor);
    VkPhysicalDevice create_physical_device(const RenderDescriptor& descriptor);
    VkDevice create_device(const RenderDescriptor& descriptor);
    VkQueue create_queue(const RenderDescriptor& descriptor);

    VkPhysicalDeviceMemoryProperties get_physical_device_memory_properties(const RenderDescriptor& descriptor);
    VkPhysicalDeviceProperties get_physical_device_properties(const RenderDescriptor& descriptor);
    uint32_t get_queue_family_index(const RenderDescriptor& descriptor);

    void create_debug_utils(const RenderDescriptor& descriptor);

    VkDebugUtilsMessengerEXT m_debug_messenger = VK_NULL_HANDLE;
    PFN_vkSetDebugUtilsObjectNameEXT m_set_object_name = nullptr;
};

} // namespace kw
