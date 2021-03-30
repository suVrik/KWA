Texture2D lighting_map;
SamplerState basic_sampler;

struct TonemappingData {
    float4x4 view_projection;
};

[[vk::push_constant]] TonemappingData tonemapping_data;

float4 main(float4 position : SV_POSITION) : SV_TARGET {
    // TODO: Actually perform tonemapping.
    return lighting_map.Sample(basic_sampler, position.xy);
}
