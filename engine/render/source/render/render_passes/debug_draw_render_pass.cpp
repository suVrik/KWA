#include "render/render_passes/debug_draw_render_pass.h"
#include "render/debug/debug_draw_manager.h"
#include "render/scene/scene.h"

#include <core/concurrency/task.h>
#include <core/memory/memory_resource.h>

#include <numeric>

namespace kw {

struct DebugDrawVertex {
    float3 position;
    float3 color;
};

class DebugDrawRenderPass::Task : public kw::Task {
public:
    Task(DebugDrawRenderPass& render_pass)
        : m_render_pass(render_pass)
    {
    }

    void run() override {
        if (m_render_pass.m_debug_draw_manager.m_last_line == nullptr) {
            return;
        }

        RenderPassContext* context = m_render_pass.begin();
        if (context != nullptr) {
            DebugDrawManager::Line* lines = m_render_pass.m_debug_draw_manager.m_last_line.load(std::memory_order_acquire);
            size_t line_count = 0;

            DebugDrawManager::Line* current = lines;
            while (current != nullptr) {
                line_count++;
                current = current->previous;
            }

            Vector<DebugDrawVertex> vertices(m_render_pass.m_transient_memory_resource);
            vertices.reserve(2 * line_count);

            current = lines;
            while (current != nullptr) {
                vertices.push_back(DebugDrawVertex{ current->from, current->color });
                vertices.push_back(DebugDrawVertex{ current->to,   current->color });
                current = current->previous;
            }

            Vector<uint32_t> indices(2 * line_count, m_render_pass.m_transient_memory_resource);
            std::iota(indices.begin(), indices.end(), 0);

            VertexBuffer* vertex_buffer = context->get_render().acquire_transient_vertex_buffer(vertices.data(), vertices.size() * sizeof(DebugDrawVertex));
            IndexBuffer* index_buffer = context->get_render().acquire_transient_index_buffer(indices.data(), indices.size() * sizeof(uint32_t), IndexSize::UINT32);

            float4x4 debug_draw_push_constants = m_render_pass.m_scene.get_camera().get_view_projection_matrix();

            DrawCallDescriptor draw_call_descriptor{};
            draw_call_descriptor.graphics_pipeline = m_render_pass.m_graphics_pipeline;
            draw_call_descriptor.vertex_buffers = &vertex_buffer;
            draw_call_descriptor.vertex_buffer_count = 1;
            draw_call_descriptor.index_buffer = index_buffer;
            draw_call_descriptor.index_count = indices.size();
            draw_call_descriptor.push_constants = &debug_draw_push_constants;
            draw_call_descriptor.push_constants_size = sizeof(debug_draw_push_constants);

            context->draw(draw_call_descriptor);
        }
    }

    const char* get_name() const {
        return "Debug Draw Render Pass";
    }

private:
    DebugDrawRenderPass& m_render_pass;
};

DebugDrawRenderPass::DebugDrawRenderPass(Render& render, Scene& scene, DebugDrawManager& debug_draw_manager, MemoryResource& transient_memory_resource)
    : m_render(render)
    , m_scene(scene)
    , m_debug_draw_manager(debug_draw_manager)
    , m_transient_memory_resource(transient_memory_resource)
    , m_graphics_pipeline(nullptr)
{
}

void DebugDrawRenderPass::get_color_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // Write to `swapchain_attachment`.
}

void DebugDrawRenderPass::get_depth_stencil_attachment_descriptors(Vector<AttachmentDescriptor>& attachment_descriptors) {
    // Test against `depth_attachment`.
}

void DebugDrawRenderPass::get_render_pass_descriptors(Vector<RenderPassDescriptor>& render_pass_descriptors) {
    static const char* const COLOR_ATTACHMENT_NAME = "swapchain_attachment";

    RenderPassDescriptor render_pass_descriptor{};
    render_pass_descriptor.name = "debug_draw_render_pass";
    render_pass_descriptor.render_pass = this;
    render_pass_descriptor.write_color_attachment_names = &COLOR_ATTACHMENT_NAME;
    render_pass_descriptor.write_color_attachment_name_count = 1;
    render_pass_descriptor.read_depth_stencil_attachment_name = "depth_attachment";
    render_pass_descriptors.push_back(render_pass_descriptor);
}

void DebugDrawRenderPass::create_graphics_pipelines(FrameGraph& frame_graph) {
    AttributeDescriptor attribute_descriptors[2]{};
    attribute_descriptors[0].semantic = Semantic::POSITION;
    attribute_descriptors[0].format = TextureFormat::RGB32_FLOAT;
    attribute_descriptors[0].offset = offsetof(DebugDrawVertex, position);
    attribute_descriptors[1].semantic = Semantic::COLOR;
    attribute_descriptors[1].format = TextureFormat::RGB32_FLOAT;
    attribute_descriptors[1].offset = offsetof(DebugDrawVertex, color);

    BindingDescriptor binding_descriptor{};
    binding_descriptor.attribute_descriptors = attribute_descriptors;
    binding_descriptor.attribute_descriptor_count = std::size(attribute_descriptors);
    binding_descriptor.stride = sizeof(DebugDrawVertex);

    GraphicsPipelineDescriptor graphics_pipeline_descriptor{};
    graphics_pipeline_descriptor.graphics_pipeline_name = "debug_draw_graphics_pipeline";
    graphics_pipeline_descriptor.render_pass_name = "debug_draw_render_pass";
    graphics_pipeline_descriptor.vertex_shader_filename = "resource/shaders/debug_draw_vertex.hlsl";
    graphics_pipeline_descriptor.fragment_shader_filename = "resource/shaders/debug_draw_fragment.hlsl";
    graphics_pipeline_descriptor.vertex_binding_descriptors = &binding_descriptor;
    graphics_pipeline_descriptor.vertex_binding_descriptor_count = 1;
    graphics_pipeline_descriptor.primitive_topology = PrimitiveTopology::LINE_LIST;
    graphics_pipeline_descriptor.fill_mode = FillMode::LINE;
    graphics_pipeline_descriptor.is_depth_test_enabled = true;
    graphics_pipeline_descriptor.depth_compare_op = CompareOp::LESS;
    graphics_pipeline_descriptor.push_constants_name = "debug_draw_push_constants";
    graphics_pipeline_descriptor.push_constants_size = sizeof(float4x4);

    m_graphics_pipeline = frame_graph.create_graphics_pipeline(graphics_pipeline_descriptor);
}

void DebugDrawRenderPass::destroy_graphics_pipelines(FrameGraph& frame_graph) {
    frame_graph.destroy_graphics_pipeline(m_graphics_pipeline);
}

Task* DebugDrawRenderPass::create_task() {
    return new (m_transient_memory_resource.allocate<Task>()) Task(*this);
}

} // namespace kw
