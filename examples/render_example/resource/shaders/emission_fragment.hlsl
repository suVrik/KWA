struct FS_INPUT {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

Texture2D emission_ao_uniform_attachment;
Texture2D reflection_probe_uniform_attachment;
SamplerState sampler_uniform;

float4 main(FS_INPUT input) : SV_TARGET {
    float4 emission_ao_sample = emission_ao_uniform_attachment.SampleLevel(sampler_uniform, input.texcoord, 0.0);
    float4 reflection_probe_sample = reflection_probe_uniform_attachment.SampleLevel(sampler_uniform, input.texcoord, 0.0);

    float3 emission = emission_ao_sample.xyz;
    float3 ambient_occlusion = emission_ao_sample.a;

    float3 reflection_color = reflection_probe_sample.xyz;
    float reflection_weight = reflection_probe_sample.w + 1e-5;

    return float4(emission + reflection_color * ambient_occlusion / max(reflection_weight, 1.0), 1.0);
}
