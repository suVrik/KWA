struct FS_INPUT {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float4 color    : COLOR;
};

Texture2D albedo_alpha_uniform_texture;
SamplerState sampler_uniform;

float4 main(FS_INPUT input) : SV_TARGET {
    return input.color * albedo_alpha_uniform_texture.Sample(sampler_uniform, input.texcoord);
}
