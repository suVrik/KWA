Texture2D shadow_map;
Texture2D albedo_ao_map;
Texture2D normal_roughness_map;
Texture2D emission_metalness_map;
Texture2D depth_map;

SamplerState basic_sampler;
SamplerState shadow_sampler;

struct PointLightData {
    float4x4 model_view_projection;
    float4 intensity;
};

[[vk::push_constant]] PointLightData point_light_data;

float4 main(float4 position : SV_POSITION) : SV_TARGET {
    // TODO: Actually perform lighting.
    return float4(albedo_ao_map.Sample(basic_sampler, position.xy).xyz, 1.0);
}
