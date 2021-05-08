#pragma once

#include "core/string.h"
#include "core/vector.h"

namespace kw::FilesystemUtils {

bool file_exists(const String& relative_path);

Vector<uint8_t> read_file(MemoryResource& memory_resource, const String& relative_path);

} // namespace kw::FilesystemUtils
