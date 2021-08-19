#pragma once

#include <core/math/float4.h>

#include <emmintrin.h>
#include <smmintrin.h>

namespace kw {

class ParticleSystemRandom {
public:
    static ParticleSystemRandom& instance();

    explicit ParticleSystemRandom(int seed_);

    float rand_float() {
        seed *= 16807;

        // Generate float number in range [1, 2].
        int x = 0x3F800000 | (seed & 0x00FFFFFF);

        // Convert to range [0, 1].
        return *reinterpret_cast<float*>(&x) - 1.f;
    }

    float3 rand_float3() {
        return float3(rand_float(), rand_float(), rand_float());
    }

    float4 rand_float4() {
        return float4(rand_float(), rand_float(), rand_float(), rand_float());
    }

    __m128 rand_simd3() {
        __m128i seed_xmm = _mm_mullo_epi32(_mm_set1_epi32(seed), _mm_set_epi32(0, 1622647863, 282475249, 16807));

        // 16807^3
        seed *= 1622647863;

        // Generate float number in range [1, 2]. The fourh component is 1.
        __m128i x_xmm = _mm_or_si128(_mm_set1_epi32(0x3F800000), _mm_and_si128(seed_xmm, _mm_set1_epi32(0x00FFFFFF)));

        // Convert to range [0, 1]. The fourth component is 0.
        return _mm_add_ps(_mm_castsi128_ps(x_xmm), _mm_set1_ps(-1.f));
    }

    __m128 rand_simd4() {
        __m128i seed_xmm = _mm_mullo_epi32(_mm_set1_epi32(seed), _mm_set_epi32(-1199696159, 1622647863, 282475249, 16807));

        // 16807^4
        seed *= -1199696159;

        // Generate float number in range [1, 2].
        __m128i x_xmm = _mm_or_si128(_mm_set1_epi32(0x3F800000), _mm_and_si128(seed_xmm, _mm_set1_epi32(0x00FFFFFF)));

        // Convert to range [0, 1].
        return _mm_add_ps(_mm_castsi128_ps(x_xmm), _mm_set1_ps(-1.f));
    }
    
    int seed;
};

} // namespace kw
