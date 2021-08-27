struct FS_INPUT {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

Texture2D albedo_alpha_uniform_texture;
SamplerState sampler_uniform;

float4 main(FS_INPUT input) : SV_TARGET {
    float4 albedo_alpha_sample = albedo_alpha_uniform_texture.Sample(sampler_uniform, input.texcoord);
    if (albedo_alpha_sample.a <= 0.5) {
        discard;
    }
    return 1.0;
}
