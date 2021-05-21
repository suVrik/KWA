#pragma once

#include "core/containers/vector.h"

namespace kw::FilesystemUtils {

bool file_exists(const char* relative_path);

Vector<uint8_t> read_file(MemoryResource& memory_resource, const char* relative_path);

Vector<uint8_t> read_file(MemoryResource& memory_resource, const char* relative_path, size_t max_size);

} // namespace kw::FilesystemUtils
