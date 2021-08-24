#pragma once

#include "render/render.h"

#include <core/containers/pair.h>

namespace kw {

class GraphicsPipeline;
class RenderPassImpl;
class Window;

struct ScissorsRect {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

struct DrawCallDescriptor {
    // It is highly encouraged to submit subsequent draw calls with the same graphics pipeline.
    GraphicsPipeline* graphics_pipeline;

    const VertexBuffer* const* vertex_buffers;
    size_t vertex_buffer_count; // Must match graphics pipeline.

    const VertexBuffer* const* instance_buffers;
    size_t instance_buffer_count; // Must match graphics pipeline.

    IndexBuffer* index_buffer;

    uint32_t index_count;
    uint32_t instance_count; // 0 is interpreted as 1.
    uint32_t index_offset; // In indices.
    uint32_t vertex_offset; // In vertices.
    uint32_t instance_offset; // In instances.

    // If not overridden, framebuffer size is used.
    bool override_scissors;
    ScissorsRect scissors;

    uint8_t stencil_reference;

    const Texture* const* uniform_textures;
    size_t uniform_texture_count; // Must match graphics pipeline.

    const UniformBuffer* const* uniform_buffers;
    size_t uniform_buffer_count; // Must match graphics pipeline.

    const void* push_constants;
    size_t push_constants_size; // Must match graphics pipeline.
};

class RenderPassContext {
public:
    virtual void draw(const DrawCallDescriptor& descriptor) = 0;

    virtual Render& get_render() const = 0;

    virtual uint32_t get_attachment_width() const = 0;
    virtual uint32_t get_attachment_height() const = 0;

    virtual uint32_t get_context_index() const = 0;
};

class RenderPass {
public:
    virtual ~RenderPass() = default;

    // Must be called between first and second frame graph tasks. May return `nullptr` if window is minimized.
    // Multiple calls per frame are allowed (useful to render shadow maps and reflection probes). Different render pass
    // contexts can be used in parallel on host, but they always execute sequentially on device. The order of execution
    // on device is defined by context index. The previous attachment content is not guaranteed to be preserved.
    RenderPassContext* begin(uint32_t context_index = 0);

    // If source attachment is smaller than destination texture, the remaining host texture area is undefined.
    // If source attachment is larger than destination texture, the source attachment is cropped.
    // Context index specifies after which context to run on device.
    void blit(const char* source_attachment, Texture* destination_texture, uint32_t destination_mip_level = 0,
              uint32_t destination_array_layer = 0, uint32_t context_index = 0);
    
    // If source attachment is smaller than destination host texture, the remaining host texture area is undefined.
    // If source attachment is larger than destination host texture, the source attachment is cropped.
    // Returns index that can be tested in `FrameGraph` on when host texture can be accessed on host.
    // Context index specifies after which context to run on device.
    uint64_t blit(const char* source_attachment, HostTexture* destination_host_texture, uint32_t context_index = 0);

private:
    // API-specific structure set by frame graph.
    RenderPassImpl* m_impl = nullptr;
    friend class FrameGraph;
};

enum class Semantic : uint32_t {
    POSITION,
    COLOR,
    TEXCOORD,
    NORMAL,
    BINORMAL,
    TANGENT,
    JOINTS,
    WEIGHTS,
};

constexpr size_t SEMANTIC_COUNT = 8;

struct AttributeDescriptor {
    Semantic semantic;
    uint32_t semantic_index;
    TextureFormat format;
    uint64_t offset;
};

struct BindingDescriptor {
    const AttributeDescriptor* attribute_descriptors;
    size_t attribute_descriptor_count;

    uint64_t stride;
};

enum class PrimitiveTopology : uint32_t {
    TRIANGLE_LIST,
    TRIANGLE_STRIP,
    LINE_LIST,
    LINE_STRIP,
    POINT_LIST,
};

constexpr size_t PRIMITIVE_TOPOLOGY_COUNT = 5;

enum class FillMode : uint32_t {
    FILL,
    LINE,
    POINT,
};

constexpr size_t FILL_MODE_COUNT = 3;

enum class CullMode : uint32_t {
    BACK,
    FRONT,
    NONE,
};

constexpr size_t CULL_MODE_COUNT = 3;

enum class FrontFace : uint32_t {
    CLOCKWISE,
    COUNTER_CLOCKWISE,
};

constexpr size_t FRONT_FACE_COUNT = 2;

enum class StencilOp : uint32_t {
    KEEP,
    ZERO,
    REPLACE,
    INCREMENT_AND_CLAMP,
    DECREMENT_AND_CLAMP,
    INVERT,
    INCREMENT_AND_WRAP,
    DECREMENT_AND_WRAP,
};

constexpr size_t STENCIL_OP_COUNT = 8;

enum class CompareOp : uint32_t {
    NEVER,
    LESS,
    EQUAL,
    LESS_OR_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_OR_EQUAL,
    ALWAYS,
};

constexpr size_t COMPARE_OP_COUNT = 8;

struct StencilOpState {
    StencilOp fail_op;
    StencilOp pass_op;
    StencilOp depth_fail_op;
    CompareOp compare_op;
};

enum class BlendFactor : uint32_t {
    ZERO,
    ONE,
    SOURCE_COLOR,
    SOURCE_INVERSE_COLOR,
    SOURCE_ALPHA,
    SOURCE_INVERSE_ALPHA,
    DESTINATION_COLOR,
    DESTINATION_INVERSE_COLOR,
    DESTINATION_ALPHA,
    DESTINATION_INVERSE_ALPHA,
};

constexpr size_t BLEND_FACTOR_COUNT = 10;

enum class BlendOp : uint32_t {
    ADD,
    SUBTRACT,
    REVERSE_SUBTRACT,
    MIN,
    MAX,
};

constexpr size_t BLEND_OP_COUNT = 5;

struct AttachmentBlendDescriptor {
    const char* attachment_name;

