#pragma once

#include <cstdint>

#if defined(__linux__)
#include <endian.h>
#if __BYTE_ORDER == __BIG_ENDIAN
#define KW_BIG_ENDIAN
#else
#define KW_LITTLE_ENDIAN
#endif
#else
#if defined(__hppa__) || defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || defined(__sparc__) || \
    (defined(__MIPS__) && defined(__MISPEB__)) || defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC)
#define KW_BIG_ENDIAN
#else
#define KW_LITTLE_ENDIAN
#endif
#endif

namespace kw::EndianUtils {

// Host to/from Little Endian.
uint16_t swap_le(uint16_t value);
int16_t swap_le(int16_t value);
uint32_t swap_le(uint32_t value);
int32_t swap_le(int32_t value);
uint64_t swap_le(uint64_t value);
int64_t swap_le(int64_t value);
float swap_le(float value);
double swap_le(double value);

// Host to/from Big Endian.
uint16_t swap_be(uint16_t value);
int16_t swap_be(int16_t value);
uint32_t swap_be(uint32_t value);
int32_t swap_be(int32_t value);
uint64_t swap_be(uint64_t value);
int64_t swap_be(int64_t value);
float swap_be(float value);
double swap_be(double value);

} // namespace kw::EndianUtils
