#define PI 3.14159265
#define SAMPLE_COUNT 16384u

struct FS_INPUT {
    float4 screen_position : SV_POSITION;
    float4 world_position  : POSITION;
};

TextureCube cubemap_uniform_texture;
SamplerState sampler_uniform;

struct PrefilterPushConstants {
    float4x4 view_projection;
    float roughness;
};

[[vk::push_constant]] PrefilterPushConstants prefilter_push_constants;

float hammersley_y(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

float2 hammersley(uint i, uint n) {
    return float2(float(i) / float(n), hammersley_y(i));
}

float3 importance_sample_ggx(float2 random, float3 normal, float roughness) {
    float alpha = roughness * roughness;

    float phi = 2.0 * PI * random.x;
    float cos_theta = sqrt((1.0 - random.y) / (1.0 + (alpha * alpha - 1.0) * random.y));
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

    float x = cos(phi) * sin_theta;
    float y = sin(phi) * sin_theta;
    float z = cos_theta;

    float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, normal));
    float3 bitangent = cross(normal, tangent);

    return normalize(tangent * x + bitangent * y + normal * z);
}

float4 main(FS_INPUT input) : SV_TARGET {
    float3 normal_direction = normalize(input.world_position.xyz);
    float3 reflection_direction = normal_direction;
    float3 view_direction = reflection_direction;

    float total_weight = 0.0;
    float3 prefiltered_color = float3(0.0, 0.0, 0.0);

    for (uint i = 0u; i < SAMPLE_COUNT; i++) {
        float2 random = hammersley(i, SAMPLE_COUNT);
        float3 halfway_direction = importance_sample_ggx(random, normal_direction, prefilter_push_constants.roughness);
        float3 light_direction = reflect(-view_direction, halfway_direction);

        float n_dot_l = max(dot(normal_direction, light_direction), 0.0);
        if (n_dot_l > 0.0) {
            prefiltered_color += cubemap_uniform_texture.SampleLevel(sampler_uniform, light_direction, 0.0).xyz * n_dot_l;
            total_weight += n_dot_l;
        }
    }

    return float4(prefiltered_color / total_weight, 1.0);
}
