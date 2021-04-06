#pragma once

#include <vulkan/vulkan.h>

#include <atomic>

namespace kw {

class RenderVulkan;

class TimelineSemaphore {
public:
    TimelineSemaphore(RenderVulkan* render);
    TimelineSemaphore(const TimelineSemaphore& original) = delete;
    TimelineSemaphore(TimelineSemaphore&& original) = delete;
    ~TimelineSemaphore();
    TimelineSemaphore& operator=(const TimelineSemaphore& original) = delete;
    TimelineSemaphore& operator=(TimelineSemaphore&& original) = delete;

    VkSemaphore semaphore;
    std::atomic_uint64_t value;

private:
    RenderVulkan* m_render;
};

} // namespace kw
