#pragma once

#include <cstdint>

namespace kw {

class FrameGraph;
class HostTexture;

class RenderPassImpl {
public:
    virtual RenderPassContext* begin(uint32_t attachment_index) = 0;

    virtual uint64_t blit(const char* source_attachment, HostTexture* destination_host_texture) = 0;
};

} // namespace kw
