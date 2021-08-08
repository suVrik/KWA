#pragma once

#include "render/render_passes/base_render_pass.h"

#include <core/math/float2.h>

namespace kw {

class FullScreenQuadRenderPass : public BaseRenderPass {
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
