struct FS_INPUT {
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float3 binormal : BINORMAL;
    float2 texcoord : TEXCOORD;
};

Texture2D color_uniform;
SamplerState sampler_uniform;

float4 main(FS_INPUT input) : SV_TARGET {
    return color_uniform.Sample(sampler_uniform, input.texcoord);
}
