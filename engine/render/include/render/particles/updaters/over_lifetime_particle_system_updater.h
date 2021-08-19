#pragma once

#include "render/particles/updaters/particle_system_updater.h"
#include "render/particles/particle_system_stream.h"

#include <core/containers/vector.h>

namespace kw {

template <typename T>
class OverLifetimeParticleSystemUpdater : public ParticleSystemUpdater {
protected:
    OverLifetimeParticleSystemUpdater(Vector<float>&& inputs, Vector<T>&& outputs);

    template <ParticleSystemStream Stream, size_t Component>
    void update_stream(ParticleSystemPrimitive& primitive) const;

    Vector<float> m_inputs;
    Vector<T> m_outputs;
    size_t m_count;
};

} // namespace kw
