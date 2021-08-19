#include "render/particles/particle_system_random.h"

namespace kw {

ParticleSystemRandom& ParticleSystemRandom::instance() {
    static ParticleSystemRandom random(1890424906);
    return random;
}

ParticleSystemRandom::ParticleSystemRandom(int seed_)
    : seed(seed_)
{
}

} // namespace kw
