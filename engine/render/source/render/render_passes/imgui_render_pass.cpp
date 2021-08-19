#include "render/render_passes/imgui_render_pass.h"
#include "render/debug/imgui_manager.h"

#include <core/concurrency/task.h>
#include <core/debug/assert.h>
#include <core/math/float2.h>

namespace kw {

struct ImguiVertex {
    float2 position;
    float2 texcoord;
    uint32_t color;
};

struct ImguiPushConstants {
    float2 scale;
    float2 translate;
};

class ImguiRenderPass::Task : public kw::Task {
public:
    Task(ImguiRenderPass& render_pass)
        : m_render_pass(render_pass)
    {
    }

    void run() override {
        ImGui& imgui = m_render_pass.m_imgui_manager.get_imgui();

        RenderPassContext* context = m_render_pass.begin();
        if (context != nullptr) {
            imgui.Render();

            ImDrawData* draw_data = imgui.GetDrawData();
            KW_ASSERT(draw_data != nullptr);

            if (draw_data->TotalVtxCount <= 0) {
                return;
            }

            Vector<ImDrawVert> vertices(draw_data->TotalVtxCount, m_render_pass.m_transient_memory_resource);
            Vector<ImDrawIdx> indices(draw_data->TotalIdxCount, m_render_pass.m_transient_memory_resource);

            ImDrawVert* current_vertex = vertices.data();
            ImDrawIdx* current_index = indices.data();

            for (int i = 0; i < draw_data->CmdListsCount; i++) {
                const ImDrawList* command_list = draw_data->CmdLists[i];

                std::memcpy(current_vertex, command_list->VtxBuffer.Data, command_list->VtxBuffer.Size * sizeof(ImDrawVert));
                std::memcpy(current_index, command_list->IdxBuffer.Data, command_list->IdxBuffer.Size * sizeof(ImDrawIdx));

                current_vertex += command_list->VtxBuffer.Size;
                current_index += command_list->IdxBuffer.Size;
            }

            VertexBuffer* vertex_buffer = context->get_render().acquire_transient_vertex_buffer(vertices.data(), vertices.size() * sizeof(ImDrawVert));
            IndexBuffer* index_buffer = context->get_render().acquire_transient_index_buffer(indices.data(), indices.size() * sizeof(ImDrawIdx), IndexSize::UINT16);

            ImguiPushConstants push_constants{};
            push_constants.scale.x = 2.f / draw_data->DisplaySize.x;
            push_constants.scale.y = -2.f / draw_data->DisplaySize.y;
            push_constants.translate.x = -1.f + draw_data->DisplayPos.x * push_constants.scale.x;
            push_constants.translate.y = 1.f - draw_data->DisplayPos.y * push_constants.scale.y;

            int index_offset = 0;
            int vertex_offset = 0;

            for (int i = 0; i < draw_data->CmdListsCount; i++) {
                const ImDrawList* draw_list = draw_data->CmdLists[i];
                KW_ASSERT(draw_list != nullptr);

                for (int j = 0; j < draw_list->CmdBuffer.Size; j++) {
                    const ImDrawCmd& draw_command = draw_list->CmdBuffer[j];
                    KW_ASSERT(draw_command.UserCallback == nullptr);

                    ImVec4 scissor{};
                    scissor.x = clamp(draw_command.ClipRect.x * draw_data->FramebufferScale.x, 0.f, static_cast<float>(context->get_attachment_width()));
                    scissor.y = clamp(draw_command.ClipRect.y * draw_data->FramebufferScale.y, 0.f, static_cast<float>(context->get_attachment_height()));
                    scissor.z = clamp(draw_command.ClipRect.z * draw_data->FramebufferScale.x, 0.f, static_cast<float>(context->get_attachment_width()));
                    scissor.w = clamp(draw_command.ClipRect.w * draw_data->FramebufferScale.y, 0.f, static_cast<float>(context->get_attachment_height()));

                    Texture* uniform_texture = static_cast<Texture*>(draw_command.TextureId);
                    KW_ASSERT(uniform_texture != nullptr);

                    if (scissor.z > scissor.x && scissor.w > scissor.y) {
                        DrawCallDescriptor draw_call_descriptor{};
                        draw_call_descriptor.graphics_pipeline = m_render_pass.m_graphics_pipeline;
                        draw_call_descriptor.vertex_buffers = &vertex_buffer;
                        draw_call_descriptor.vertex_buffer_count = 1;
                        draw_call_descriptor.index_buffer = index_buffer;
                        draw_call_descriptor.index_count = draw_command.ElemCount;
                        draw_call_descriptor.index_offset = draw_command.IdxOffset + index_offset;
                        draw_call_descriptor.vertex_offset = draw_command.VtxOffset + vertex_offset;
                        draw_call_descriptor.override_scissors = true;
                        draw_call_descriptor.scissors.x = static_cast<uint32_t>(scissor.x);
                        draw_call_descriptor.scissors.y = static_cast<uint32_t>(scissor.y);
                        draw_call_descriptor.scissors.width = static_cast<uint32_t>(scissor.z - scissor.x);
                        draw_call_descriptor.scissors.height = static_cast<uint32_t>(scissor.w - scissor.y);
                        draw_call_descriptor.uniform_textures = &uniform_texture;
                        draw_call_descriptor.uniform_texture_count = 1;
                        draw_call_descriptor.push_constants = &push_constants;
                        draw_call_descriptor.push_constants_size = sizeof(push_constants);

                        context->draw(draw_call_descriptor);
                    }
                }

                index_offset += draw_list->IdxBuffer.Size;
                vertex_offset += draw_list->VtxBuffer.Size;
            }
        } else {
            // Either `Render` or `EndFrame` must be called before `NewFrame`.
            imgui.EndFrame();
        }
    }