    BlendFactor source_color_blend_factor;
    BlendFactor destination_color_blend_factor;
    BlendOp color_blend_op;

    BlendFactor source_alpha_blend_factor;
    BlendFactor destination_alpha_blend_factor;
    BlendOp alpha_blend_op;
};

struct UniformAttachmentDescriptor {
    const char* variable_name;
    const char* attachment_name;
};

struct UniformTextureDescriptor {
    const char* variable_name;
    TextureType texture_type;
};

enum class Filter : uint32_t {
    LINEAR,
    NEAREST,
};

constexpr size_t FILTER_COUNT = 2;

enum class AddressMode : uint32_t {
    WRAP,
    MIRROR,
    CLAMP,
    BORDER,
};

constexpr size_t ADDRESS_MODE_COUNT = 4;

enum class BorderColor : uint32_t {
    FLOAT_TRANSPARENT_BLACK,
    INT_TRANSPARENT_BLACK,
    FLOAT_OPAQUE_BLACK,
    INT_OPAQUE_BLACK,
    FLOAT_OPAQUE_WHITE,
    INT_OPAQUE_WHITE,
};

constexpr size_t BORDER_COLOR_COUNT = 6;

struct UniformSamplerDescriptor {
    const char* variable_name;

    Filter min_filter;
    Filter mag_filter;
    Filter mip_filter;

    AddressMode address_mode_u;
    AddressMode address_mode_v;
    AddressMode address_mode_w;

    float mip_lod_bias;

    bool anisotropy_enable;
    float max_anisotropy;

    bool compare_enable;
    CompareOp compare_op;

    float min_lod;
    float max_lod;

    BorderColor border_color;
};

struct UniformBufferDescriptor {
    const char* variable_name;
    uint64_t size;
};

struct GraphicsPipelineDescriptor {
    const char* graphics_pipeline_name;
    const char* render_pass_name;

    const char* vertex_shader_filename;
    const char* fragment_shader_filename;

    const BindingDescriptor* vertex_binding_descriptors;
    size_t vertex_binding_descriptor_count;

    const BindingDescriptor* instance_binding_descriptors;
    size_t instance_binding_descriptor_count;

    PrimitiveTopology primitive_topology;

    FillMode fill_mode;
    CullMode cull_mode;
    FrontFace front_face;

    float depth_bias_constant_factor;
    float depth_bias_clamp;
    float depth_bias_slope_factor;

    bool is_depth_test_enabled;
    bool is_depth_write_enabled;
    CompareOp depth_compare_op;

    bool is_stencil_test_enabled;
    uint8_t stencil_compare_mask;
    uint8_t stencil_write_mask;
    StencilOpState front_stencil_op_state;
    StencilOpState back_stencil_op_state;

    const AttachmentBlendDescriptor* attachment_blend_descriptors;
    size_t attachment_blend_descriptor_count;

    const UniformAttachmentDescriptor* uniform_attachment_descriptors;
    size_t uniform_attachment_descriptor_count;

    const UniformTextureDescriptor* uniform_texture_descriptors;
    size_t uniform_texture_descriptor_count;

    const UniformSamplerDescriptor* uniform_sampler_descriptors;
    size_t uniform_sampler_descriptor_count;

    const UniformBufferDescriptor* uniform_buffer_descriptors;
    size_t uniform_buffer_descriptor_count;

    const char* push_constants_name;
    size_t push_constants_size;
};

// Some uniforms or push constants could be optimized away. It doesn't mean that you should remove those from
// `GraphicsPipelineDescriptor`, because it gracefully handles this situation.
struct ShaderReflection {
    // Does not distinguish different attribute bindings. Offset is undefined.
    const AttributeDescriptor* attribute_descriptors;
    size_t attribute_descriptor_count;

