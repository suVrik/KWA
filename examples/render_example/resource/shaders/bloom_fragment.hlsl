struct FS_INPUT {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

Texture2D downsampling_2_uniform_attachment;
SamplerState sampler_uniform;

struct BloomPushConstants {
    float4 transparency;
};

[[vk::push_constant]] BloomPushConstants bloom_push_constants;

float4 main(FS_INPUT input) : SV_TARGET {
    return float4(downsampling_2_uniform_attachment.Sample(sampler_uniform, input.texcoord).xyz * bloom_push_constants.transparency.x, 1.0);
}
