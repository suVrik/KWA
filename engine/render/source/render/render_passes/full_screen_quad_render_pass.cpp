#include "render/render_passes/full_screen_quad_render_pass.h"

namespace kw {

FullScreenQuadRenderPass::FullScreenQuadRenderPass(Render& render)
    : m_render(render)
{
    static const Vertex VERTEX_DATA[] = {
        { float2(-1.f,  1.f), float2(0.f, 0.f) },
        { float2( 1.f,  1.f), float2(1.f, 0.f) },
        { float2( 1.f, -1.f), float2(1.f, 1.f) },
        { float2(-1.f, -1.f), float2(0.f, 1.f) },
    };

    m_vertex_buffer = render.create_vertex_buffer("full_screen_quad", sizeof(VERTEX_DATA));
    render.upload_vertex_buffer(m_vertex_buffer, VERTEX_DATA, sizeof(VERTEX_DATA));

    static const uint16_t INDEX_DATA[] = {
        0, 1, 3,
        1, 2, 3,
    };

    m_index_buffer = render.create_index_buffer("full_screen_quad", sizeof(INDEX_DATA), IndexSize::UINT16);
    render.upload_index_buffer(m_index_buffer, INDEX_DATA, sizeof(INDEX_DATA));
}

FullScreenQuadRenderPass::~FullScreenQuadRenderPass() {
    m_render.destroy_index_buffer(m_index_buffer);
    m_render.destroy_vertex_buffer(m_vertex_buffer);
}

} // namespace kw
