#define GAMMA 2.2

struct FS_INPUT {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

Texture2D lighting_uniform_attachment;
SamplerState sampler_uniform;

float3 filmic_tonemapping(float3 color) {
    color = max(color - 0.004, 0.0);
    return (color * (color * 6.2 + 0.5)) / (color * (color * 6.2 + 1.7) + 0.06);
}

float3 uncharted_tonemapping(float3 color) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    float exposure = 2.0;
    color *= exposure;
    color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
    float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
    color /= white;
    color = pow(color, 1.0 / GAMMA);
    return color;
}

float3 raw_tonemapping(float3 color) {
    return color;
}

float4 main(FS_INPUT input) : SV_TARGET {
    return float4(uncharted_tonemapping(lighting_uniform_attachment.Sample(sampler_uniform, input.texcoord).xyz), 1.0);
}
