#pragma once

#include "render/render.h"

#include <core/string.h>

namespace kw::RenderUtils {

TextureDescriptor load_dds(const StringLinear& relative_path);

} // namespace kw::RenderUtils