    const char* get_name() const override {
        return "ImGui Render Pass";
    }

private:
    ImguiRenderPass& m_render_pass;
};

ImguiRenderPass::ImguiRenderPass(const ImguiRenderPassDescriptor& descriptor)
    : m_render(*descriptor.render)
    , m_imgui_manager(*descriptor.imgui_manager)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
    , m_font_texture(nullptr)
    , m_graphics_pipeline(nullptr)
{
    KW_ASSERT(descriptor.render != nullptr);
    KW_ASSERT(descriptor.imgui_manager != nullptr);
    KW_ASSERT(descriptor.transient_memory_resource != nullptr);

    ImGuiIO& io = m_imgui_manager.get_imgui().GetIO();

    io.Fonts->AddFontDefault();

    unsigned char* data;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&data, &width, &height);

    CreateTextureDescriptor create_texture_descriptor{};
    create_texture_descriptor.name = "imgui_font";
    create_texture_descriptor.type = TextureType::TEXTURE_2D;
    create_texture_descriptor.format = TextureFormat::RGBA8_UNORM;
    create_texture_descriptor.width = static_cast<uint32_t>(width);
    create_texture_descriptor.height = static_cast<uint32_t>(height);

    m_font_texture = m_render.create_texture(create_texture_descriptor);

    UploadTextureDescriptor upload_texture_descriptor{};
    upload_texture_descriptor.texture = m_font_texture;
    upload_texture_descriptor.data = data;
    upload_texture_descriptor.size = 4ull * width * height;
    upload_texture_descriptor.width = static_cast<uint32_t>(width);
    upload_texture_descriptor.height = static_cast<uint32_t>(height);

    m_render.upload_texture(upload_texture_descriptor);

    io.Fonts->SetTexID(m_font_texture);
}

ImguiRenderPass::~ImguiRenderPass() {
    m_render.destroy_texture(m_font_texture);
}

void ImguiRenderPass::get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // Write to `swapchain_attachment`.
}

void ImguiRenderPass::get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // Doesn't use any depth stencil attachments.
}

void ImguiRenderPass::get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) {
    static const char* const COLOR_ATTACHMENT_NAME = "swapchain_attachment";

    RenderPassDescriptor render_pass_descriptor{};
    render_pass_descriptor.name = "imgui_render_pass";
    render_pass_descriptor.render_pass = this;
    render_pass_descriptor.write_color_attachment_names = &COLOR_ATTACHMENT_NAME;
    render_pass_descriptor.write_color_attachment_name_count = 1;
    render_pass_descriptors.push_back(render_pass_descriptor);
}

