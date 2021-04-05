#pragma once

#include "render/render.h"

#include <core/string.h>

namespace kw::RenderUtils {

TextureDescriptor load_dds(MemoryResource& memory_resource, const String& relative_path);

} // namespace kw::RenderUtils
