#pragma once

namespace kw {

enum class ParticleSystemStream {
    POSITION_X,
    POSITION_Y,
    POSITION_Z,
    ROTATION,
    GENERATED_SCALE_X,
    GENERATED_SCALE_Y,
    GENERATED_SCALE_Z,
    SCALE_X,
    SCALE_Y,
    SCALE_Z,
    GENERATED_VELOCITY_X,
    GENERATED_VELOCITY_Y,
    GENERATED_VELOCITY_Z,
    VELOCITY_X,
    VELOCITY_Y,
    VELOCITY_Z,
    COLOR_R,
    COLOR_G,
    COLOR_B,
    COLOR_A,
    FRAME,
    TOTAL_LIFETIME,
    CURRENT_LIFETIME,
};

constexpr size_t PARTICLE_SYSTEM_STREAM_COUNT = 23;

} // namespace kw
