#pragma once

#include "render/frame_graph.h"

#include <core/math/float2.h>

namespace kw {

class FullScreenQuadRenderPass : public RenderPass {
public:
    struct Vertex {
        float2 position;
        float2 texcoord;
    };

    FullScreenQuadRenderPass(Render& render);
    ~FullScreenQuadRenderPass() override;

protected:
    Render& m_render;

    VertexBuffer* m_vertex_buffer;
    IndexBuffer* m_index_buffer;
};

} // namespace kw
