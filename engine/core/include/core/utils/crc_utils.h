#pragma once

#include <cstdint>

namespace kw::CrcUtils {

uint32_t crc32(uint32_t crc, const void* data, size_t size);
uint64_t crc64(uint64_t crc, const void* data, size_t size);

} // namespace kw::CrcUtils
