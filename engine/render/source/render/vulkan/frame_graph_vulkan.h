#include "render/frame_graph.h"

#include <core/enum.h>
#include <core/string.h>
#include <core/unordered_map.h>
#include <core/vector.h>

#include <memory/linear_memory_resource.h>

#include <vulkan/vulkan.h>

namespace kw {

class RenderVulkan;

enum class AttachmentAccess : uint8_t {
    NONE            = 0b00000000,
    READ            = 0b00000001,
    WRITE           = 0b00000010,
    ATTACHMENT      = 0b00000100,
    VERTEX_SHADER   = 0b00001000,
    FRAGMENT_SHADER = 0b00010000,
    BLEND           = 0b00100000,
    LOAD            = 0b01000000,
    STORE           = 0b10000000,
};

KW_DEFINE_ENUM_BITMASK(AttachmentAccess);

class FrameGraphVulkan : public FrameGraph {
public:
    FrameGraphVulkan(const FrameGraphDescriptor& descriptor);
    ~FrameGraphVulkan() override;

    void render() override;
    void recreate_swapchain() override;

private:
    static constexpr size_t INVALID_VALUE = SIZE_MAX;
    static constexpr size_t SWAPCHAIN_IMAGE_COUNT = 3;

    struct CreateContext {
        const FrameGraphDescriptor& frame_graph_descriptor;

        // Mapping from attachment names to attachment indices.
        UnorderedMap<StringView, size_t> attachment_mapping;

        // Attachment_count x render_pass_count matrix of access to a certain attachment on a certain render pass.
        Vector<AttachmentAccess> attachment_access_matrix;

        // Allocate a piece of memory and reuse it for each graphics pipeline.
        LinearMemoryResource graphics_pipeline_memory_resource;
    };

    struct AttachmentRecreateData {
        size_t allocation_index = INVALID_VALUE;
        size_t allocation_offset = INVALID_VALUE;
    };

    struct AllocationRecreateData {
        size_t size = 0;
        uint32_t memory_type_index = 0;
    };

    struct RecreateContext {
        uint32_t swapchain_width = 0;
        uint32_t swapchain_height = 0;

        // To access attachments from largest to smallest use the `sorted_attachment_indices` mapping.
        Vector<VkMemoryRequirements> memory_requirements;
        Vector<size_t> sorted_attachment_indices;

        Vector<AttachmentRecreateData> attachment_data;
        Vector<AllocationRecreateData> allocation_data;
    };

    struct AttachmentData {
        VkImage image = VK_NULL_HANDLE;
        VkImageView image_view = VK_NULL_HANDLE;

        // Occasionally min index may be larger than max index. This means that the attachment is created at the end
        // of the frame and used at the beginning of the next frame and can be aliased in between.
        size_t min_parallel_block_index = 0;
        size_t max_parallel_block_index = 0;

        // Layout transition from `VK_IMAGE_LAYOUT_UNDEFINED` to specified image layout is performed manually
        // before the first render pass once after the attachment is created.
        VkAccessFlags initial_access_mask = 0;
        VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    struct GraphicsPipelineData {
        GraphicsPipelineData(MemoryResource& memory_resource);

        VkShaderModule vertex_shader_module = VK_NULL_HANDLE;
        VkShaderModule fragment_shader_module = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
        VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;

        Vector<VkSampler> samplers;

        // These attachments are automatically bound to pipeline layout.
        Vector<size_t> attachment_indices;
    };

    struct RenderPassData {
        RenderPassData(MemoryResource& memory_resource);

        VkRenderPass render_pass = VK_NULL_HANDLE;

        Vector<GraphicsPipelineData> graphics_pipeline_data;

        uint32_t framebuffer_width = 0;
        uint32_t framebuffer_height = 0;

        // Framebuffers reference image views. Therefore swapchain framebuffers reference swapchain image views, which
        // are different from frame to frame. Therefore for the same render pass different framebuffers may be required
        // for different frames. Swapchain framebuffers are not common though, so it would be a waste of resources to
        // create this many framebuffers for every single render pass. This array contains either `SWAPCHAIN_IMAGE_COUNT`
        // valid framebuffers or just one and the rest of array is filled with `VK_NULL_HANDLE`. The latter case means
        // that only the first framebuffer must be used for every frame.
        VkFramebuffer framebuffers[SWAPCHAIN_IMAGE_COUNT]{};

