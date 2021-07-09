#include "render/frame_graph.h"
#include "render/graphics_pipeline_impl.h"
#include "render/render_pass_impl.h"

#include <core/concurrency/task.h>
#include <core/containers/queue.h>
#include <core/containers/shared_ptr.h>
#include <core/containers/string.h>
#include <core/containers/unique_ptr.h>
#include <core/containers/unordered_map.h>
#include <core/containers/vector.h>
#include <core/utils/enum_utils.h>

#include <vulkan/vulkan.h>

#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <thread>

namespace kw {

class RenderVulkan;
class TimelineSemaphore;

class GraphicsPipelineVulkan : public GraphicsPipeline {
public:
    struct NoOpHash {
        // crc64 is already a decent hash.
        size_t operator()(uint64_t value) const;
    };

    struct DescriptorSetData {
        DescriptorSetData(VkDescriptorSet descriptor_set_, uint64_t last_frame_usage_);
        DescriptorSetData(const DescriptorSetData& other);

        VkDescriptorSet descriptor_set;

        // When descriptor set is not used for a certain number of frames,
        // it is considered to be retired and goes back to free list.
        std::atomic_uint64_t last_frame_usage;
    };

    GraphicsPipelineVulkan(MemoryResource& memory_resource);
    GraphicsPipelineVulkan(GraphicsPipelineVulkan&& other);

    VkShaderModule vertex_shader_module;
    VkShaderModule fragment_shader_module;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;

    // These are needed for draw call validation.
    uint32_t vertex_buffer_count;
    uint32_t instance_buffer_count;

    // Key here is crc64 of all descriptors in a descriptor set.
    UnorderedMap<uint64_t, DescriptorSetData, NoOpHash> bound_descriptor_sets;
    std::shared_mutex bound_descriptor_sets_mutex;

    // Descriptor sets are allocated in batches, the extra ones end up here. When descriptor sets retire,
    // they end up here. When new descriptor set is needed, it's queried from here.
    Vector<VkDescriptorSet> unbound_descriptor_sets;
    std::mutex unbound_descriptor_sets_mutex;

    // Descriptor sets are allocated in geometric sequence: 1, 2, 4, 8, 16 and so on. This way graphics pipelines
    // that use a few descriptor sets don't waste much of a descriptor memory, and render passes that need many
    // descriptor sets don't waste CPU time to allocate them.
    uint32_t descriptor_set_count;

    // This is needed for draw call validation. Actual number of descriptors written to descriptor set may be less
    // because of shader optimizations.
    uint32_t uniform_attachment_descriptor_count;

    // Mapping from `VkWriteDescriptorSet` indices to `m_attachment_data` indices.
    Vector<uint32_t> uniform_attachment_indices;

    // Mapping from `VkWriteDescriptorSet` indices to `DrawCallDescriptor` indices.
    Vector<uint32_t> uniform_attachment_mapping;

    // This is needed for draw call validation. Actual number of descriptors written to descriptor set may be less
    // because of shader optimizations.
    uint32_t uniform_texture_descriptor_count;

    // First texture from `uniform_texture_mapping` goes to this binding.
    uint32_t uniform_texture_first_binding;

    // Mapping from `VkWriteDescriptorSet` indices to `DrawCallDescriptor` indices.
    Vector<uint32_t> uniform_texture_mapping;

    // These are immutable samplers and there's no need to bind them to descriptor set.
    // They are stored to be destroyed after descriptor set layout is destroyed.
    Vector<VkSampler> uniform_samplers;

    // This is needed for draw call validation. Actual number of descriptors written to descriptor set may be less
    // because of shader optimizations.
    uint32_t uniform_buffer_descriptor_count;

    // First buffer from `uniform_buffer_mapping` goes to this binding.
    uint32_t uniform_buffer_first_binding;

    // Mapping from `VkWriteDescriptorSet` indices to `DrawCallDescriptor` indices.
    Vector<uint32_t> uniform_buffer_mapping;

    // Sizes of each uniform buffer in descriptor set.
    Vector<uint32_t> uniform_buffer_sizes;

    // This is needed for draw call validation and for push constants command. When push constants are optimized
    // away from the shader stages, this value is equal to the expected one anyway.
    uint32_t push_constants_size;

