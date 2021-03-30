struct FS_INPUT {
    float4 position : SV_POSITION;
    float4 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float2 texcoord : TEXCOORD;
};

struct FS_OUTPUT {
    float4 albedo_ao_target          : SV_TARGET0;
    float4 normal_roughness_target   : SV_TARGET1;
    float4 emission_metalness_target : SV_TARGET2;
};

Texture2D albedo_ao_map;
Texture2D normal_roughness_map;
Texture2D emission_metalness_map;

SamplerState basic_sampler;

FS_OUTPUT main(FS_INPUT input) {
    float4 normal_roughness_sample = normal_roughness_map.Sample(basic_sampler, input.texcoord);

    float3 tangent_space_normal = normal_roughness_sample.xyz;
    float roughness = normal_roughness_sample.w;

    // TODO: Properly compute normal from input.
    float3 world_space_normal = tangent_space_normal;

    FS_OUTPUT output;
    output.albedo_ao_target = albedo_ao_map.Sample(basic_sampler, input.texcoord);
    output.normal_roughness_target = float4(world_space_normal, roughness);
    output.emission_metalness_target = emission_metalness_map.Sample(basic_sampler, input.texcoord);
    return output;
}
