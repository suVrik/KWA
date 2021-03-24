#pragma once

#include "core/string.h"
#include "core/vector.h"

namespace kw::FilesystemUtils {

VectorLinear<std::byte> read_file(const StringLinear& relative_path);

} // namespace kw::FilesystemUtils