    // This is needed for push constants command. Might be different than graphics pipeline descriptor value
    // because of shader optimizations. Value of 0 means push constants were optimized away from all stages and
    // won't be pushed.
    VkShaderStageFlags push_constants_visibility;
};

class FrameGraphVulkan : public FrameGraph {
public:
    explicit FrameGraphVulkan(const FrameGraphDescriptor& descriptor);
    ~FrameGraphVulkan() override;

    ShaderReflection get_shader_reflection(const char* relative_path) override;

    GraphicsPipeline* create_graphics_pipeline(const GraphicsPipelineDescriptor& graphics_pipeline_descriptor) override;
    void destroy_graphics_pipeline(GraphicsPipeline* graphics_pipeline) override;

    std::pair<Task*, Task*> create_tasks() override;
    
    void recreate_swapchain() override;

    uint64_t get_frame_index() const override;

    uint32_t get_width() const override;
    uint32_t get_height() const override;

private:
    static constexpr size_t SWAPCHAIN_IMAGE_COUNT = 3;
    
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

    KW_DECLARE_ENUM_BITMASK(AttachmentAccess);

    struct AttachmentBoundsData {
        // `UINT32_MAX` if there's no read/write render pass for this attachment.
        uint32_t min_read_render_pass_index;
        uint32_t max_read_render_pass_index;
        uint32_t min_write_render_pass_index;
        uint32_t max_write_render_pass_index;
    };

    struct AttachmentBarrierData {
        VkImageLayout source_image_layout;
        VkAccessFlags source_access_mask;
        VkPipelineStageFlags source_pipeline_stage_mask;
        VkImageLayout destination_image_layout;
        VkAccessFlags destination_access_mask;
        VkPipelineStageFlags destination_pipeline_stage_mask;
    };

    struct CreateContext {
        const FrameGraphDescriptor& frame_graph_descriptor;

        // Mapping from attachment names to attachment indices.
        UnorderedMap<StringView, uint32_t> attachment_mapping;

        // Min/max read/write render pass indices for each attachment.
        Vector<AttachmentBoundsData> attachment_bounds_data;

        // Attachment_count x render_pass_count matrix of access masks and image layouts for pipeline barriers,
        // render passes and blit requests.
        Vector<AttachmentBarrierData> attachment_barrier_matrix;
    };

    struct AttachmentData {
        AttachmentData(MemoryResource& memory_resource);

        VkImage image;
        VkImageView image_view;

        // Sampled depth stencil attachments must have either depth or stencil aspect, not both.
        // Attachment must have both though. For color attachment and depth attachments these views alias each other.
        VkImageView sampled_view;

        // Occasionally min index may be larger than max index. This means that the attachment is created at the end
        // of the frame and used at the beginning of the next frame and therefore must be aliased in between.
        uint32_t min_parallel_block_index;
        uint32_t max_parallel_block_index;

        // Defines whether attachment is used as color attachment, depth stencil attachment or sampled.
        VkImageUsageFlags usage_mask;

        // Layout transition from `VK_IMAGE_LAYOUT_UNDEFINED` to specified image layout is performed manually
        // before the first render pass once after the attachment is created.
        VkImageLayout initial_layout;

        // Empty if attachment is not blit-allowed.
        Vector<AttachmentBarrierData> blit_data;
    };

    struct AllocationData {
        // Render manages all device allocations.
        // These are indices and offsets in its internal allocators.
        uint64_t data_index : 16;
        uint64_t data_offset : 48;
    };
    
    struct DescriptorPoolData {
        VkDescriptorPool descriptor_pool;

        // When descriptor set needs more descriptor bindings than
        // descriptor pool has left, new descriptor pool is allocated.
        uint32_t descriptor_sets_left;
        uint32_t textures_left;
        uint32_t samplers_left;
        uint32_t uniform_buffers_left;
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
        CommandPoolData(CommandPoolData&& other);

        VkCommandPool command_pool;

        // Some of these are preallocated, some are created on demand during rendering.
        Vector<VkCommandBuffer> command_buffers;

        // Index of available command buffer.
        size_t current_command_buffer;
    };

