#pragma once

namespace kw {

class MemoryResourceLinear;

enum class RenderApi {
    VULKAN,
    DIRECTX,
};

struct RenderDescriptor {
    RenderApi api;

    MemoryResourceLinear* memory_resource;

    bool is_validation_enabled;
    bool is_debug_names_enabled;
};

class Render {
public:
    static Render* create_instance(const RenderDescriptor& descriptor);

    virtual ~Render() = default;

    virtual RenderApi get_api() const = 0;
};

} // namespace kw
