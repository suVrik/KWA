#include "render/frame_graph.h"
#include "render/render.h"
#include "render/render_pass_impl.h"
#include "render/vulkan/frame_graph_vulkan.h"

#include <core/debug/assert.h>
#include <core/error.h>

#include <algorithm>

namespace kw {

RenderPassContext* RenderPass::begin(uint32_t attachment_index) {
    KW_ASSERT(m_impl != nullptr, "Frame graph was not initialized yet.");

    return m_impl->begin(attachment_index);
}

uint64_t RenderPass::blit(const char* source_attachment, HostTexture* destination_host_texture) {
    KW_ASSERT(m_impl != nullptr, "Frame graph was not initialized yet.");

    return m_impl->blit(source_attachment, destination_host_texture);
}

inline bool check_overlap(ShaderVisibility a, ShaderVisibility b) {
    return a == ShaderVisibility::ALL || b == ShaderVisibility::ALL || a == b;
}

static void validate_push_constants(const FrameGraphDescriptor& frame_graph_descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline_descriptor = render_pass_descriptor.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        graphics_pipeline_descriptor.uniform_sampler_descriptors != nullptr || graphics_pipeline_descriptor.uniform_sampler_descriptor_count == 0,
        "Invalid samplers (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
    );

    if (graphics_pipeline_descriptor.push_constants_name != nullptr) {
        for (size_t j = 0; j < graphics_pipeline_descriptor.uniform_attachment_descriptor_count; j++) {
            const UniformAttachmentDescriptor& another_uniform_attachment_descriptor = graphics_pipeline_descriptor.uniform_attachment_descriptors[j];

            if (check_overlap(graphics_pipeline_descriptor.push_constants_visibility, another_uniform_attachment_descriptor.visibility)) {
                KW_ERROR(
                    strcmp(graphics_pipeline_descriptor.push_constants_name, another_uniform_attachment_descriptor.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", graphics_pipeline_descriptor.push_constants_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );
            }
        }

        for (size_t j = 0; j < graphics_pipeline_descriptor.uniform_texture_descriptor_count; j++) {
            const UniformTextureDescriptor& another_uniform_texture_descriptor = graphics_pipeline_descriptor.uniform_texture_descriptors[j];

            if (check_overlap(graphics_pipeline_descriptor.push_constants_visibility, another_uniform_texture_descriptor.visibility)) {
                KW_ERROR(
                    strcmp(graphics_pipeline_descriptor.push_constants_name, another_uniform_texture_descriptor.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", graphics_pipeline_descriptor.push_constants_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );
            }
        }

        for (size_t j = 0; j < graphics_pipeline_descriptor.uniform_sampler_descriptor_count; j++) {
            const UniformSamplerDescriptor& another_uniform_sampler_descriptor = graphics_pipeline_descriptor.uniform_sampler_descriptors[j];

            if (check_overlap(graphics_pipeline_descriptor.push_constants_visibility, another_uniform_sampler_descriptor.visibility)) {
                KW_ERROR(
                    strcmp(graphics_pipeline_descriptor.push_constants_name, another_uniform_sampler_descriptor.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", graphics_pipeline_descriptor.push_constants_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );
            }
        }

        for (size_t j = 0; j < graphics_pipeline_descriptor.uniform_buffer_descriptor_count; j++) {
            const UniformBufferDescriptor& another_uniform_buffer_descriptor = graphics_pipeline_descriptor.uniform_buffer_descriptors[j];

            if (check_overlap(graphics_pipeline_descriptor.push_constants_visibility, another_uniform_buffer_descriptor.visibility)) {
                KW_ERROR(
                    strcmp(graphics_pipeline_descriptor.push_constants_name, another_uniform_buffer_descriptor.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", graphics_pipeline_descriptor.push_constants_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );
            }
        }

        KW_ERROR(
            graphics_pipeline_descriptor.push_constants_size <= 128,
            "Push constants size must be not greater than 128 bytes to satisfy Vulkan requirements (render pass \"%s\", graphics pipeline \"%s\")."
        );
    }
}

static void validate_uniform_buffer_descriptors(const FrameGraphDescriptor& frame_graph_descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline_descriptor = render_pass_descriptor.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        graphics_pipeline_descriptor.uniform_buffer_descriptors != nullptr || graphics_pipeline_descriptor.uniform_buffer_descriptor_count == 0,
        "Invalid uniform buffers (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
    );

    for (size_t i = 0; i < graphics_pipeline_descriptor.uniform_buffer_descriptor_count; i++) {
        const UniformBufferDescriptor& uniform_buffer_descriptor = graphics_pipeline_descriptor.uniform_buffer_descriptors[i];

        KW_ERROR(
            uniform_buffer_descriptor.variable_name != nullptr,
            "Invalid variable name (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
        );

        for (size_t j = 0; j < graphics_pipeline_descriptor.uniform_attachment_descriptor_count; j++) {
            const UniformAttachmentDescriptor& another_uniform_attachment_descriptor = graphics_pipeline_descriptor.uniform_attachment_descriptors[j];

            if (check_overlap(uniform_buffer_descriptor.visibility, another_uniform_attachment_descriptor.visibility)) {
                KW_ERROR(
                    strcmp(uniform_buffer_descriptor.variable_name, another_uniform_attachment_descriptor.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", uniform_buffer_descriptor.variable_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );
            }
        }

        for (size_t j = 0; j < graphics_pipeline_descriptor.uniform_texture_descriptor_count; j++) {
            const UniformTextureDescriptor& another_uniform_texture_descriptor = graphics_pipeline_descriptor.uniform_texture_descriptors[j];

            if (check_overlap(uniform_buffer_descriptor.visibility, another_uniform_texture_descriptor.visibility)) {
                KW_ERROR(
                    strcmp(uniform_buffer_descriptor.variable_name, another_uniform_texture_descriptor.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", uniform_buffer_descriptor.variable_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );
            }
        }

        for (size_t j = 0; j < graphics_pipeline_descriptor.uniform_sampler_descriptor_count; j++) {
            const UniformSamplerDescriptor& another_uniform_sampler_descriptor = graphics_pipeline_descriptor.uniform_sampler_descriptors[j];

            if (check_overlap(uniform_buffer_descriptor.visibility, another_uniform_sampler_descriptor.visibility)) {
                KW_ERROR(
                    strcmp(uniform_buffer_descriptor.variable_name, another_uniform_sampler_descriptor.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", uniform_buffer_descriptor.variable_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );
            }
        }

        for (size_t j = 0; j < i; j++) {
            const UniformBufferDescriptor& another_uniform_buffer_descriptor = graphics_pipeline_descriptor.uniform_buffer_descriptors[j];

            if (check_overlap(uniform_buffer_descriptor.visibility, another_uniform_buffer_descriptor.visibility)) {
                KW_ERROR(
                    strcmp(uniform_buffer_descriptor.variable_name, another_uniform_buffer_descriptor.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", uniform_buffer_descriptor.variable_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );
            }
        }

        KW_ERROR(
            uniform_buffer_descriptor.size > 0,
            "Uniform buffer \"%s\" must not be empty (render pass \"%s\", graphics pipeline \"%s\").", uniform_buffer_descriptor.variable_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
        );
    }
}

static void validate_sampler_descriptors(const FrameGraphDescriptor& frame_graph_descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline_descriptor = render_pass_descriptor.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        graphics_pipeline_descriptor.uniform_sampler_descriptors != nullptr || graphics_pipeline_descriptor.uniform_sampler_descriptor_count == 0,
        "Invalid samplers (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
    );

    for (size_t i = 0; i < graphics_pipeline_descriptor.uniform_sampler_descriptor_count; i++) {
        const UniformSamplerDescriptor& uniform_sampler_descriptor = graphics_pipeline_descriptor.uniform_sampler_descriptors[i];

        KW_ERROR(
            uniform_sampler_descriptor.variable_name != nullptr,
            "Invalid variable name (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
        );

        for (size_t j = 0; j < graphics_pipeline_descriptor.uniform_attachment_descriptor_count; j++) {
            const UniformAttachmentDescriptor& another_uniform_attachment_descriptor = graphics_pipeline_descriptor.uniform_attachment_descriptors[j];

            if (check_overlap(uniform_sampler_descriptor.visibility, another_uniform_attachment_descriptor.visibility)) {
                KW_ERROR(
                    strcmp(uniform_sampler_descriptor.variable_name, another_uniform_attachment_descriptor.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", uniform_sampler_descriptor.variable_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );
            }
        }

        for (size_t j = 0; j < graphics_pipeline_descriptor.uniform_texture_descriptor_count; j++) {
            const UniformTextureDescriptor& another_uniform_texture_descriptor = graphics_pipeline_descriptor.uniform_texture_descriptors[j];

            if (check_overlap(uniform_sampler_descriptor.visibility, another_uniform_texture_descriptor.visibility)) {
                KW_ERROR(
                    strcmp(uniform_sampler_descriptor.variable_name, another_uniform_texture_descriptor.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", uniform_sampler_descriptor.variable_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );
            }
        }

        for (size_t j = 0; j < i; j++) {
            const UniformSamplerDescriptor& another_uniform_sampler_descriptor = graphics_pipeline_descriptor.uniform_sampler_descriptors[j];

            if (check_overlap(uniform_sampler_descriptor.visibility, another_uniform_sampler_descriptor.visibility)) {
                KW_ERROR(
                    strcmp(uniform_sampler_descriptor.variable_name, another_uniform_sampler_descriptor.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", uniform_sampler_descriptor.variable_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );
            }
        }

        KW_ERROR(
            uniform_sampler_descriptor.max_anisotropy >= 0.f,
            "Invalid max anisotropy (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
        );

        KW_ERROR(
            uniform_sampler_descriptor.min_lod >= 0.f,
            "Invalid min LOD (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
        );

        KW_ERROR(
            uniform_sampler_descriptor.max_lod >= 0.f,
            "Invalid max LOD (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
        );
    }
}

static void validate_texture_descriptors(const FrameGraphDescriptor& frame_graph_descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline_descriptor = render_pass_descriptor.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        graphics_pipeline_descriptor.uniform_texture_descriptors != nullptr || graphics_pipeline_descriptor.uniform_texture_descriptor_count == 0,
        "Invalid textures (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
    );

    for (size_t i = 0; i < graphics_pipeline_descriptor.uniform_texture_descriptor_count; i++) {
        const UniformTextureDescriptor& uniform_texture_descriptor = graphics_pipeline_descriptor.uniform_texture_descriptors[i];

        KW_ERROR(
            uniform_texture_descriptor.variable_name != nullptr,
            "Invalid variable name (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
        );

        for (size_t j = 0; j < graphics_pipeline_descriptor.uniform_attachment_descriptor_count; j++) {
            const UniformAttachmentDescriptor& another_uniform_attachment_descriptor = graphics_pipeline_descriptor.uniform_attachment_descriptors[j];

            if (check_overlap(uniform_texture_descriptor.visibility, another_uniform_attachment_descriptor.visibility)) {
                KW_ERROR(
                    strcmp(uniform_texture_descriptor.variable_name, another_uniform_attachment_descriptor.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", uniform_texture_descriptor.variable_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );
            }
        }

        for (size_t j = 0; j < i; j++) {
            const UniformTextureDescriptor& another_uniform_texture_descriptor = graphics_pipeline_descriptor.uniform_texture_descriptors[j];

            if (check_overlap(uniform_texture_descriptor.visibility, another_uniform_texture_descriptor.visibility)) {
                KW_ERROR(
                    strcmp(uniform_texture_descriptor.variable_name, another_uniform_texture_descriptor.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", uniform_texture_descriptor.variable_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );
            }
        }
    }
}

static void validate_uniform_attachment_descriptors(const FrameGraphDescriptor& frame_graph_descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline_descriptor = render_pass_descriptor.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        graphics_pipeline_descriptor.uniform_attachment_descriptors != nullptr || graphics_pipeline_descriptor.uniform_attachment_descriptor_count == 0,
        "Invalid uniform attachments (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
    );

    for (size_t i = 0; i < graphics_pipeline_descriptor.uniform_attachment_descriptor_count; i++) {
        const UniformAttachmentDescriptor& attachment_uniform_descriptor = graphics_pipeline_descriptor.uniform_attachment_descriptors[i];

        KW_ERROR(
            attachment_uniform_descriptor.variable_name != nullptr,
            "Invalid variable name (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
        );

        KW_ERROR(
            attachment_uniform_descriptor.attachment_name != nullptr,
            "Invalid attachment name (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
        );

        for (size_t j = 0; j < i; j++) {
            const UniformAttachmentDescriptor& another_attachment_uniform_descriptor = graphics_pipeline_descriptor.uniform_attachment_descriptors[j];

            if (check_overlap(attachment_uniform_descriptor.visibility, another_attachment_uniform_descriptor.visibility)) {
                KW_ERROR(
                    strcmp(attachment_uniform_descriptor.variable_name, another_attachment_uniform_descriptor.variable_name) != 0,
                    "Variable \"%s\" is already defined (render pass \"%s\", graphics pipeline \"%s\").", attachment_uniform_descriptor.variable_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );
            }
        }

        for (size_t j = 0; j < render_pass_descriptor.color_attachment_name_count; j++) {
            KW_ERROR(
                strcmp(attachment_uniform_descriptor.attachment_name, render_pass_descriptor.color_attachment_names[j]) != 0,
                "Attachment \"%s\" is both written and read (render pass \"%s\", graphics pipeline \"%s\").", attachment_uniform_descriptor.attachment_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
            );
        }

        if (render_pass_descriptor.depth_stencil_attachment_name != nullptr) {
            if ((graphics_pipeline_descriptor.is_depth_test_enabled && graphics_pipeline_descriptor.is_depth_write_enabled) || (graphics_pipeline_descriptor.is_stencil_test_enabled && graphics_pipeline_descriptor.stencil_write_mask != 0)) {
                KW_ERROR(
                    strcmp(attachment_uniform_descriptor.attachment_name, render_pass_descriptor.depth_stencil_attachment_name) != 0,
                    "Attachment \"%s\" is both written and read (render pass \"%s\", graphics pipeline \"%s\").", attachment_uniform_descriptor.attachment_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );
            }
        }

        bool attachment_found = false;

        if (strcmp(attachment_uniform_descriptor.attachment_name, frame_graph_descriptor.swapchain_attachment_name) == 0) {
            KW_ERROR(
                attachment_uniform_descriptor.count <= 1,
                "Attachment \"%s\" count mismatch (render pass \"%s\", graphics pipeline \"%s\").", attachment_uniform_descriptor.attachment_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
            );

            attachment_found = true;
        }

        for (size_t j = 0; j < frame_graph_descriptor.color_attachment_descriptor_count && !attachment_found; j++) {
            const AttachmentDescriptor& attachment_descriptor = frame_graph_descriptor.color_attachment_descriptors[j];

            if (strcmp(attachment_uniform_descriptor.attachment_name, attachment_descriptor.name) == 0) {
                KW_ERROR(
                    attachment_uniform_descriptor.count <= std::max(attachment_descriptor.count, 1U),
                    "Attachment \"%s\" count mismatch (render pass \"%s\", graphics pipeline \"%s\").", attachment_uniform_descriptor.attachment_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );

                attachment_found = true;
            }
        }

        for (size_t j = 0; j < frame_graph_descriptor.depth_stencil_attachment_descriptor_count && !attachment_found; j++) {
            const AttachmentDescriptor& attachment_descriptor = frame_graph_descriptor.depth_stencil_attachment_descriptors[j];

            if (strcmp(attachment_uniform_descriptor.attachment_name, attachment_descriptor.name) == 0) {
                KW_ERROR(
                    attachment_uniform_descriptor.count <= std::max(attachment_descriptor.count, 1U),
                    "Attachment \"%s\" count mismatch (render pass \"%s\", graphics pipeline \"%s\").", attachment_uniform_descriptor.attachment_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );

                attachment_found = true;
            }
        }

        KW_ERROR(
            attachment_found,
            "Attachment \"%s\" is not found (render pass \"%s\", graphics pipeline \"%s\").", attachment_uniform_descriptor.attachment_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
        );
    }
}

static void validate_attachment_blend_descriptors(const FrameGraphDescriptor& frame_graph_descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline_descriptor = render_pass_descriptor.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        graphics_pipeline_descriptor.attachment_blend_descriptors != nullptr || graphics_pipeline_descriptor.attachment_blend_descriptor_count == 0,
        "Invalid attachment blends (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
    );

    for (size_t i = 0; i < graphics_pipeline_descriptor.attachment_blend_descriptor_count; i++) {
        const AttachmentBlendDescriptor& attachment_blend_descriptor = graphics_pipeline_descriptor.attachment_blend_descriptors[i];

        KW_ERROR(
            attachment_blend_descriptor.attachment_name != nullptr,
            "Invalid attachment name (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
        );

        for (size_t j = 0; j < i; j++) {
            const AttachmentBlendDescriptor& another_attachment_blend_descriptor = graphics_pipeline_descriptor.attachment_blend_descriptors[j];
            
            KW_ERROR(
                strcmp(attachment_blend_descriptor.attachment_name, another_attachment_blend_descriptor.attachment_name) != 0,
                "Attachment \"%s\" is already blend (render pass \"%s\", graphics pipeline \"%s\").", attachment_blend_descriptor.attachment_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
            );
        }

        bool attachment_found = strcmp(attachment_blend_descriptor.attachment_name, frame_graph_descriptor.swapchain_attachment_name) == 0;

        for (size_t j = 0; j < render_pass_descriptor.color_attachment_name_count && !attachment_found; j++) {
            if (strcmp(attachment_blend_descriptor.attachment_name, render_pass_descriptor.color_attachment_names[j]) == 0) {
                attachment_found = true;
            }
        }

        KW_ERROR(
            attachment_found,
            "Attachment \"%s\" is not found (render pass \"%s\", graphics pipeline \"%s\").", attachment_blend_descriptor.attachment_name, render_pass_descriptor.name, graphics_pipeline_descriptor.name
        );
    }
}

static void validate_depth_stencil_test(const FrameGraphDescriptor& frame_graph_descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline_descriptor = render_pass_descriptor.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        !graphics_pipeline_descriptor.is_depth_test_enabled || render_pass_descriptor.depth_stencil_attachment_name != nullptr,
        "Depth test requires a depth stencil attachment (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
    );

    KW_ERROR(
        !graphics_pipeline_descriptor.is_stencil_test_enabled || render_pass_descriptor.depth_stencil_attachment_name != nullptr,
        "Stencil test requires a depth stencil attachment (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
    );

    if (graphics_pipeline_descriptor.is_stencil_test_enabled) {
        for (size_t i = 0; i < frame_graph_descriptor.depth_stencil_attachment_descriptor_count; i++) {
            const AttachmentDescriptor& attachment_descriptor = frame_graph_descriptor.depth_stencil_attachment_descriptors[i];
            if (strcmp(render_pass_descriptor.depth_stencil_attachment_name, attachment_descriptor.name) == 0) {
                KW_ERROR(
                    attachment_descriptor.format == TextureFormat::D24_UNORM_S8_UINT || attachment_descriptor.format == TextureFormat::D32_FLOAT_S8X24_UINT,
                    "Stencil test requires a texture format that supports stencil (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );
            }
        }
    }
}

static void validate_primitive_topology(const FrameGraphDescriptor& frame_graph_descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline_descriptor = render_pass_descriptor.graphics_pipeline_descriptors[graphics_pipeline_index];

    if (graphics_pipeline_descriptor.primitive_topology == PrimitiveTopology::LINE_LIST || graphics_pipeline_descriptor.primitive_topology == PrimitiveTopology::LINE_STRIP) {
        KW_ERROR(
            graphics_pipeline_descriptor.fill_mode == FillMode::LINE || graphics_pipeline_descriptor.fill_mode == FillMode::POINT,
            "Line primitive topologies don't support FILL fill mode (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
        );
    } else if (graphics_pipeline_descriptor.primitive_topology == PrimitiveTopology::POINT_LIST) {
        KW_ERROR(
            graphics_pipeline_descriptor.fill_mode == FillMode::POINT,
            "Point primitive topology supports only POINT fill mode (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
        );
    }
}

static void validate_instance_binding_descriptors(const FrameGraphDescriptor& frame_graph_descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline_descriptor = render_pass_descriptor.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        graphics_pipeline_descriptor.instance_binding_descriptors != nullptr || graphics_pipeline_descriptor.instance_binding_descriptor_count == 0,
        "Invalid instance bindings (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
    );

    for (size_t i = 0; i < graphics_pipeline_descriptor.instance_binding_descriptor_count; i++) {
        const BindingDescriptor& binding_descriptor = graphics_pipeline_descriptor.instance_binding_descriptors[i];

        KW_ERROR(
            binding_descriptor.attribute_descriptor_count > 0,
            "Input binding requires at least one attribute (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
        );

        uint64_t stride = 0;

        for (size_t j = 0; j < binding_descriptor.attribute_descriptor_count; j++) {
            const AttributeDescriptor& attribute_descriptor = binding_descriptor.attribute_descriptors[j];

            for (size_t k = 0; k < graphics_pipeline_descriptor.vertex_binding_descriptor_count; k++) {
                const BindingDescriptor& another_binding_descriptor = graphics_pipeline_descriptor.vertex_binding_descriptors[k];

                for (size_t l = 0; l < another_binding_descriptor.attribute_descriptor_count; l++) {
                    const AttributeDescriptor& another_attribute_descriptor = another_binding_descriptor.attribute_descriptors[l];

                    KW_ERROR(
                        attribute_descriptor.semantic != another_attribute_descriptor.semantic || attribute_descriptor.semantic_index != another_attribute_descriptor.semantic_index,
                        "Semantic is already used (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
                    );
                }
            }

            for (size_t k = 0; k < i; k++) {
                const BindingDescriptor& another_binding_descriptor = graphics_pipeline_descriptor.instance_binding_descriptors[k];

                for (size_t l = 0; l < another_binding_descriptor.attribute_descriptor_count; l++) {
                    const AttributeDescriptor& another_attribute_descriptor = another_binding_descriptor.attribute_descriptors[l];

                    KW_ERROR(
                        attribute_descriptor.semantic != another_attribute_descriptor.semantic || attribute_descriptor.semantic_index != another_attribute_descriptor.semantic_index,
                        "Semantic is already used (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
                    );
                }
            }

            for (size_t k = 0; k < j; k++) {
                const AttributeDescriptor& another_attribute_descriptor = binding_descriptor.attribute_descriptors[k];

                KW_ERROR(
                    attribute_descriptor.semantic != another_attribute_descriptor.semantic || attribute_descriptor.semantic_index != another_attribute_descriptor.semantic_index,
                    "Semantic is already used (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );
            }

            KW_ERROR(
                attribute_descriptor.format != TextureFormat::UNKNOWN,
                "Invalid attribute format (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
            );

            KW_ERROR(
                !TextureFormatUtils::is_depth_stencil(attribute_descriptor.format),
                "Invalid attribute format (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
            );

            KW_ERROR(
                !TextureFormatUtils::is_compressed(attribute_descriptor.format),
                "Invalid attribute format (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
            );

            uint64_t size = TextureFormatUtils::get_texel_size(attribute_descriptor.format);

            KW_ERROR(
                attribute_descriptor.offset >= stride,
                "Overlapping attribute (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
            );

            stride = attribute_descriptor.offset + size;
        }

        KW_ERROR(
            binding_descriptor.stride >= stride,
            "Overlapping input binding (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
        );
    }
}

static void validate_vertex_binding_descriptors(const FrameGraphDescriptor& frame_graph_descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline_descriptor = render_pass_descriptor.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        graphics_pipeline_descriptor.vertex_binding_descriptors != nullptr || graphics_pipeline_descriptor.vertex_binding_descriptor_count == 0,
        "Invalid vertex bindings (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
    );

    for (size_t i = 0; i < graphics_pipeline_descriptor.vertex_binding_descriptor_count; i++) {
        const BindingDescriptor& binding_descriptor = graphics_pipeline_descriptor.vertex_binding_descriptors[i];

        KW_ERROR(
            binding_descriptor.attribute_descriptor_count > 0,
            "Input binding requires at least one attribute (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
        );

        uint64_t stride = 0;

        for (size_t j = 0; j < binding_descriptor.attribute_descriptor_count; j++) {
            const AttributeDescriptor& attribute_descriptor = binding_descriptor.attribute_descriptors[j];

            for (size_t k = 0; k < i; k++) {
                const BindingDescriptor& another_binding_descriptor = graphics_pipeline_descriptor.vertex_binding_descriptors[k];

                for (size_t l = 0; l < another_binding_descriptor.attribute_descriptor_count; l++) {
                    const AttributeDescriptor& another_attribute_descriptor = another_binding_descriptor.attribute_descriptors[l];

                    KW_ERROR(
                        attribute_descriptor.semantic != another_attribute_descriptor.semantic || attribute_descriptor.semantic_index != another_attribute_descriptor.semantic_index,
                        "Semantic is already used (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
                    );
                }
            }

            for (size_t k = 0; k < j; k++) {
                const AttributeDescriptor& another_attribute_descriptor = binding_descriptor.attribute_descriptors[k];

                KW_ERROR(
                    attribute_descriptor.semantic != another_attribute_descriptor.semantic || attribute_descriptor.semantic_index != another_attribute_descriptor.semantic_index,
                    "Semantic is already used (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
                );
            }

            KW_ERROR(
                attribute_descriptor.format != TextureFormat::UNKNOWN,
                "Invalid attribute format (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
            );

            KW_ERROR(
                !TextureFormatUtils::is_depth_stencil(attribute_descriptor.format),
                "Invalid attribute format (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
            );

            KW_ERROR(
                !TextureFormatUtils::is_compressed(attribute_descriptor.format),
                "Invalid attribute format (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
            );

            uint64_t size = TextureFormatUtils::get_texel_size(attribute_descriptor.format);

            KW_ERROR(
                attribute_descriptor.offset >= stride,
                "Overlapping attribute (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
            );

            stride = attribute_descriptor.offset + size;
        }

        KW_ERROR(
            binding_descriptor.stride >= stride,
            "Overlapping input binding (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
        );
    }
}

static void validate_graphics_pipeline_name(const FrameGraphDescriptor& frame_graph_descriptor, size_t render_pass_index, size_t graphics_pipeline_index) {
    const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline_descriptor = render_pass_descriptor.graphics_pipeline_descriptors[graphics_pipeline_index];

    KW_ERROR(
        graphics_pipeline_descriptor.name != nullptr,
        "Invalid graphics pipeline name (render pass \"%s\").", render_pass_descriptor.name
    );

    for (size_t i = 0; i < render_pass_index; i++) {
        const RenderPassDescriptor& another_render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[i];

        for (size_t j = 0; j < another_render_pass_descriptor.graphics_pipeline_descriptor_count; j++) {
            const GraphicsPipelineDescriptor& another_graphics_pipeline_descriptor = another_render_pass_descriptor.graphics_pipeline_descriptors[j];

            KW_ERROR(
                strcmp(graphics_pipeline_descriptor.name, another_graphics_pipeline_descriptor.name) != 0,
                "Graphics pipeline name \"%s\" is already used (render pass \"%s\").", graphics_pipeline_descriptor.name, render_pass_descriptor.name
            );
        }
    }

    for (size_t i = 0; i < graphics_pipeline_index; i++) {
        const GraphicsPipelineDescriptor& another_graphics_pipeline_descriptor = render_pass_descriptor.graphics_pipeline_descriptors[i];

        KW_ERROR(
            strcmp(graphics_pipeline_descriptor.name, another_graphics_pipeline_descriptor.name) != 0,
            "Graphics pipeline name \"%s\" is already used (render pass \"%s\").", graphics_pipeline_descriptor.name, render_pass_descriptor.name
        );
    }
}

static void validate_graphics_pipeline_descriptors(const FrameGraphDescriptor& frame_graph_descriptor, size_t render_pass_index) {
    const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];

    KW_ERROR(
        render_pass_descriptor.graphics_pipeline_descriptors != nullptr || render_pass_descriptor.graphics_pipeline_descriptor_count == 0,
        "Invalid graphics pipelines (render pass \"%s\").", render_pass_descriptor.name
    );

    for (size_t i = 0; i < render_pass_descriptor.graphics_pipeline_descriptor_count; i++) {
        const GraphicsPipelineDescriptor& graphics_pipeline_descriptor = render_pass_descriptor.graphics_pipeline_descriptors[i];

        validate_graphics_pipeline_name(frame_graph_descriptor, render_pass_index, i);
        validate_vertex_binding_descriptors(frame_graph_descriptor, render_pass_index, i);
        validate_instance_binding_descriptors(frame_graph_descriptor, render_pass_index, i);
        validate_primitive_topology(frame_graph_descriptor, render_pass_index, i);
        validate_depth_stencil_test(frame_graph_descriptor, render_pass_index, i);
        validate_attachment_blend_descriptors(frame_graph_descriptor, render_pass_index, i);
        validate_uniform_attachment_descriptors(frame_graph_descriptor, render_pass_index, i);
        validate_texture_descriptors(frame_graph_descriptor, render_pass_index, i);
        validate_sampler_descriptors(frame_graph_descriptor, render_pass_index, i);
        validate_uniform_buffer_descriptors(frame_graph_descriptor, render_pass_index, i);
        validate_push_constants(frame_graph_descriptor, render_pass_index, i);

        KW_ERROR(
            graphics_pipeline_descriptor.vertex_shader_filename != nullptr,
            "Invalid vertex shader (render pass \"%s\", graphics pipeline \"%s\").", render_pass_descriptor.name, graphics_pipeline_descriptor.name
        );
    }
}

template <typename T>
inline bool check_equal(T a, T b) {
    return a == b || (a == static_cast<T>(0) && b == static_cast<T>(1)) || (a == static_cast<T>(1) && b == static_cast<T>(0));
}

static bool validate_size(bool& is_set, SizeClass& size_class, float& width, float& height, uint32_t& count, SizeClass attachment_size_class, float attachment_width, float attachment_height, uint32_t attachment_count) {
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

static void validate_attachments(const FrameGraphDescriptor& frame_graph_descriptor, size_t render_pass_index) {
    const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];

    bool is_set = false;
    SizeClass size_class = SizeClass::RELATIVE;
    float width = 0.f;
    float height = 0.f;
    uint32_t count = 0;

    KW_ERROR(
        render_pass_descriptor.color_attachment_names != nullptr || render_pass_descriptor.color_attachment_name_count == 0,
        "Invalid write color attachments (render pass \"%s\").", render_pass_descriptor.name
    );

    for (size_t i = 0; i < render_pass_descriptor.color_attachment_name_count; i++) {
        const char* color_attachment_name = render_pass_descriptor.color_attachment_names[i];
        KW_ERROR(
            color_attachment_name != nullptr,
            "Invalid attachment name (render pass \"%s\").", render_pass_descriptor.name
        );

        bool create_found = false;

        if (strcmp(color_attachment_name, frame_graph_descriptor.swapchain_attachment_name) == 0) {
            KW_ERROR(
                validate_size(is_set, size_class, width, height, count, SizeClass::RELATIVE, 1.f, 1.f, 1),
                "Attachment \"%s\" size doesn't match (render pass \"%s\").", color_attachment_name, render_pass_descriptor.name
            );

            create_found = true;
        }

        for (size_t j = 0; j < frame_graph_descriptor.color_attachment_descriptor_count && !create_found; j++) {
            const AttachmentDescriptor& attachment_descriptor = frame_graph_descriptor.color_attachment_descriptors[j];

            if (strcmp(color_attachment_name, attachment_descriptor.name) == 0) {
                KW_ERROR(
                    validate_size(is_set, size_class, width, height, count, attachment_descriptor.size_class, attachment_descriptor.width, attachment_descriptor.height, attachment_descriptor.count),
                    "Attachment \"%s\" size doesn't match (render pass \"%s\").", color_attachment_name, render_pass_descriptor.name
                );

                create_found = true;
            }
        }

        KW_ERROR(
            create_found,
            "Attachment \"%s\" is not found (render pass \"%s\").", color_attachment_name, render_pass_descriptor.name
        );

        for (size_t j = 0; j < i; j++) {
            KW_ERROR(
                strcmp(color_attachment_name, render_pass_descriptor.color_attachment_names[j]) != 0,
                "Attachment \"%s\" is specified twice (render pass \"%s\").", color_attachment_name, render_pass_descriptor.name
            );
        }
    }

    KW_ERROR(
        render_pass_descriptor.color_attachment_name_count > 0 || render_pass_descriptor.depth_stencil_attachment_name != nullptr,
        "Attachments are not specified (render pass \"%s\").", render_pass_descriptor.name
    );

    if (render_pass_descriptor.depth_stencil_attachment_name != nullptr) {
        const char* depth_stencil_attachment_name = render_pass_descriptor.depth_stencil_attachment_name;

        bool create_found = false;

        for (size_t i = 0; i < frame_graph_descriptor.depth_stencil_attachment_descriptor_count && !create_found; i++) {
            const AttachmentDescriptor& attachment_descriptor = frame_graph_descriptor.depth_stencil_attachment_descriptors[i];

            if (strcmp(depth_stencil_attachment_name, attachment_descriptor.name) == 0) {
                KW_ERROR(
                    validate_size(is_set, size_class, width, height, count, attachment_descriptor.size_class, attachment_descriptor.width, attachment_descriptor.height, attachment_descriptor.count),
                    "Attachment \"%s\" size doesn't match (render pass \"%s\").", depth_stencil_attachment_name, render_pass_descriptor.name
                );

                create_found = true;
            }
        }

        KW_ERROR(
            create_found,
            "Attachment \"%s\" is not found (render pass \"%s\").", depth_stencil_attachment_name, render_pass_descriptor.name
        );
    }
}

static void validate_render_pass_name(const FrameGraphDescriptor& frame_graph_descriptor, size_t render_pass_index) {
    const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];

    KW_ERROR(render_pass_descriptor.name != nullptr, "Invalid render pass name.");

    for (size_t i = 0; i < render_pass_index; i++) {
        const RenderPassDescriptor& another_render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[i];

        KW_ERROR(
            strcmp(render_pass_descriptor.name, another_render_pass_descriptor.name) != 0,
            "Render pass name \"%s\" is already used.", render_pass_descriptor.name
        );
    }
}

static void validate_render_passes(const FrameGraphDescriptor& frame_graph_descriptor) {
    KW_ERROR(
        frame_graph_descriptor.render_pass_descriptors != nullptr || frame_graph_descriptor.render_pass_descriptor_count == 0,
        "Invalid render passes."
    );

    for (size_t i = 0; i < frame_graph_descriptor.render_pass_descriptor_count; i++) {
        const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[i];

        validate_render_pass_name(frame_graph_descriptor, i);

        validate_attachments(frame_graph_descriptor, i);

        validate_graphics_pipeline_descriptors(frame_graph_descriptor, i);

        KW_ERROR(
            render_pass_descriptor.render_pass != nullptr,
            "Invalid render pass \"%s\".", render_pass_descriptor.name
        );
    }
}

static void validate_depth_stencil_attachments(const FrameGraphDescriptor& frame_graph_descriptor) {
    KW_ERROR(
        frame_graph_descriptor.depth_stencil_attachment_descriptors != nullptr || frame_graph_descriptor.depth_stencil_attachment_descriptor_count == 0,
        "Invalid depth stencil attachments."
    );

    for (size_t i = 0; i < frame_graph_descriptor.depth_stencil_attachment_descriptor_count; i++) {
        const AttachmentDescriptor& attachment_descriptor = frame_graph_descriptor.depth_stencil_attachment_descriptors[i];

        KW_ERROR(
            attachment_descriptor.name != nullptr,
            "Invalid depth stencil attachment name."
        );

        KW_ERROR(
            strcmp(attachment_descriptor.name, frame_graph_descriptor.swapchain_attachment_name) != 0,
            "Attachment name \"%s\" is already used.", attachment_descriptor.name
        );

        for (size_t j = 0; j < frame_graph_descriptor.color_attachment_descriptor_count; j++) {
            const AttachmentDescriptor& another_attachment_descriptor = frame_graph_descriptor.color_attachment_descriptors[j];

            KW_ERROR(
                strcmp(attachment_descriptor.name, another_attachment_descriptor.name) != 0,
                "Attachment name \"%s\" is already used.", attachment_descriptor.name
            );
        }

        for (size_t j = 0; j < i; j++) {
            const AttachmentDescriptor& another_attachment_descriptor = frame_graph_descriptor.depth_stencil_attachment_descriptors[j];

            KW_ERROR(
                strcmp(attachment_descriptor.name, another_attachment_descriptor.name) != 0,
                "Attachment name \"%s\" is already used.", attachment_descriptor.name
            );
        }

        KW_ERROR(
            TextureFormatUtils::is_depth_stencil(attachment_descriptor.format),
            "Invalid depth stencil attachment \"%s\" format.", attachment_descriptor.name
        );

        if (attachment_descriptor.size_class == SizeClass::RELATIVE) {
            KW_ERROR(
                attachment_descriptor.width >= 0.f && attachment_descriptor.width <= 1.f,
                "Invalid attachment \"%s\" width.", attachment_descriptor.name
            );

            KW_ERROR(
                attachment_descriptor.height >= 0.f && attachment_descriptor.height <= 1.f,
                "Invalid attachment \"%s\" height.", attachment_descriptor.name
            );
        } else {
            KW_ERROR(
                attachment_descriptor.width > 0.f && static_cast<float>(static_cast<uint32_t>(attachment_descriptor.width)) == attachment_descriptor.width,
                "Invalid attachment \"%s\" width.", attachment_descriptor.name
            );

            KW_ERROR(
                attachment_descriptor.height > 0.f && static_cast<float>(static_cast<uint32_t>(attachment_descriptor.height)) == attachment_descriptor.height,
                "Invalid attachment \"%s\" height.", attachment_descriptor.name
            );
        }

        KW_ERROR(
            attachment_descriptor.clear_depth >= 0.f && attachment_descriptor.clear_depth >= 1.f,
            "Invalid attachment \"%s\" clear depth.", attachment_descriptor.name
        );
    }
}

static void validate_color_attachments(const FrameGraphDescriptor& frame_graph_descriptor) {
    KW_ERROR(
        frame_graph_descriptor.color_attachment_descriptors != nullptr || frame_graph_descriptor.color_attachment_descriptor_count == 0,
        "Invalid color attachments."
    );

    for (size_t i = 0; i < frame_graph_descriptor.color_attachment_descriptor_count; i++) {
        const AttachmentDescriptor& attachment_descriptor = frame_graph_descriptor.color_attachment_descriptors[i];

        KW_ERROR(
            attachment_descriptor.name != nullptr,
            "Invalid attachment name."
        );

        KW_ERROR(
            strcmp(attachment_descriptor.name, frame_graph_descriptor.swapchain_attachment_name) != 0,
            "Attachment name \"%s\" is already used.", attachment_descriptor.name
        );

        for (size_t j = 0; j < i; j++) {
            const AttachmentDescriptor& another_attachment_descriptor = frame_graph_descriptor.color_attachment_descriptors[j];

            KW_ERROR(
                strcmp(attachment_descriptor.name, another_attachment_descriptor.name) != 0,
                "Attachment name \"%s\" is already used.", attachment_descriptor.name
            );
        }

        KW_ERROR(
            attachment_descriptor.format != TextureFormat::UNKNOWN,
            "Invalid color attachment \"%s\" format.", attachment_descriptor.name
        );

        KW_ERROR(
            !TextureFormatUtils::is_depth_stencil(attachment_descriptor.format),
            "Invalid color attachment \"%s\" format.", attachment_descriptor.name
        );

        KW_ERROR(
            !TextureFormatUtils::is_compressed(attachment_descriptor.format),
            "Invalid color attachment \"%s\" format.", attachment_descriptor.name
        );

        if (attachment_descriptor.size_class == SizeClass::RELATIVE) {
            KW_ERROR(
                attachment_descriptor.width >= 0.f && attachment_descriptor.width <= 1.f,
                "Invalid attachment \"%s\" width.", attachment_descriptor.name
            );

            KW_ERROR(
                attachment_descriptor.height >= 0.f && attachment_descriptor.height <= 1.f,
                "Invalid attachment \"%s\" height.", attachment_descriptor.name
            );
        } else {
            KW_ERROR(
                attachment_descriptor.width > 0.f && static_cast<float>(static_cast<uint32_t>(attachment_descriptor.width)) == attachment_descriptor.width,
                "Invalid attachment \"%s\" width.", attachment_descriptor.name
            );

            KW_ERROR(
                attachment_descriptor.height > 0.f && static_cast<float>(static_cast<uint32_t>(attachment_descriptor.height)) == attachment_descriptor.height,
                "Invalid attachment \"%s\" height.", attachment_descriptor.name
            );
        }
        
        for (float component : attachment_descriptor.clear_color) {
            KW_ERROR(
                component >= 0.f,
                "Invalid attachment \"%s\" clear color.", attachment_descriptor.name
            );
        }
    }
}

FrameGraph* FrameGraph::create_instance(const FrameGraphDescriptor& frame_graph_descriptor) {
    KW_ERROR(frame_graph_descriptor.render != nullptr, "Invalid render.");
    KW_ERROR(frame_graph_descriptor.window != nullptr, "Invalid window.");
    KW_ERROR(frame_graph_descriptor.descriptor_set_count_per_descriptor_pool > 0, "At least one frame_graph_descriptor set per frame_graph_descriptor pool is required.");
    KW_ERROR(frame_graph_descriptor.uniform_texture_count_per_descriptor_pool > 0, "At least one texture per frame_graph_descriptor pool is required.");
    KW_ERROR(frame_graph_descriptor.uniform_sampler_count_per_descriptor_pool > 0, "At least one sampler per frame_graph_descriptor pool is required.");
    KW_ERROR(frame_graph_descriptor.uniform_buffer_count_per_descriptor_pool > 0, "At least one uniform buffer per frame_graph_descriptor pool is required.");
    KW_ERROR(frame_graph_descriptor.swapchain_attachment_name != nullptr, "Invalid swapchain name.");

    validate_color_attachments(frame_graph_descriptor);

    validate_depth_stencil_attachments(frame_graph_descriptor);

    validate_render_passes(frame_graph_descriptor);

    switch (frame_graph_descriptor.render->get_api()) {
    case RenderApi::VULKAN:
        return new FrameGraphVulkan(frame_graph_descriptor);
    default:
        KW_ERROR(false, "Chosen render API is not supported on your platform.");
    }
}

RenderPassImpl*& FrameGraph::get_render_pass_impl(RenderPass* render_pass) {
    return render_pass->m_impl;
}

} // namespace kw