    class RenderPassContextVulkan : public RenderPassContext {
    public:
        // Called from a thread that constructs render passes. May be different than the one that submits draw calls.
        // Therefore command buffer at this point is undefined.
        RenderPassContextVulkan(FrameGraphVulkan& frame_graph, uint32_t render_pass_index, uint32_t framebuffer_index);

        void draw(const DrawCallDescriptor& descriptor) override;

        Render& get_render() const override;
        uint32_t get_attachment_width() const override;
        uint32_t get_attachment_height() const override;
        uint32_t get_attachemnt_index() const override;

        // Reset command buffer, transfer semaphore value, framebuffer size and etc.
        void reset();

        // Command buffer is set from render pass implementation. Queried by `PresentTask`.
        VkCommandBuffer command_buffer;
        
        // Used for synchronization of transfer and graphics queues.
        uint64_t transfer_semaphore_value;

    private:
        bool allocate_descriptor_sets();

        FrameGraphVulkan& m_frame_graph;
        uint32_t m_render_pass_index;
        uint32_t m_framebuffer_index;

        uint32_t m_framebuffer_width;
        uint32_t m_framebuffer_height;

        GraphicsPipelineVulkan* m_graphics_pipeline_vulkan;
    };

    class RenderPassImplVulkan : public RenderPassImpl {
    public:
        RenderPassImplVulkan(FrameGraphVulkan& frame_graph, uint32_t render_pass_index);

        RenderPassContextVulkan* begin(uint32_t framebuffer_index) override;

        uint64_t blit(const char* source_attachment, HostTexture* destination_host_texture) override;

        // Queried by `PresentTask`.
        Vector<RenderPassContextVulkan> contexts;

        // Stores blit commands. Most likely `VK_NULL_HANDLE`.
        VkCommandBuffer blit_command_buffer;

    private:
        CommandPoolData& acquire_command_pool();
        VkCommandBuffer acquire_command_buffer();

        FrameGraphVulkan& m_frame_graph;
        uint32_t m_render_pass_index;
    };

    struct RenderPassData {
        RenderPassData(MemoryResource& memory_resource);
        RenderPassData(RenderPassData&& other);

        // Needed to match graphics pipelines with render passes.
        String name;

        VkRenderPass render_pass;

        // These are used to begin render pass and to set scissor.
        uint32_t framebuffer_width;
        uint32_t framebuffer_height;

        // A render pass has multiple framebuffers if its attachments have `count` greater than 1.
        // A render pass has `SWAPCHAIN_IMAGE_COUNT` framebuffers if one of its attachments is a swapchain attachment.
        Vector<VkFramebuffer> framebuffers;

        // Render passes with the same parallel index are executed without pipeline barriers in between.
        uint32_t parallel_block_index;

        // Attachment indices that are allowed to be read from shaders.
        Vector<uint32_t> read_attachment_indices;

        // Color attachment indices, followed by a depth stencil attachment index.
        Vector<uint32_t> write_attachment_indices;

        // Passed to an actual render pass via `FrameGraph`.
        UniquePtr<RenderPassImplVulkan> render_pass_impl;
    };

    struct GraphicsPipelineDestroyCommand {
        GraphicsPipelineVulkan* graphics_pipeline;
        uint64_t semahore_value;
    };

    class AcquireTask : public Task {
    public:
        AcquireTask(FrameGraphVulkan& frame_graph);

        void run() override;

    private:
        FrameGraphVulkan& m_frame_graph;
    };

    class PresentTask : public Task {
    public:
        PresentTask(FrameGraphVulkan& frame_graph);

        void run() override;

    private:
        FrameGraphVulkan& m_frame_graph;
    };
    
    void create_lifetime_resources(const FrameGraphDescriptor& descriptor);
    void destroy_lifetime_resources();

    void create_surface(CreateContext& create_context);
    void compute_present_mode(CreateContext& create_context);

    void compute_attachment_descriptors(CreateContext& create_context);
    void compute_attachment_mapping(CreateContext& create_context);
    void compute_attachment_access(CreateContext& create_context);
    void compute_attachment_barrier_data(CreateContext& create_context);
    void compute_attachment_bounds_data(CreateContext& create_context);
    void compute_parallel_block_indices(CreateContext& create_context);
    void compute_parallel_blocks(CreateContext& create_context);
    void compute_attachment_ranges(CreateContext& create_context);
    void compute_attachment_usage_mask(CreateContext& create_context);
    void compute_attachment_layouts(CreateContext& create_context);
    void compute_attachment_blit_data(CreateContext& create_context);

