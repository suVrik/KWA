#include "render/vulkan/timeline_semaphore.h"
#include "render/vulkan/render_vulkan.h"
#include "render/vulkan/vulkan_utils.h"

namespace kw {

static VkSemaphore create_timeline_semaphore(RenderVulkan* render) {
    VkSemaphoreTypeCreateInfo semaphore_type_create_info{};
    semaphore_type_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    semaphore_type_create_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    semaphore_type_create_info.initialValue = 0;

    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = &semaphore_type_create_info;
    semaphore_create_info.flags = 0;

    VkSemaphore semaphore;
    VK_ERROR(
        vkCreateSemaphore(render->device, &semaphore_create_info, &render->allocation_callbacks, &semaphore),
        "Failed to create timeline semaphore."
    );

    return semaphore;
}

TimelineSemaphore::TimelineSemaphore(RenderVulkan* render)
    : semaphore(create_timeline_semaphore(render))
    , m_render(render) {
}

TimelineSemaphore::~TimelineSemaphore() {
    vkDestroySemaphore(m_render->device, semaphore, &m_render->allocation_callbacks);
}

} // namespace kw
