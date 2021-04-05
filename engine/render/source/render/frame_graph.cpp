#include "render/frame_graph.h"
#include "render/render.h"
#include "render/vulkan/frame_graph_vulkan.h"

#include <core/error.h>

#include <algorithm>

namespace kw {

inline bool check_overlap(ShaderVisibility a, ShaderVisibility b) {
    return a == ShaderVisibility::ALL || b == ShaderVisibility::ALL || a == b;
}

static void validate_push_constants(const FrameGraphDescriptor& descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass = descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline = render_pass.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        graphics_pipeline.sampler_descriptors != nullptr || graphics_pipeline.sampler_descriptor_count == 0,
        "Invalid samplers (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
    );

    if (graphics_pipeline.push_constants_name != nullptr) {
        for (size_t j = 0; j < graphics_pipeline.uniform_attachment_descriptor_count; j++) {
            const UniformAttachmentDescriptor& another_attachment_uniform = graphics_pipeline.uniform_attachment_descriptors[j];

            if (check_overlap(graphics_pipeline.push_constants_visibility, another_attachment_uniform.visibility)) {
                KW_ERROR(
                    strcmp(graphics_pipeline.push_constants_name, another_attachment_uniform.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", graphics_pipeline.push_constants_name, render_pass.name, graphics_pipeline.name
                );
            }
        }

        for (size_t j = 0; j < graphics_pipeline.uniform_buffer_descriptor_count; j++) {
            const UniformDescriptor& another_uniform = graphics_pipeline.uniform_buffer_descriptors[j];

            if (check_overlap(graphics_pipeline.push_constants_visibility, another_uniform.visibility)) {
                KW_ERROR(
                    strcmp(graphics_pipeline.push_constants_name, another_uniform.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", graphics_pipeline.push_constants_name, render_pass.name, graphics_pipeline.name
                );
            }
        }

        for (size_t j = 0; j < graphics_pipeline.texture_descriptor_count; j++) {
            const UniformDescriptor& another_uniform = graphics_pipeline.texture_descriptors[j];

            if (check_overlap(graphics_pipeline.push_constants_visibility, another_uniform.visibility)) {
                KW_ERROR(
                    strcmp(graphics_pipeline.push_constants_name, another_uniform.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", graphics_pipeline.push_constants_name, render_pass.name, graphics_pipeline.name
                );
            }
        }

        for (size_t j = 0; j < graphics_pipeline.sampler_descriptor_count; j++) {
            const SamplerDescriptor& another_sampler = graphics_pipeline.sampler_descriptors[j];

            if (check_overlap(graphics_pipeline.push_constants_visibility, another_sampler.visibility)) {
                KW_ERROR(
                    strcmp(graphics_pipeline.push_constants_name, another_sampler.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", graphics_pipeline.push_constants_name, render_pass.name, graphics_pipeline.name
                );
            }
        }
    }
}

static void validate_sampler_descriptors(const FrameGraphDescriptor& descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass = descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline = render_pass.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        graphics_pipeline.sampler_descriptors != nullptr || graphics_pipeline.sampler_descriptor_count == 0,
        "Invalid samplers (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
    );

    for (size_t i = 0; i < graphics_pipeline.sampler_descriptor_count; i++) {
        const SamplerDescriptor& sampler = graphics_pipeline.sampler_descriptors[i];

        KW_ERROR(
            sampler.variable_name != nullptr,
            "Invalid variable name (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
        );

        for (size_t j = 0; j < graphics_pipeline.uniform_attachment_descriptor_count; j++) {
            const UniformAttachmentDescriptor& another_attachment_uniform = graphics_pipeline.uniform_attachment_descriptors[j];

            if (check_overlap(sampler.visibility, another_attachment_uniform.visibility)) {
                KW_ERROR(
                    strcmp(sampler.variable_name, another_attachment_uniform.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", sampler.variable_name, render_pass.name, graphics_pipeline.name
                );
            }
        }

        for (size_t j = 0; j < graphics_pipeline.uniform_buffer_descriptor_count; j++) {
            const UniformDescriptor& another_uniform = graphics_pipeline.uniform_buffer_descriptors[j];

            if (check_overlap(sampler.visibility, another_uniform.visibility)) {
                KW_ERROR(
                    strcmp(sampler.variable_name, another_uniform.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", sampler.variable_name, render_pass.name, graphics_pipeline.name
                );
            }
        }

        for (size_t j = 0; j < graphics_pipeline.texture_descriptor_count; j++) {
            const UniformDescriptor& another_uniform = graphics_pipeline.texture_descriptors[j];

            if (check_overlap(sampler.visibility, another_uniform.visibility)) {
                KW_ERROR(
                    strcmp(sampler.variable_name, another_uniform.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", sampler.variable_name, render_pass.name, graphics_pipeline.name
                );
            }
        }

        for (size_t j = 0; j < i; j++) {
            const SamplerDescriptor& another_sampler = graphics_pipeline.sampler_descriptors[j];

            if (check_overlap(sampler.visibility, another_sampler.visibility)) {
                KW_ERROR(
                    strcmp(sampler.variable_name, another_sampler.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", sampler.variable_name, render_pass.name, graphics_pipeline.name
                );
            }
        }

        KW_ERROR(
            sampler.max_anisotropy >= 0.f,
            "Invalid max anisotropy (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
        );

        KW_ERROR(
            sampler.min_lod >= 0.f,
            "Invalid min LOD (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
        );

        KW_ERROR(
            sampler.max_lod >= 0.f,
            "Invalid max LOD (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
        );
    }
}

static void validate_texture_descriptors(const FrameGraphDescriptor& descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass = descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline = render_pass.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        graphics_pipeline.texture_descriptors != nullptr || graphics_pipeline.texture_descriptor_count == 0,
        "Invalid textures (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
    );

    for (size_t i = 0; i < graphics_pipeline.texture_descriptor_count; i++) {
        const UniformDescriptor& uniform = graphics_pipeline.texture_descriptors[i];

        KW_ERROR(
            uniform.variable_name != nullptr,
            "Invalid variable name (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
        );

        for (size_t j = 0; j < graphics_pipeline.uniform_attachment_descriptor_count; j++) {
            const UniformAttachmentDescriptor& another_attachment_uniform = graphics_pipeline.uniform_attachment_descriptors[j];

            if (check_overlap(uniform.visibility, another_attachment_uniform.visibility)) {
                KW_ERROR(
                    strcmp(uniform.variable_name, another_attachment_uniform.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", uniform.variable_name, render_pass.name, graphics_pipeline.name
                );
            }
        }

        for (size_t j = 0; j < graphics_pipeline.uniform_buffer_descriptor_count; j++) {
            const UniformDescriptor& another_uniform = graphics_pipeline.uniform_buffer_descriptors[j];

            if (check_overlap(uniform.visibility, another_uniform.visibility)) {
                KW_ERROR(
                    strcmp(uniform.variable_name, another_uniform.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", uniform.variable_name, render_pass.name, graphics_pipeline.name
                );
            }
        }

        for (size_t j = 0; j < i; j++) {
            const UniformDescriptor& another_uniform = graphics_pipeline.texture_descriptors[j];

            if (check_overlap(uniform.visibility, another_uniform.visibility)) {
                KW_ERROR(
                    strcmp(uniform.variable_name, another_uniform.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", uniform.variable_name, render_pass.name, graphics_pipeline.name
                );
            }
        }
    }
}

static void validate_uniform_buffer_descriptors(const FrameGraphDescriptor& descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass = descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline = render_pass.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        graphics_pipeline.uniform_buffer_descriptors != nullptr || graphics_pipeline.uniform_buffer_descriptor_count == 0,
        "Invalid uniform buffers (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
    );

    for (size_t i = 0; i < graphics_pipeline.uniform_buffer_descriptor_count; i++) {
        const UniformDescriptor& uniform = graphics_pipeline.uniform_buffer_descriptors[i];

        KW_ERROR(
            uniform.variable_name != nullptr,
            "Invalid variable name (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
        );

        for (size_t j = 0; j < graphics_pipeline.uniform_attachment_descriptor_count; j++) {
            const UniformAttachmentDescriptor& another_attachment_uniform = graphics_pipeline.uniform_attachment_descriptors[j];

            if (check_overlap(uniform.visibility, another_attachment_uniform.visibility)) {
                KW_ERROR(
                    strcmp(uniform.variable_name, another_attachment_uniform.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", uniform.variable_name, render_pass.name, graphics_pipeline.name
                );
            }
        }

        for (size_t j = 0; j < i; j++) {
            const UniformDescriptor& another_uniform = graphics_pipeline.uniform_buffer_descriptors[j];

            if (check_overlap(uniform.visibility, another_uniform.visibility)) {
                KW_ERROR(
                    strcmp(uniform.variable_name, another_uniform.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", uniform.variable_name, render_pass.name, graphics_pipeline.name
                );
            }
        }
    }
}

static void validate_uniform_attachment_descriptors(const FrameGraphDescriptor& descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass = descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline = render_pass.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        graphics_pipeline.uniform_attachment_descriptors != nullptr || graphics_pipeline.uniform_attachment_descriptor_count == 0,
        "Invalid uniform attachments (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
    );

    for (size_t i = 0; i < graphics_pipeline.uniform_attachment_descriptor_count; i++) {
        const UniformAttachmentDescriptor& attachment_uniform = graphics_pipeline.uniform_attachment_descriptors[i];

        KW_ERROR(
            attachment_uniform.variable_name != nullptr,
            "Invalid variable name (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
        );

        KW_ERROR(
            attachment_uniform.attachment_name != nullptr,
            "Invalid attachment name (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
        );

        for (size_t j = 0; j < i; j++) {
            const UniformAttachmentDescriptor& another_attachment_uniform = graphics_pipeline.uniform_attachment_descriptors[j];

            if (check_overlap(attachment_uniform.visibility, another_attachment_uniform.visibility)) {
                KW_ERROR(
                    strcmp(attachment_uniform.variable_name, another_attachment_uniform.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", attachment_uniform.variable_name, render_pass.name, graphics_pipeline.name
                );
            }
        }

        for (size_t j = 0; j < render_pass.color_attachment_name_count; j++) {
            KW_ERROR(
                strcmp(attachment_uniform.attachment_name, render_pass.color_attachment_names[j]) != 0,
                "Attachment \"%s\" is both written and read (render pass \"%s\", graphics pipeline \"%s\").", attachment_uniform.attachment_name, render_pass.name, graphics_pipeline.name
            );
        }

        if (render_pass.depth_stencil_attachment_name != nullptr) {
            if ((graphics_pipeline.is_depth_test_enabled && graphics_pipeline.is_depth_write_enabled) || (graphics_pipeline.is_stencil_test_enabled && graphics_pipeline.stencil_write_mask != 0)) {
                KW_ERROR(
                    strcmp(attachment_uniform.attachment_name, render_pass.depth_stencil_attachment_name) != 0,
                    "Attachment \"%s\" is both written and read (render pass \"%s\", graphics pipeline \"%s\").", attachment_uniform.attachment_name, render_pass.name, graphics_pipeline.name
                );
            }
        }

        bool attachment_found = strcmp(attachment_uniform.attachment_name, descriptor.swapchain_attachment_name) == 0;

        for (size_t j = 0; j < descriptor.color_attachment_descriptor_count && !attachment_found; j++) {
            const AttachmentDescriptor& attachment = descriptor.color_attachment_descriptors[j];

            if (strcmp(attachment_uniform.attachment_name, attachment.name) == 0) {
                attachment_found = true;
            }
        }

        for (size_t j = 0; j < descriptor.depth_stencil_attachment_descriptor_count && !attachment_found; j++) {
            const AttachmentDescriptor& attachment = descriptor.depth_stencil_attachment_descriptors[j];

            if (strcmp(attachment_uniform.attachment_name, attachment.name) == 0) {
                attachment_found = true;
            }
        }

        KW_ERROR(
            attachment_found,
            "Attachment \"%s\" is not found (render pass \"%s\", graphics pipeline \"%s\").", attachment_uniform.attachment_name, render_pass.name, graphics_pipeline.name
        );
    }
}

static void validate_attachment_blend_descriptors(const FrameGraphDescriptor& descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass = descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline = render_pass.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        graphics_pipeline.attachment_blend_descriptors != nullptr || graphics_pipeline.attachment_blend_descriptor_count == 0,
        "Invalid attachment blends (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
    );

    for (size_t i = 0; i < graphics_pipeline.attachment_blend_descriptor_count; i++) {
        const AttachmentBlendDescriptor& attachment_blend = graphics_pipeline.attachment_blend_descriptors[i];

        KW_ERROR(
            attachment_blend.attachment_name != nullptr,
            "Invalid attachment name (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
        );

        for (size_t j = 0; j < i; j++) {
            const AttachmentBlendDescriptor& another_attachment_blend = graphics_pipeline.attachment_blend_descriptors[j];
            
            KW_ERROR(
                strcmp(attachment_blend.attachment_name, another_attachment_blend.attachment_name) != 0,
                "Attachment \"%s\" is already blend (render pass \"%s\", graphics pipeline \"%s\").", attachment_blend.attachment_name, render_pass.name, graphics_pipeline.name
            );
        }

        bool attachment_found = strcmp(attachment_blend.attachment_name, descriptor.swapchain_attachment_name) == 0;

        for (size_t j = 0; j < render_pass.color_attachment_name_count && !attachment_found; j++) {
            if (strcmp(attachment_blend.attachment_name, render_pass.color_attachment_names[j]) == 0) {
                attachment_found = true;
            }
        }

        KW_ERROR(
            attachment_found,
            "Attachment \"%s\" is not found (render pass \"%s\", graphics pipeline \"%s\").", attachment_blend.attachment_name, render_pass.name, graphics_pipeline.name
        );
    }
}

static void validate_depth_stencil_test(const FrameGraphDescriptor& descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass = descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline = render_pass.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        !graphics_pipeline.is_depth_test_enabled || render_pass.depth_stencil_attachment_name != nullptr,
        "Depth test requires a depth stencil attachment (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
    );

    KW_ERROR(
        !graphics_pipeline.is_stencil_test_enabled || render_pass.depth_stencil_attachment_name != nullptr,
        "Stencil test requires a depth stencil attachment (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
    );

    if (graphics_pipeline.is_stencil_test_enabled) {
        for (size_t i = 0; i < descriptor.depth_stencil_attachment_descriptor_count; i++) {
            const AttachmentDescriptor& attachment_descriptor = descriptor.depth_stencil_attachment_descriptors[i];
            if (strcmp(render_pass.depth_stencil_attachment_name, attachment_descriptor.name) == 0) {
                KW_ERROR(
                    attachment_descriptor.format == TextureFormat::D24_UNORM_S8_UINT || attachment_descriptor.format == TextureFormat::D32_FLOAT_S8X24_UINT,
                    "Stencil test requires a texture format that supports stencil (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
                );
            }
        }
    }
}

static void validate_primitive_topology(const FrameGraphDescriptor& descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass = descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline = render_pass.graphics_pipeline_descriptors[graphics_pipeline_index];

    if (graphics_pipeline.primitive_topology == PrimitiveTopology::LINE_LIST || graphics_pipeline.primitive_topology == PrimitiveTopology::LINE_STRIP) {
        KW_ERROR(
            graphics_pipeline.fill_mode == FillMode::LINE || graphics_pipeline.fill_mode == FillMode::POINT,
            "Line primitive topologies don't support FILL fill mode (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
        );
    } else if (graphics_pipeline.primitive_topology == PrimitiveTopology::POINT_LIST) {
        KW_ERROR(
            graphics_pipeline.fill_mode == FillMode::POINT,
            "Point primitive topology supports only POINT fill mode (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
        );
    }
}

static void validate_instance_binding_descriptors(const FrameGraphDescriptor& descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass = descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline = render_pass.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        graphics_pipeline.instance_binding_descriptors != nullptr || graphics_pipeline.instance_binding_descriptor_count == 0,
        "Invalid instance bindings (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
    );

    for (size_t i = 0; i < graphics_pipeline.instance_binding_descriptor_count; i++) {
        const BindingDescriptor& binding_descriptor = graphics_pipeline.instance_binding_descriptors[i];

        KW_ERROR(
            binding_descriptor.attribute_descriptor_count > 0,
            "Input binding requires at least one attribute (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
        );

        size_t stride = 0;

        for (size_t j = 0; j < binding_descriptor.attribute_descriptor_count; j++) {
            const AttributeDescriptor& attribute_descriptor = binding_descriptor.attribute_descriptors[j];

            for (size_t k = 0; k < graphics_pipeline.vertex_binding_descriptor_count; k++) {
                const BindingDescriptor& another_binding_descriptor = graphics_pipeline.vertex_binding_descriptors[k];

                for (size_t l = 0; l < another_binding_descriptor.attribute_descriptor_count; l++) {
                    const AttributeDescriptor& another_attribute_descriptor = another_binding_descriptor.attribute_descriptors[l];

                    KW_ERROR(
                        attribute_descriptor.semantic != another_attribute_descriptor.semantic || attribute_descriptor.semantic_index != another_attribute_descriptor.semantic_index,
                        "Semantic is already used (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
                    );
                }
            }

            for (size_t k = 0; k < i; k++) {
                const BindingDescriptor& another_binding_descriptor = graphics_pipeline.instance_binding_descriptors[k];

                for (size_t l = 0; l < another_binding_descriptor.attribute_descriptor_count; l++) {
                    const AttributeDescriptor& another_attribute_descriptor = another_binding_descriptor.attribute_descriptors[l];

                    KW_ERROR(
                        attribute_descriptor.semantic != another_attribute_descriptor.semantic || attribute_descriptor.semantic_index != another_attribute_descriptor.semantic_index,
                        "Semantic is already used (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
                    );
                }
            }

            for (size_t k = 0; k < j; k++) {
                const AttributeDescriptor& another_attribute_descriptor = binding_descriptor.attribute_descriptors[k];

                KW_ERROR(
                    attribute_descriptor.semantic != another_attribute_descriptor.semantic || attribute_descriptor.semantic_index != another_attribute_descriptor.semantic_index,
                    "Semantic is already used (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
                );
            }

            KW_ERROR(
                attribute_descriptor.format != TextureFormat::UNKNOWN,
                "Invalid attribute format (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
            );

            KW_ERROR(
                !TextureFormatUtils::is_depth_stencil(attribute_descriptor.format),
                "Invalid attribute format (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
            );

            KW_ERROR(
                !TextureFormatUtils::is_compressed(attribute_descriptor.format),
                "Invalid attribute format (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
            );

            size_t size = TextureFormatUtils::get_pixel_size(attribute_descriptor.format);

            KW_ERROR(
                attribute_descriptor.offset >= stride,
                "Overlapping attribute (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
            );

            stride = attribute_descriptor.offset + size;
        }

        KW_ERROR(
            binding_descriptor.stride >= stride,
            "Overlapping input binding (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
        );
    }
}

static void validate_vertex_binding_descriptors(const FrameGraphDescriptor& descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass = descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline = render_pass.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        graphics_pipeline.vertex_binding_descriptors != nullptr || graphics_pipeline.vertex_binding_descriptor_count == 0,
        "Invalid vertex bindings (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
    );

    for (size_t i = 0; i < graphics_pipeline.vertex_binding_descriptor_count; i++) {
        const BindingDescriptor& binding_descriptor = graphics_pipeline.vertex_binding_descriptors[i];

        KW_ERROR(
            binding_descriptor.attribute_descriptor_count > 0,
            "Input binding requires at least one attribute (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
        );

        size_t stride = 0;

        for (size_t j = 0; j < binding_descriptor.attribute_descriptor_count; j++) {
            const AttributeDescriptor& attribute_descriptor = binding_descriptor.attribute_descriptors[j];

            for (size_t k = 0; k < i; k++) {
                const BindingDescriptor& another_binding_descriptor = graphics_pipeline.vertex_binding_descriptors[k];

                for (size_t l = 0; l < another_binding_descriptor.attribute_descriptor_count; l++) {
                    const AttributeDescriptor& another_attribute_descriptor = another_binding_descriptor.attribute_descriptors[l];

                    KW_ERROR(
                        attribute_descriptor.semantic != another_attribute_descriptor.semantic || attribute_descriptor.semantic_index != another_attribute_descriptor.semantic_index,
                        "Semantic is already used (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
                    );
                }
            }

            for (size_t k = 0; k < j; k++) {
                const AttributeDescriptor& another_attribute_descriptor = binding_descriptor.attribute_descriptors[k];

                KW_ERROR(
                    attribute_descriptor.semantic != another_attribute_descriptor.semantic || attribute_descriptor.semantic_index != another_attribute_descriptor.semantic_index,
                    "Semantic is already used (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
                );
            }

            KW_ERROR(
                attribute_descriptor.format != TextureFormat::UNKNOWN,
                "Invalid attribute format (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
            );

            KW_ERROR(
                !TextureFormatUtils::is_depth_stencil(attribute_descriptor.format),
                "Invalid attribute format (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
            );

            KW_ERROR(
                !TextureFormatUtils::is_compressed(attribute_descriptor.format),
                "Invalid attribute format (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
            );

            size_t size = TextureFormatUtils::get_pixel_size(attribute_descriptor.format);

            KW_ERROR(
                attribute_descriptor.offset >= stride,
                "Overlapping attribute (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
            );

            stride = attribute_descriptor.offset + size;
        }

        KW_ERROR(
            binding_descriptor.stride >= stride,
            "Overlapping input binding (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
        );
    }
}

static void validate_graphics_pipeline_name(const FrameGraphDescriptor& descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass = descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline = render_pass.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        graphics_pipeline.name != nullptr,
        "Invalid graphics pipeline name (render pass \"%s\").", render_pass.name
    );

    for (size_t i = 0; i < render_pass_index; i++) {
        const RenderPassDescriptor& another_render_pass = descriptor.render_pass_descriptors[i];

        for (size_t j = 0; j < another_render_pass.graphics_pipeline_descriptor_count; j++) {
            const GraphicsPipelineDescriptor& another_graphics_pipeline = another_render_pass.graphics_pipeline_descriptors[j];

            KW_ERROR(
                strcmp(graphics_pipeline.name, another_graphics_pipeline.name) != 0,
                "Graphics pipeline name \"%s\" is already used (render pass \"%s\").", graphics_pipeline.name, render_pass.name
            );
        }
    }

    for (size_t i = 0; i < graphics_pipeline_index; i++) {
        const GraphicsPipelineDescriptor& another_graphics_pipeline = render_pass.graphics_pipeline_descriptors[i];

        KW_ERROR(
            strcmp(graphics_pipeline.name, another_graphics_pipeline.name) != 0,
            "Graphics pipeline name \"%s\" is already used (render pass \"%s\").", graphics_pipeline.name, render_pass.name
        );
    }
}

static void validate_graphics_pipeline_descriptors(const FrameGraphDescriptor& descriptor, size_t render_pass_index) {
    const RenderPassDescriptor& render_pass = descriptor.render_pass_descriptors[render_pass_index];

    KW_ERROR(
        render_pass.graphics_pipeline_descriptors != nullptr || render_pass.graphics_pipeline_descriptor_count == 0,
        "Invalid graphics pipelines (render pass \"%s\").", render_pass.name
    );

    for (size_t i = 0; i < render_pass.graphics_pipeline_descriptor_count; i++) {
        const GraphicsPipelineDescriptor& graphics_pipeline = render_pass.graphics_pipeline_descriptors[i];

        validate_graphics_pipeline_name(descriptor, render_pass_index, i);
        validate_vertex_binding_descriptors(descriptor, render_pass_index, i);
        validate_instance_binding_descriptors(descriptor, render_pass_index, i);
        validate_primitive_topology(descriptor, render_pass_index, i);
        validate_depth_stencil_test(descriptor, render_pass_index, i);
        validate_attachment_blend_descriptors(descriptor, render_pass_index, i);
        validate_uniform_attachment_descriptors(descriptor, render_pass_index, i);
        validate_uniform_buffer_descriptors(descriptor, render_pass_index, i);
        validate_texture_descriptors(descriptor, render_pass_index, i);
        validate_sampler_descriptors(descriptor, render_pass_index, i);
        validate_push_constants(descriptor, render_pass_index, i);

        KW_ERROR(
            graphics_pipeline.vertex_shader_filename != nullptr,
            "Invalid vertex shader (render pass \"%s\", graphics pipeline \"%s\").", render_pass.name, graphics_pipeline.name
        );
    }
}

template <typename T>
inline bool check_equal(T a, T b) {
    return a == b || (a == static_cast<T>(0) && b == static_cast<T>(1)) || (a == static_cast<T>(1) && b == static_cast<T>(0));
}

static bool validate_size(bool& is_set, SizeClass& size_class, float& width, float& height, size_t& count, SizeClass attachment_size_class, float attachment_width, float attachment_height, size_t attachment_count) {
    if (!is_set) {
        is_set = true;

        size_class = attachment_size_class;
        width = attachment_width;
        height = attachment_height;
        count = attachment_count;

        return true;
    }

    return attachment_size_class == size_class && check_equal(attachment_width, width) && check_equal(attachment_height, height) && check_equal(attachment_count, count);
}

static void validate_attachments(const FrameGraphDescriptor& descriptor, size_t render_pass_index) {
    const RenderPassDescriptor& render_pass = descriptor.render_pass_descriptors[render_pass_index];

    bool is_set = false;
    SizeClass size_class = SizeClass::RELATIVE;
    float width = 0.f;
    float height = 0.f;
    size_t count = 0;

    KW_ERROR(
        render_pass.color_attachment_names != nullptr || render_pass.color_attachment_name_count == 0,
        "Invalid write color attachments (render pass \"%s\").", render_pass.name
    );

    for (size_t i = 0; i < render_pass.color_attachment_name_count; i++) {
        const char* attachment_name = render_pass.color_attachment_names[i];
        KW_ERROR(
            attachment_name != nullptr,
            "Invalid attachment name (render pass \"%s\").", render_pass.name
        );

        bool create_found = strcmp(attachment_name, descriptor.swapchain_attachment_name) == 0;

        for (size_t j = 0; j < descriptor.color_attachment_descriptor_count && !create_found; j++) {
            const AttachmentDescriptor& attachment = descriptor.color_attachment_descriptors[j];

            if (strcmp(attachment_name, attachment.name) == 0) {
                KW_ERROR(
                    validate_size(is_set, size_class, width, height, count, attachment.size_class, attachment.width, attachment.height, attachment.count),
                    "Attachment \"%s\" size doesn't match (render pass \"%s\").", attachment_name, render_pass.name
                );

                create_found = true;
            }
        }

        KW_ERROR(
            create_found,
            "Attachment \"%s\" is not found (render pass \"%s\").", attachment_name, render_pass.name
        );

        for (size_t j = 0; j < i; j++) {
            KW_ERROR(
                strcmp(attachment_name, render_pass.color_attachment_names[j]) != 0,
                "Attachment \"%s\" is specified twice (render pass \"%s\").", attachment_name, render_pass.name
            );
        }
    }

    KW_ERROR(
        render_pass.color_attachment_name_count > 0 || render_pass.depth_stencil_attachment_name != nullptr,
        "Attachments are not specified (render pass \"%s\").", render_pass.name
    );

    if (render_pass.depth_stencil_attachment_name != nullptr) {
        const char* attachment_name = render_pass.depth_stencil_attachment_name;

        bool create_found = false;

        for (size_t i = 0; i < descriptor.depth_stencil_attachment_descriptor_count && !create_found; i++) {
            const AttachmentDescriptor& attachment = descriptor.depth_stencil_attachment_descriptors[i];

            if (strcmp(attachment_name, attachment.name) == 0) {
                KW_ERROR(
                    validate_size(is_set, size_class, width, height, count, attachment.size_class, attachment.width, attachment.height, attachment.count),
                    "Attachment \"%s\" size doesn't match (render pass \"%s\").", attachment_name, render_pass.name
                );

                create_found = true;
            }
        }

        for (size_t i = 0; i < render_pass.color_attachment_name_count; i++) {
            KW_ERROR(
                strcmp(attachment_name, render_pass.color_attachment_names[i]) != 0,
                "Attachment \"%s\" is specified twice (render pass \"%s\").", attachment_name, render_pass.name
            );
        }

        KW_ERROR(
            create_found,
            "Attachment \"%s\" is not found (render pass \"%s\").", attachment_name, render_pass.name
        );
    }
}

static void validate_render_pass_name(const FrameGraphDescriptor& descriptor, size_t render_pass_index) {
    const RenderPassDescriptor& render_pass = descriptor.render_pass_descriptors[render_pass_index];

    KW_ERROR(render_pass.name != nullptr, "Invalid render pass name.");

    for (size_t i = 0; i < render_pass_index; i++) {
        const RenderPassDescriptor& another_render_pass = descriptor.render_pass_descriptors[i];

        KW_ERROR(strcmp(render_pass.name, another_render_pass.name) != 0, "Render pass name \"%s\" is already used.", render_pass.name);
    }
}

static void validate_render_passes(const FrameGraphDescriptor& descriptor) {
    KW_ERROR(
        descriptor.render_pass_descriptors != nullptr || descriptor.render_pass_descriptor_count == 0,
        "Invalid render passes."
    );

    for (size_t i = 0; i < descriptor.render_pass_descriptor_count; i++) {
        const RenderPassDescriptor& render_pass = descriptor.render_pass_descriptors[i];

        validate_render_pass_name(descriptor, i);

        validate_attachments(descriptor, i);

        validate_graphics_pipeline_descriptors(descriptor, i);

        // TODO: Put `render_pass != nullptr` check back.
        //KW_ERROR(
        //    render_pass.render_pass != nullptr,
        //    "Invalid render pass \"%s\".", render_pass.name
        //);
    }
}

static void validate_depth_stencil_attachments(const FrameGraphDescriptor& descriptor) {
    KW_ERROR(
        descriptor.depth_stencil_attachment_descriptors != nullptr || descriptor.depth_stencil_attachment_descriptor_count == 0,
        "Invalid depth stencil attachments."
    );

    for (size_t i = 0; i < descriptor.depth_stencil_attachment_descriptor_count; i++) {
        const AttachmentDescriptor& attachment = descriptor.depth_stencil_attachment_descriptors[i];

        KW_ERROR(
            attachment.name != nullptr,
            "Invalid depth stencil attachment name."
        );

        KW_ERROR(
            strcmp(attachment.name, descriptor.swapchain_attachment_name) != 0,
            "Attachment name \"%s\" is already used.", attachment.name
        );

        for (size_t j = 0; j < descriptor.color_attachment_descriptor_count; j++) {
            const AttachmentDescriptor& another_attachment = descriptor.color_attachment_descriptors[j];

            KW_ERROR(
                strcmp(attachment.name, another_attachment.name) != 0,
                "Attachment name \"%s\" is already used.", attachment.name
            );
        }

        for (size_t j = 0; j < i; j++) {
            const AttachmentDescriptor& another_attachment = descriptor.depth_stencil_attachment_descriptors[j];

            KW_ERROR(
                strcmp(attachment.name, another_attachment.name) != 0,
                "Attachment name \"%s\" is already used.", attachment.name
            );
        }

        KW_ERROR(
            TextureFormatUtils::is_depth_stencil(attachment.format),
            "Invalid attachment \"%s\" format.", attachment.name
        );

        if (attachment.size_class == SizeClass::RELATIVE) {
            KW_ERROR(
                attachment.width >= 0.f && attachment.width <= 1.f,
                "Invalid attachment \"%s\" width.", attachment.name
            );

            KW_ERROR(
                attachment.height >= 0.f && attachment.height <= 1.f,
                "Invalid attachment \"%s\" height.", attachment.name
            );
        } else {
            KW_ERROR(
                attachment.width > 0.f && static_cast<float>(static_cast<uint32_t>(attachment.width)) == attachment.width,
                "Invalid attachment \"%s\" width.", attachment.name
            );

            KW_ERROR(
                attachment.height > 0.f && static_cast<float>(static_cast<uint32_t>(attachment.height)) == attachment.height,
                "Invalid attachment \"%s\" height.", attachment.name
            );
        }
    }
}

static void validate_color_attachments(const FrameGraphDescriptor& descriptor) {
    KW_ERROR(
        descriptor.color_attachment_descriptors != nullptr || descriptor.color_attachment_descriptor_count == 0,
        "Invalid color attachments."
    );

    for (size_t i = 0; i < descriptor.color_attachment_descriptor_count; i++) {
        const AttachmentDescriptor& attachment = descriptor.color_attachment_descriptors[i];

        KW_ERROR(
            attachment.name != nullptr,
            "Invalid attachment name."
        );

        KW_ERROR(
            strcmp(attachment.name, descriptor.swapchain_attachment_name) != 0,
            "Attachment name \"%s\" is already used.", attachment.name
        );

        for (size_t j = 0; j < i; j++) {
            const AttachmentDescriptor& another_attachment = descriptor.color_attachment_descriptors[j];

            KW_ERROR(
                strcmp(attachment.name, another_attachment.name) != 0,
                "Attachment name \"%s\" is already used.", attachment.name
            );
        }

        KW_ERROR(
            attachment.format != TextureFormat::UNKNOWN,
            "Invalid attachment \"%s\" format.", attachment.name
        );

        KW_ERROR(
            !TextureFormatUtils::is_depth_stencil(attachment.format),
            "Invalid attachment \"%s\" format.", attachment.name
        );

        KW_ERROR(
            !TextureFormatUtils::is_compressed(attachment.format),
            "Invalid attachment \"%s\" format.", attachment.name
        );

        if (attachment.size_class == SizeClass::RELATIVE) {
            KW_ERROR(
                attachment.width >= 0.f && attachment.width <= 1.f,
                "Invalid attachment \"%s\" width.", attachment.name
            );

            KW_ERROR(
                attachment.height >= 0.f && attachment.height <= 1.f,
                "Invalid attachment \"%s\" height.", attachment.name
            );
        } else {
            KW_ERROR(
                attachment.width > 0.f && static_cast<float>(static_cast<uint32_t>(attachment.width)) == attachment.width,
                "Invalid attachment \"%s\" width.", attachment.name
            );

            KW_ERROR(
                attachment.height > 0.f && static_cast<float>(static_cast<uint32_t>(attachment.height)) == attachment.height,
                "Invalid attachment \"%s\" height.", attachment.name
            );
        }
        
        for (float component : attachment.clear_color) {
            KW_ERROR(
                component >= 0.f,
                "Invalid attachment \"%s\" clear color.", attachment.name
            );
        }
    }
}

FrameGraph* FrameGraph::create_instance(const FrameGraphDescriptor& descriptor) {
    KW_ERROR(descriptor.render != nullptr, "Invalid render.");
    KW_ERROR(descriptor.window != nullptr, "Invalid window.");
    KW_ERROR(descriptor.thread_pool != nullptr, "Invalid thread pool.");
    KW_ERROR(descriptor.swapchain_attachment_name != nullptr, "Invalid swapchain name.");

    validate_color_attachments(descriptor);

    validate_depth_stencil_attachments(descriptor);

    validate_render_passes(descriptor);

    switch (descriptor.render->get_api()) {
    case RenderApi::VULKAN:
        return new FrameGraphVulkan(descriptor);
    default:
        KW_ERROR(false, "Chosen render API is not supported on your platform.");
    }
}

} // namespace kw