    void create_render_passes(CreateContext& create_context);
    void create_render_pass(CreateContext& create_context, uint32_t render_pass_index);

    void create_synchronization(CreateContext& create_context);

    void create_temporary_resources();
    void destroy_temporary_resources();

    bool create_swapchain();
    void create_swapchain_images();
    void create_swapchain_image_views();

    void create_attachment_images();
    void allocate_attachment_memory();
    void create_attachment_image_views();

    void create_framebuffers();

    void destroy_dynamic_resources();

    RenderVulkan& m_render;
    Window& m_window;

    uint32_t m_descriptor_set_count_per_descriptor_pool;
    uint32_t m_uniform_texture_count_per_descriptor_pool;
    uint32_t m_uniform_sampler_count_per_descriptor_pool;
    uint32_t m_uniform_buffer_count_per_descriptor_pool;

    VkFormat m_surface_format;
    VkColorSpaceKHR m_color_space;
    VkPresentModeKHR m_present_mode;

    uint32_t m_swapchain_width;
    uint32_t m_swapchain_height;

    VkSurfaceKHR m_surface;
    VkSwapchainKHR m_swapchain;
    VkImage m_swapchain_images[SWAPCHAIN_IMAGE_COUNT];
    VkImageView m_swapchain_image_views[SWAPCHAIN_IMAGE_COUNT];

    // Flattened attachment descriptors from frame graph descriptor.
    Vector<AttachmentDescriptor> m_attachment_descriptors;

    // Attachment_count x render_pass_count matrix of access to a certain attachment on a certain render pass.
    Vector<AttachmentAccess> m_attachment_access_matrix;
    std::mutex m_attachment_access_matrix_mutex;

    Vector<AttachmentData> m_attachment_data;
    Vector<AllocationData> m_allocation_data;
    Vector<RenderPassData> m_render_pass_data;

    // The number of parallel blocks can (and is encouraged to) be less than the number of render passes.
    Vector<ParallelBlockData> m_parallel_block_data;
    std::mutex m_parallel_block_data_mutex;

    // Each frame in flight and each thread requires own set of command pools.
    UnorderedMap<std::thread::id, CommandPoolData> m_command_pool_data[SWAPCHAIN_IMAGE_COUNT];
    std::shared_mutex m_command_pool_mutex;

    // Descriptor pools are mostly traversed and rarely changed, so prefer shared access.
    Vector<DescriptorPoolData> m_descriptor_pools;
    std::mutex m_descriptor_pools_mutex;

    // Queued graphics pipeline destroy commands waiting for semaphore to signal.
    Queue<GraphicsPipelineDestroyCommand> m_graphics_pipeline_destroy_commands;
    std::mutex m_graphics_pipeline_destroy_command_mutex;

    // `vkAcquireNextImageKHR` and `vkQueuePresentKHR` don't support timeline semaphores,
    // so we're forced to deal with a bunch of binary semaphores.
    VkSemaphore m_image_acquired_binary_semaphores[SWAPCHAIN_IMAGE_COUNT];
    VkSemaphore m_render_finished_binary_semaphores[SWAPCHAIN_IMAGE_COUNT];

    // Fences for command buffer access synchronization.
    VkFence m_fences[SWAPCHAIN_IMAGE_COUNT];

    // This semaphore is signaled when frame rendering is finished along with `m_render_finished_binary_semaphores`.
    SharedPtr<TimelineSemaphore> m_render_finished_timeline_semaphore;

    // Used to calculate semaphore index and to detect descriptor set retirement.
    uint64_t m_frame_index;

    // When swapchain and all attachments are recreated, they are in undefined layout
    // and need to be manually transitioned to a layout expected by render passes.
    bool m_is_attachment_layout_set;

    // Current command pool/semaphore/fence to use.
    uint64_t m_semaphore_index;

    // Current swapchain image to use.
    uint32_t m_swapchain_image_index;
};

KW_DEFINE_ENUM_BITMASK(FrameGraphVulkan::AttachmentAccess);

} // namespace kw
