#pragma once

#include "core/string.h"
#include "core/vector.h"

namespace kw::FilesystemUtils {

Vector<uint8_t> read_file(MemoryResource& memory_resource, const String& relative_path);

} // namespace kw::FilesystemUtils
