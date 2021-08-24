#define PI 3.14159265
#define SAMPLE_COUNT 524288

// ((2 * PI) / STEP) * ((PI / 2) / STEP) = SAMPLE_COUNT =>
static const float STEP = PI / sqrt(SAMPLE_COUNT);

struct FS_INPUT {
    float4 screen_position : SV_POSITION;
    float4 world_position  : POSITION;
};

TextureCube cubemap_uniform_texture;
SamplerState sampler_uniform;

float4 main(FS_INPUT input) : SV_TARGET {
    float3 forward = normalize(input.world_position.xyz);
    float3 up = float3(0.0, 1.0, 0.0);
    float3 right = normalize(cross(up, forward));
    up = normalize(cross(forward, right));

    float3 irradiance = float3(0.0, 0.0, 0.0);

    // TODO: Integrate per pixel?
    for (float phi = 0.0; phi < 2.0 * PI; phi += STEP) {
        for (float theta = 0.0; theta < PI / 2.0; theta += STEP) {
            float3 local_space = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            float3 world_space = local_space.x * right + local_space.y * up + local_space.z * forward;
            irradiance += cubemap_uniform_texture.Sample(sampler_uniform, world_space).xyz * cos(theta) * sin(theta);
        }
    }

    return float4(PI * irradiance / SAMPLE_COUNT, 1.0);
}
