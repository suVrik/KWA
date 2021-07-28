struct FS_INPUT {
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
    float4 color    : COLOR;
};

Texture2D texture_uniform;
SamplerState sampler_uniform;

float4 main(FS_INPUT input) : SV_TARGET {
    return input.color * texture_uniform.Sample(sampler_uniform, input.uv);
}
