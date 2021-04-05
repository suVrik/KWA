#pragma once

#include "core/string.h"
#include "core/vector.h"

namespace kw::FilesystemUtils {

Vector<std::byte> read_file(MemoryResource& memory_resource, const String& relative_path);

} // namespace kw::FilesystemUtils
