#pragma once

#include <cstdint>

namespace kw {

class FrameGraph;

class RenderPassImpl {
public:
    virtual RenderPassContext* begin(uint32_t attachment_index) = 0;
};

} // namespace kw
