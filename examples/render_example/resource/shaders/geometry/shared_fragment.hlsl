struct FS_INPUT {
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float3 binormal : BINORMAL;
    float2 texcoord : TEXCOORD;
};

struct FS_OUTPUT {
    float4 albedo_metalness_target : SV_TARGET0;
    float4 normal_roughness_target : SV_TARGET1;
    float4 emission_ao_target      : SV_TARGET2;
};

Texture2D albedo_alpha_uniform_attachment;
Texture2D normal_uniform_attachment;
Texture2D ao_roughness_metalness_uniform_attachment;
Texture2D emission_uniform_attachment;

SamplerState sampler_uniform;

FS_OUTPUT main(FS_INPUT input) {
    float4 albedo_alpha_sample = albedo_alpha_uniform_attachment.Sample(sampler_uniform, input.texcoord);
    if (albedo_alpha_sample.w <= 0.5) {
        discard;
    }

    float4 normal_sample = normal_uniform_attachment.Sample(sampler_uniform, input.texcoord);
    float4 ao_roughness_metalness_sample = ao_roughness_metalness_uniform_attachment.Sample(sampler_uniform, input.texcoord);
    float4 emission_sample = emission_uniform_attachment.Sample(sampler_uniform, input.texcoord);

    float3x3 tangent_space = float3x3(input.tangent, input.binormal, input.normal);

    float3 normal;
    normal.xy = normal_sample.xy * 2.0 - 1.0;
    normal.z = sqrt(1.0 - clamp(dot(normal.xy, normal.xy), 0.0, 1.0));
    normal = normalize(mul(normal, tangent_space));

    FS_OUTPUT output;
    output.albedo_metalness_target = float4(albedo_alpha_sample.xyz, ao_roughness_metalness_sample.z);
    output.normal_roughness_target = float4(normal, ao_roughness_metalness_sample.y);
    output.emission_ao_target = float4(emission_sample.xyz, ao_roughness_metalness_sample.x);
    return output;
}