    // Does not distinguish uniform attachment and uniform texture. Visibility is undefined.
    const UniformTextureDescriptor* uniform_texture_descriptors;
    size_t uniform_texture_descriptor_count;

    // Nothing other than name is available for samplers.
    const char* const* uniform_sampler_names;
    size_t uniform_sampler_name_count;

    // Visibility is undefined.
    const UniformBufferDescriptor* uniform_buffer_descriptors;
    size_t uniform_buffer_descriptor_count;

    const char* push_constants_name;
    size_t push_constants_size;
};

struct RenderPassDescriptor {
    const char* name;

    // This render pass instance is initialized in the first frame graph task.
    RenderPass* render_pass;

    // These color and depth stencil attachments may be read by this render pass.
    const char* const* read_attachment_names;
    size_t read_attachment_name_count;

    // These color attachments are written by this render pass.
    const char* const* write_color_attachment_names;
    size_t write_color_attachment_name_count;

    // This depth stencil attachment may be depth-stencil tested by this render pass.
    // Must not be used along with `write_depth_stencil_attachment_name`.
    const char* read_depth_stencil_attachment_name;

    // This depth stencil attachment is written by this render pass.
    // Must not be used along with `read_depth_stencil_attachment_name`.
    const char* write_depth_stencil_attachment_name;
};

enum class SizeClass : uint32_t {
    RELATIVE,
    ABSOLUTE,
};

constexpr size_t ATTACHMENT_SIZE_CLASS_COUNT = 2;

enum class LoadOp : uint32_t {
    CLEAR,
    DONT_CARE,
    LOAD,
};

constexpr size_t LOAD_OP_COUNT = 3;

struct AttachmentDescriptor {
    const char* name;

    // Only color and depth stencil formats are allowed.
    TextureFormat format;

    // Operation that is performed on first write access to the attachment.
    LoadOp load_op;

    SizeClass size_class;
    float width; // 0 is interpreted as 1.
    float height; // 0 is interpreted as 1.

    // For color formats.
    float clear_color[4];

    // For depth stencil formats.
    float clear_depth;
    uint8_t clear_stencil;

    // Whether it is allowed to blit from this attachment to another texture (either device or host).
    bool is_blit_source;
};

struct FrameGraphDescriptor {
    Render* render;

    // Window is allowed to be `nullptr` in which case swapchain is not created, acquire and present are never called.
    Window* window;

    bool is_aliasing_enabled;
    bool is_vsync_enabled;

    // Descriptor set count is pretty much the number of materials per descriptor pool. If most of your materials are
    // 4 textures, 1 sampler and 1 uniform buffer, then good values for these fields are 256, 1024, 256, 256.
    uint32_t descriptor_set_count_per_descriptor_pool;
    uint32_t uniform_texture_count_per_descriptor_pool;
    uint32_t uniform_sampler_count_per_descriptor_pool;
    uint32_t uniform_buffer_count_per_descriptor_pool;

    // `format` is decided automatically (most likely `RGBA8_UNORM`), `load_op` is `DONT_CARE`.
    const char* swapchain_attachment_name;

    const AttachmentDescriptor* color_attachment_descriptors;
    size_t color_attachment_descriptor_count;

    const AttachmentDescriptor* depth_stencil_attachment_descriptors;
    size_t depth_stencil_attachment_descriptor_count;

    // Render passes are executed in order they are specified in this array. However, renderer can execute
    // consecutive render passes in parallel if they don't have any write dependencies.
    const RenderPassDescriptor* render_pass_descriptors;
    size_t render_pass_descriptor_count;
};

class FrameGraph {
public:
    static FrameGraph* create_instance(const FrameGraphDescriptor& frame_graph_descriptor);

    virtual ~FrameGraph() = default;

    // Works for any type of shaders.
    virtual ShaderReflection get_shader_reflection(const char* relative_path) = 0;

    virtual GraphicsPipeline* create_graphics_pipeline(const GraphicsPipelineDescriptor& graphics_pipeline_descriptor) = 0;
    virtual void destroy_graphics_pipeline(GraphicsPipeline* graphics_pipeline) = 0;

    // The first task acquires the swapchain and resets render pass implementations.
    // The second task submits the frame and presents the swapchain.
    virtual Pair<Task*, Task*> create_tasks() = 0;

    // Must be called when window size changes.
    virtual void recreate_swapchain() = 0;

    // Get rendered frame index, must be used along with blit.
    virtual uint64_t get_frame_index() const = 0;

    // Query swapchain size.
    virtual uint32_t get_width() const = 0;
    virtual uint32_t get_height() const = 0;

protected:
    // Called by API implementations, because only base FrameGraph class has access to render pass implementation.
    RenderPassImpl*& get_render_pass_impl(RenderPass* render_pass);
};

} // namespace kw