        // Render passes with the same parallel index are executed without pipeline barriers in between.
        size_t parallel_block_index = 0;

        // Color attachment indices, followed by a depth stencil attachment index.
        Vector<size_t> attachment_indices;
    };

    struct ParallelBlockData {
        // These define a pipeline barrier that is placed between two consecutive parallel blocks.
        VkPipelineStageFlags source_stage_mask;
        VkPipelineStageFlags destination_stage_mask;
        VkAccessFlags source_access_mask;
        VkAccessFlags destination_access_mask;
    };

    struct CommandPoolData {
        CommandPoolData(MemoryResource& memory_resource);

        VkCommandPool command_pool = VK_NULL_HANDLE;

        // Some of these are preallocated, some are created on demand during rendering.
        Vector<VkCommandBuffer> command_buffers;
    };

    void create_lifetime_resources(const FrameGraphDescriptor& descriptor);
    void destroy_lifetime_resources();

    void create_surface(CreateContext& create_context);
    void compute_present_mode(CreateContext& create_context);

    void compute_attachment_descriptors(CreateContext& create_context);
    void compute_attachment_mapping(CreateContext& create_context);
    void compute_attachment_access(CreateContext& create_context);
    void compute_parallel_block_indices(CreateContext& create_context);
    void compute_parallel_blocks(CreateContext& create_context);
    void compute_attachment_ranges(CreateContext& create_context);
    void compute_attachment_layouts(CreateContext& create_context);

    void create_render_passes(CreateContext& create_context);
    void create_render_pass(CreateContext& create_context, size_t render_pass_index);
    void create_graphics_pipeline(CreateContext& create_context, size_t render_pass_index, size_t graphics_pipeline_index);

    void create_command_pools(CreateContext& create_context);
    void create_synchronization(CreateContext& create_context);

    void create_temporary_resources();
    void destroy_temporary_resources();

    bool create_swapchain(RecreateContext& recreate_context);
    void create_swapchain_images(RecreateContext& recreate_context);
    void create_swapchain_image_views(RecreateContext& recreate_context);

    void create_attachment_images(RecreateContext& recreate_context);
    void compute_memory_requirements(RecreateContext& recreate_context);
    void compute_sorted_attachment_indices(RecreateContext& recreate_context);
    void compute_allocation_indices(RecreateContext& recreate_context);
    void compute_allocation_offsets(RecreateContext& recreate_context);
    void allocate_attachment_memory(RecreateContext& recreate_context);
    void bind_attachment_image_memory(RecreateContext& recreate_context);
    void create_attachment_image_views(RecreateContext& recreate_context);

    void create_framebuffers(RecreateContext& recreate_context);

    RenderVulkan* m_render = nullptr;
    Window* m_window = nullptr;
    ThreadPool* m_thread_pool = nullptr;

    VkFormat m_surface_format = VK_FORMAT_B8G8R8A8_UNORM;
    VkColorSpaceKHR m_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkPresentModeKHR m_present_mode = VK_PRESENT_MODE_FIFO_KHR;

    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkImage m_swapchain_images[SWAPCHAIN_IMAGE_COUNT]{};
    VkImageView m_swapchain_image_views[SWAPCHAIN_IMAGE_COUNT]{};

    Vector<AttachmentData> m_attachment_data;
    Vector<AttachmentDescriptor> m_attachment_descriptors;
    Vector<VkDeviceMemory> m_allocations;

    Vector<RenderPassData> m_render_pass_data;
    Vector<ParallelBlockData> m_parallel_block_data;

    Vector<CommandPoolData> m_command_pool_data[SWAPCHAIN_IMAGE_COUNT];

    VkSemaphore m_image_acquired_semaphores[SWAPCHAIN_IMAGE_COUNT]{};
    VkSemaphore m_render_finished_semaphores[SWAPCHAIN_IMAGE_COUNT]{};
    VkFence m_fences[SWAPCHAIN_IMAGE_COUNT]{};

    size_t m_semaphore_index = 0;
    size_t m_frame_index = 0;
};

} // namespace kw
