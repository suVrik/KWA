#pragma once

#include <cstdint>

namespace kw {

class FrameGraph;
class HostTexture;
class Texture;

class RenderPassImpl {
public:
    virtual RenderPassContext* begin(uint32_t context_index) = 0;

    virtual uint64_t blit(const char* source_attachment, HostTexture* destination_host_texture, uint32_t context_index) = 0;

    virtual void blit(const char* source_attachment, Texture* destination_texture, uint32_t destination_layer, uint32_t context_index) = 0;
};

} // namespace kw