void ImguiRenderPass::create_graphics_pipelines(FrameGraph& frame_graph) {
    AttributeDescriptor attribute_descriptors[3]{};
    attribute_descriptors[0].semantic = Semantic::POSITION;
    attribute_descriptors[0].format = TextureFormat::RG32_FLOAT;
    attribute_descriptors[0].offset = offsetof(ImguiVertex, position);
    attribute_descriptors[1].semantic = Semantic::TEXCOORD;
    attribute_descriptors[1].format = TextureFormat::RG32_FLOAT;
    attribute_descriptors[1].offset = offsetof(ImguiVertex, texcoord);
    attribute_descriptors[2].semantic = Semantic::COLOR;
    attribute_descriptors[2].format = TextureFormat::RGBA8_UNORM;
    attribute_descriptors[2].offset = offsetof(ImguiVertex, color);

    BindingDescriptor binding_descriptor{};
    binding_descriptor.attribute_descriptors = attribute_descriptors;
    binding_descriptor.attribute_descriptor_count = std::size(attribute_descriptors);
    binding_descriptor.stride = sizeof(ImguiVertex);

    AttachmentBlendDescriptor attachment_blend_descriptor{};
    attachment_blend_descriptor.attachment_name = "swapchain_attachment";
    attachment_blend_descriptor.source_color_blend_factor = BlendFactor::SOURCE_ALPHA;
    attachment_blend_descriptor.destination_color_blend_factor = BlendFactor::SOURCE_INVERSE_ALPHA;
    attachment_blend_descriptor.color_blend_op = BlendOp::ADD;
    attachment_blend_descriptor.source_alpha_blend_factor = BlendFactor::ONE;
    attachment_blend_descriptor.destination_alpha_blend_factor = BlendFactor::SOURCE_INVERSE_ALPHA;
    attachment_blend_descriptor.alpha_blend_op = BlendOp::ADD;

    UniformTextureDescriptor uniform_texture_descriptor{};
    uniform_texture_descriptor.variable_name = "texture_uniform";

    UniformSamplerDescriptor uniform_sampler_descriptor{};
    uniform_sampler_descriptor.variable_name = "sampler_uniform";
    uniform_sampler_descriptor.max_lod = 15.f;

    GraphicsPipelineDescriptor graphics_pipeline_descriptor{};
    graphics_pipeline_descriptor.graphics_pipeline_name = "imgui_graphics_pipeline";
    graphics_pipeline_descriptor.render_pass_name = "imgui_render_pass";
    graphics_pipeline_descriptor.vertex_shader_filename = "resource/shaders/imgui_vertex.hlsl";
    graphics_pipeline_descriptor.fragment_shader_filename = "resource/shaders/imgui_fragment.hlsl";
    graphics_pipeline_descriptor.vertex_binding_descriptors = &binding_descriptor;
    graphics_pipeline_descriptor.vertex_binding_descriptor_count = 1;
    graphics_pipeline_descriptor.cull_mode = CullMode::NONE;
    graphics_pipeline_descriptor.attachment_blend_descriptors = &attachment_blend_descriptor;
    graphics_pipeline_descriptor.attachment_blend_descriptor_count = 1;
    graphics_pipeline_descriptor.uniform_texture_descriptors = &uniform_texture_descriptor;
    graphics_pipeline_descriptor.uniform_texture_descriptor_count = 1;
    graphics_pipeline_descriptor.uniform_sampler_descriptors = &uniform_sampler_descriptor;
    graphics_pipeline_descriptor.uniform_sampler_descriptor_count = 1;
    graphics_pipeline_descriptor.push_constants_name = "imgui_push_constants";
    graphics_pipeline_descriptor.push_constants_size = sizeof(ImguiPushConstants);

    m_graphics_pipeline = frame_graph.create_graphics_pipeline(graphics_pipeline_descriptor);
}

void ImguiRenderPass::destroy_graphics_pipelines(FrameGraph& frame_graph) {
    frame_graph.destroy_graphics_pipeline(m_graphics_pipeline);
}

Task* ImguiRenderPass::create_task() {
    return m_transient_memory_resource.construct<Task>(*this);
}

} // namespace kw
