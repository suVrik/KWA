#include "render/frame_graph.h"
#include "render/render.h"
#include "render/render_pass_impl.h"
#include "render/vulkan/frame_graph_vulkan.h"
#include "render/vulkan/render_vulkan.h"

#include <core/debug/assert.h>
#include <core/error.h>

#include <algorithm>

namespace kw {

RenderPassContext* RenderPass::begin(uint32_t context_index) {
    KW_ASSERT(m_impl != nullptr, "Frame graph was not initialized yet.");

    return m_impl->begin(context_index);
}

uint64_t RenderPass::blit(const char* source_attachment, HostTexture* destination_host_texture, uint32_t context_index) {
    KW_ASSERT(m_impl != nullptr, "Frame graph was not initialized yet.");

    return m_impl->blit(source_attachment, destination_host_texture, context_index);
}

void RenderPass::blit(const char* source_attachment, Texture* destination_texture, uint32_t destination_layer, uint32_t context_index) {
    KW_ASSERT(m_impl != nullptr, "Frame graph was not initialized yet.");

    m_impl->blit(source_attachment, destination_texture, destination_layer, context_index);
}

inline bool check_equal(float a, float b) {
    return a == b || (a == 0.f && b == 1.f) || (a == 1.f && b == 0.f);
}

static bool validate_size(bool& is_set, SizeClass& size_class, float& width, float& height, SizeClass attachment_size_class, float attachment_width, float attachment_height) {
    if (!is_set) {
        is_set = true;

        size_class = attachment_size_class;
        width = attachment_width;
        height = attachment_height;

        return true;
    }

    return attachment_size_class == size_class && check_equal(attachment_width, width) && check_equal(attachment_height, height);
}

static void validate_attachments(const FrameGraphDescriptor& frame_graph_descriptor, size_t render_pass_index) {
    const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];

    for (size_t i = 0; i < render_pass_descriptor.read_attachment_name_count; i++) {
        const char* attachment_name = render_pass_descriptor.read_attachment_names[i];

        KW_ERROR(
            attachment_name != nullptr,
            "Invalid read attachment name (render pass \"%s\").", render_pass_descriptor.name
        );

        bool create_found = false;

        if (std::strcmp(attachment_name, frame_graph_descriptor.swapchain_attachment_name) == 0) {
            create_found = true;
        }

        for (size_t j = 0; j < frame_graph_descriptor.color_attachment_descriptor_count && !create_found; j++) {
            if (std::strcmp(attachment_name, frame_graph_descriptor.color_attachment_descriptors[j].name) == 0) {
                create_found = true;
            }
        }

        for (size_t j = 0; j < frame_graph_descriptor.depth_stencil_attachment_descriptor_count && !create_found; j++) {
            if (std::strcmp(attachment_name, frame_graph_descriptor.depth_stencil_attachment_descriptors[j].name) == 0) {
                create_found = true;
            }
        }

        KW_ERROR(
            create_found,
            "Read attachment \"%s\" is not found (render pass \"%s\").", attachment_name, render_pass_descriptor.name
        );

        for (size_t j = 0; j < i; j++) {
            KW_ERROR(
                std::strcmp(attachment_name, render_pass_descriptor.read_attachment_names[j]) != 0,
                "Read attachment \"%s\" is specified twice (render pass \"%s\").", attachment_name, render_pass_descriptor.name
            );
        }
    }

    bool is_set = false;
    SizeClass size_class = SizeClass::RELATIVE;
    float width = 0.f;
    float height = 0.f;

    KW_ERROR(
        render_pass_descriptor.write_color_attachment_names != nullptr || render_pass_descriptor.write_color_attachment_name_count == 0,
        "Invalid write color attachments (render pass \"%s\").", render_pass_descriptor.name
    );

    for (size_t i = 0; i < render_pass_descriptor.write_color_attachment_name_count; i++) {
        const char* color_attachment_name = render_pass_descriptor.write_color_attachment_names[i];

        KW_ERROR(
            color_attachment_name != nullptr,
            "Invalid write color attachment name (render pass \"%s\").", render_pass_descriptor.name
        );

        bool create_found = false;

        if (std::strcmp(color_attachment_name, frame_graph_descriptor.swapchain_attachment_name) == 0) {
            KW_ERROR(
                validate_size(is_set, size_class, width, height, SizeClass::RELATIVE, 1.f, 1.f),
                "Attachment \"%s\" size doesn't match (render pass \"%s\").", color_attachment_name, render_pass_descriptor.name
            );

            create_found = true;
        }

        for (size_t j = 0; j < frame_graph_descriptor.color_attachment_descriptor_count && !create_found; j++) {
            const AttachmentDescriptor& attachment_descriptor = frame_graph_descriptor.color_attachment_descriptors[j];

            if (std::strcmp(color_attachment_name, attachment_descriptor.name) == 0) {
                KW_ERROR(
                    validate_size(is_set, size_class, width, height, attachment_descriptor.size_class, attachment_descriptor.width, attachment_descriptor.height),
                    "Attachment \"%s\" size doesn't match (render pass \"%s\").", color_attachment_name, render_pass_descriptor.name
                );

                create_found = true;
            }
        }

        KW_ERROR(
            create_found,
            "Write color attachment \"%s\" is not found (render pass \"%s\").", color_attachment_name, render_pass_descriptor.name
        );

        for (size_t j = 0; j < render_pass_descriptor.read_attachment_name_count; j++) {
            KW_ERROR(
                std::strcmp(color_attachment_name, render_pass_descriptor.read_attachment_names[j]) != 0,
                "Write color attachment \"%s\" is already specified in read attachments (render pass \"%s\").", color_attachment_name, render_pass_descriptor.name
            );
        }

        for (size_t j = 0; j < i; j++) {
            KW_ERROR(
                std::strcmp(color_attachment_name, render_pass_descriptor.write_color_attachment_names[j]) != 0,
                "Write color attachment \"%s\" is specified twice (render pass \"%s\").", color_attachment_name, render_pass_descriptor.name
            );
        }
    }

