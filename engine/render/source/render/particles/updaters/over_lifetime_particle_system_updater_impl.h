#pragma once

#include "render/particles/updaters/over_lifetime_particle_system_updater.h"

#include <core/debug/assert.h>

#include <emmintrin.h>
#include <immintrin.h>

namespace kw {

template <typename T>
inline OverLifetimeParticleSystemUpdater<T>::OverLifetimeParticleSystemUpdater(Vector<float>&& inputs, Vector<T>&& outputs)
    : m_inputs(std::move(inputs))
    , m_outputs(std::move(outputs))
    , m_count(m_inputs.size())
{
    KW_ASSERT(m_count > 0);
    KW_ASSERT(m_inputs.front() == 0.f);
    KW_ASSERT(m_inputs.back() == 1.f);
    KW_ASSERT(m_outputs.size() == m_count);
}

template <typename T>
template <ParticleSystemStream Stream, size_t Component>
inline void OverLifetimeParticleSystemUpdater<T>::update_stream(ParticleSystemPrimitive& primitive) const {
    size_t particle_count = primitive.get_particle_count();

    float* total_lifetime_stream = primitive.get_particle_system_stream(ParticleSystemStream::TOTAL_LIFETIME);
    KW_ASSERT(total_lifetime_stream != nullptr);

    float* current_lifetime_stream = primitive.get_particle_system_stream(ParticleSystemStream::CURRENT_LIFETIME);
    KW_ASSERT(current_lifetime_stream != nullptr);

    __m128 zero_xmm = _mm_set1_ps(0.f);

    float* color_x_stream = primitive.get_particle_system_stream(Stream);
    KW_ASSERT(color_x_stream != nullptr);

    for (size_t i = 0; i < particle_count; i += 4) {
        __m128 total_lifetime_xmm = _mm_load_ps(total_lifetime_stream + i);
        __m128 current_lifetime_xmm = _mm_load_ps(current_lifetime_stream + i);
        __m128 input_xmm = _mm_div_ps(current_lifetime_xmm, total_lifetime_xmm);

        __m128 previous_input_xmm = _mm_set1_ps(m_inputs.front());
        __m128 previous_output_xmm = _mm_set1_ps((&m_outputs.front())[Component]);
        __m128 output_xmm = previous_output_xmm;

        for (size_t j = 1; j < m_count; j++) {
            __m128 current_input_xmm = _mm_set1_ps(m_inputs[j]);
            __m128 current_output_xmm = _mm_set1_ps((&m_outputs[j])[Component]);

            __m128 relative_input_xmm = _mm_div_ps(_mm_sub_ps(input_xmm, previous_input_xmm), _mm_sub_ps(current_input_xmm, previous_input_xmm));
            __m128 temp_output_xmm = _mm_fmadd_ps(_mm_sub_ps(current_output_xmm, previous_output_xmm), relative_input_xmm, previous_output_xmm);
            __m128 mask_xmm = _mm_cmp_ps(relative_input_xmm, zero_xmm, _CMP_GE_OQ);

            output_xmm = _mm_or_ps(_mm_andnot_ps(mask_xmm, output_xmm), _mm_and_ps(mask_xmm, temp_output_xmm));
            previous_output_xmm = current_output_xmm;
            previous_input_xmm = current_input_xmm;
        }

        _mm_store_ps(color_x_stream + i, output_xmm);
    }
}

} // namespace kw
