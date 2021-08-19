#pragma once

#include <core/utils/enum_utils.h>

namespace kw {

enum class ParticleSystemStreamMask {
    NONE                 = 0,
    POSITION_X           = 1 << 0,
    POSITION_Y           = 1 << 1,
    POSITION_Z           = 1 << 2,
    ROTATION             = 1 << 3,
    GENERATED_SCALE_X    = 1 << 4,
    GENERATED_SCALE_Y    = 1 << 5,
    GENERATED_SCALE_Z    = 1 << 6,
    SCALE_X              = 1 << 7,
    SCALE_Y              = 1 << 8,
    SCALE_Z              = 1 << 9,
    GENERATED_VELOCITY_X = 1 << 10,
    GENERATED_VELOCITY_Y = 1 << 11,
    GENERATED_VELOCITY_Z = 1 << 12,
    VELOCITY_X           = 1 << 13,
    VELOCITY_Y           = 1 << 14,
    VELOCITY_Z           = 1 << 15,
    COLOR_R              = 1 << 16,
    COLOR_G              = 1 << 17,
    COLOR_B              = 1 << 18,
    COLOR_A              = 1 << 19,
    FRAME                = 1 << 20,
    TOTAL_LIFETIME       = 1 << 21,
    CURRENT_LIFETIME     = 1 << 22,
};

KW_DEFINE_ENUM_BITMASK(ParticleSystemStreamMask);

} // namespace kw