    KW_ERROR(
        render_pass_descriptor.write_color_attachment_name_count > 0 ||
        render_pass_descriptor.write_depth_stencil_attachment_name != nullptr,
        "Write attachments are not specified (render pass \"%s\").", render_pass_descriptor.name
    );

    KW_ERROR(
        render_pass_descriptor.read_depth_stencil_attachment_name == nullptr ||
        render_pass_descriptor.write_depth_stencil_attachment_name == nullptr,
        "Both read and write depth stencil attachments are not allowed (render pass \"%s\").", render_pass_descriptor.name
    );

    if (render_pass_descriptor.read_depth_stencil_attachment_name != nullptr ||
        render_pass_descriptor.write_depth_stencil_attachment_name != nullptr)
    {
        const char* depth_stencil_attachment_name;
        if (render_pass_descriptor.read_depth_stencil_attachment_name != nullptr) {
            depth_stencil_attachment_name = render_pass_descriptor.read_depth_stencil_attachment_name;
        } else {
            depth_stencil_attachment_name = render_pass_descriptor.write_depth_stencil_attachment_name;
        }

        bool create_found = false;

        for (size_t i = 0; i < frame_graph_descriptor.depth_stencil_attachment_descriptor_count && !create_found; i++) {
            const AttachmentDescriptor& attachment_descriptor = frame_graph_descriptor.depth_stencil_attachment_descriptors[i];

            if (std::strcmp(depth_stencil_attachment_name, attachment_descriptor.name) == 0) {
                KW_ERROR(
                    validate_size(is_set, size_class, width, height, attachment_descriptor.size_class, attachment_descriptor.width, attachment_descriptor.height),
                    "Attachment \"%s\" size doesn't match (render pass \"%s\").", depth_stencil_attachment_name, render_pass_descriptor.name
                );

                create_found = true;
            }
        }

        KW_ERROR(
            create_found,
            "Depth stencil attachment \"%s\" is not found (render pass \"%s\").", depth_stencil_attachment_name, render_pass_descriptor.name
        );

        if (render_pass_descriptor.write_depth_stencil_attachment_name != nullptr) {
            for (size_t i = 0; i < render_pass_descriptor.read_attachment_name_count; i++) {
                KW_ERROR(
                    std::strcmp(depth_stencil_attachment_name, render_pass_descriptor.read_attachment_names[i]) != 0,
                    "Write depth stencil attachment \"%s\" is already specified in read attachments (render pass \"%s\").", depth_stencil_attachment_name, render_pass_descriptor.name
                );
            }
        }
    }
}

static void validate_render_pass_name(const FrameGraphDescriptor& frame_graph_descriptor, size_t render_pass_index) {
    const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];

    KW_ERROR(render_pass_descriptor.name != nullptr, "Invalid render pass name.");

    for (size_t i = 0; i < render_pass_index; i++) {
        const RenderPassDescriptor& another_render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[i];

        KW_ERROR(
            std::strcmp(render_pass_descriptor.name, another_render_pass_descriptor.name) != 0,
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
            std::strcmp(attachment_descriptor.name, frame_graph_descriptor.swapchain_attachment_name) != 0,
            "Attachment name \"%s\" is already used.", attachment_descriptor.name
        );

        for (size_t j = 0; j < frame_graph_descriptor.color_attachment_descriptor_count; j++) {
            const AttachmentDescriptor& another_attachment_descriptor = frame_graph_descriptor.color_attachment_descriptors[j];

            KW_ERROR(
                std::strcmp(attachment_descriptor.name, another_attachment_descriptor.name) != 0,
                "Attachment name \"%s\" is already used.", attachment_descriptor.name
            );
        }

        for (size_t j = 0; j < i; j++) {
            const AttachmentDescriptor& another_attachment_descriptor = frame_graph_descriptor.depth_stencil_attachment_descriptors[j];

            KW_ERROR(
                std::strcmp(attachment_descriptor.name, another_attachment_descriptor.name) != 0,
                "Attachment name \"%s\" is already used.", attachment_descriptor.name
            );
        }

        KW_ERROR(
            TextureFormatUtils::is_depth(attachment_descriptor.format),
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
            attachment_descriptor.clear_depth >= 0.f && attachment_descriptor.clear_depth <= 1.f,
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
            std::strcmp(attachment_descriptor.name, frame_graph_descriptor.swapchain_attachment_name) != 0,
            "Attachment name \"%s\" is already used.", attachment_descriptor.name
        );

        for (size_t j = 0; j < i; j++) {
            const AttachmentDescriptor& another_attachment_descriptor = frame_graph_descriptor.color_attachment_descriptors[j];

            KW_ERROR(
                std::strcmp(attachment_descriptor.name, another_attachment_descriptor.name) != 0,
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
        // TODO: Perhaps own memory resource for frame graph? To distinguish memory allocations for profiling.
        return static_cast<RenderVulkan*>(frame_graph_descriptor.render)->persistent_memory_resource.construct<FrameGraphVulkan>(frame_graph_descriptor);
    default:
        KW_ERROR(false, "Chosen render API is not supported on your platform.");
    }
}

RenderPassImpl*& FrameGraph::get_render_pass_impl(RenderPass* render_pass) {
    return render_pass->m_impl;
}

} // namespace kw
