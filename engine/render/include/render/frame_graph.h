#pragma once

#include "render/render.h"

namespace kw {

class MemoryResourceLinear;
class Render;
class RenderPass;
class ThreadPool;
class Window;

enum class Semantic {
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
    size_t semantic_index;
    TextureFormat format;
    size_t offset;
};

struct BindingDescriptor {
    const AttributeDescriptor* attribute_descriptors;
    size_t attribute_descriptor_count;

    size_t stride;
};

enum class PrimitiveTopology {
    TRIANGLE_LIST,
    TRIANGLE_STRIP,
    LINE_LIST,
    LINE_STRIP,
    POINT_LIST,
};

constexpr size_t PRIMITIVE_TOPOLOGY_COUNT = 5;

enum class FillMode {
    FILL,
    LINE,
    POINT,
};

constexpr size_t FILL_MODE_COUNT = 3;

enum class CullMode {
    BACK,
    FRONT,
    NONE,
};

constexpr size_t CULL_MODE_COUNT = 3;

enum class FrontFace {
    COUNTER_CLOCKWISE,
    CLOCKWISE,
};

constexpr size_t FRONT_FACE_COUNT = 2;

enum class StencilOp {
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

enum class CompareOp {
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

enum class BlendFactor {
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

enum class BlendOp {
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

enum class ShaderVisibility {
    ALL,
    VERTEX,
    FRAGMENT,
};

constexpr size_t SHADER_VISILITY_COUNT = 3;

struct UniformAttachmentDescriptor {
    const char* variable_name;
    const char* attachment_name;
    ShaderVisibility visibility;
    // `count` is defined by attachment's count.
};

struct UniformDescriptor {
    const char* variable_name;
    ShaderVisibility visibility;
    size_t count; // 0 is interpreted as 1.
};

enum class Filter {
    LINEAR,
    NEAREST,
};

constexpr size_t FILTER_COUNT = 2;

enum class AddressMode {
    WRAP,
    MIRROR,
    CLAMP,
    BORDER,
};

constexpr size_t ADDRESS_MODE_COUNT = 4;

enum class BorderColor {
    FLOAT_TRANSPARENT_BLACK,
    INT_TRANSPARENT_BLACK,
    FLOAT_OPAQUE_BLACK,
    INT_OPAQUE_BLACK,
    FLOAT_OPAQUE_WHITE,
    INT_OPAQUE_WHITE,
};

constexpr size_t BORDER_COLOR_COUNT = 6;

struct SamplerDescriptor {
    const char* variable_name;
    ShaderVisibility visibility;

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

struct GraphicsPipelineDescriptor {
    const char* name;

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

    const UniformDescriptor* uniform_buffer_descriptors;
    size_t uniform_buffer_descriptor_count;

    const UniformDescriptor* texture_descriptors;
    size_t texture_descriptor_count;

    const SamplerDescriptor* sampler_descriptors;
    size_t sampler_descriptor_count;

    const char* push_constants_name;
    ShaderVisibility push_constants_visibility;
    size_t push_constants_size;
};

struct RenderPassDescriptor {
    const char* name;

    RenderPass* render_pass;

    const GraphicsPipelineDescriptor* graphics_pipeline_descriptors;
    size_t graphics_pipeline_descriptor_count;

    const char* const* color_attachment_names;
    size_t color_attachment_name_count;

    // If depth write is disabled and stencil write mask is 0, depth stencil attachment
    // is considered to be read-only and therefore can be used in parallel.
    const char* depth_stencil_attachment_name;
};

enum class SizeClass {
    RELATIVE,
    ABSOLUTE,
};

constexpr size_t ATTACHMENT_SIZE_CLASS_COUNT = 2;

enum class LoadOp {
    CLEAR,
    DONT_CARE,
    LOAD,
};

constexpr size_t LOAD_OP_COUNT = 3;

struct AttachmentDescriptor {
    const char* name;

    // Only color and depth stencil formats are allowed.
    TextureFormat format;

    SizeClass size_class;
    float width; // 0 is interpreted as 1.
    float height; // 0 is interpreted as 1.

    size_t count; // 0 is interpreted as 1.

    // Operation that is performed on first write access to the attachment.
    LoadOp load_op;

    // For color formats.
    float clear_color[4];

    // For depth stencil formats.
    float clear_depth;
    uint8_t clear_stencil;
};

struct FrameGraphDescriptor {
    Render* render;
    Window* window;
    ThreadPool* thread_pool;
    MemoryResourceLinear* memory_resource;

    bool is_aliasing_enabled;
    bool is_vsync_enabled;

    // `format` is decided automatically, `load_op` is `DONT_CARE`.
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
    static FrameGraph* create_instance(const FrameGraphDescriptor& descriptor);

    virtual ~FrameGraph() = default;

    virtual void render() = 0;

    /** Must be called when window size changes. */
    virtual void recreate_swapchain() = 0;
};

} // namespace kw
