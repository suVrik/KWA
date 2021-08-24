cbuffer ReflectionProbeUniformBuffer {
    float4x4 view_projection;
    float4x4 inverse_view_projection;
    float3 view_position;
    float2 texel_size;
};

struct ReflectionProbePushConstants {
    float4 position;

    float4 aabbox_min;
    float4 aabbox_max;

    float4 radius_lod;
};

[[vk::push_constant]] ReflectionProbePushConstants reflection_probe_push_constants;

float4 main(float3 position : POSITION0) : SV_POSITION {
    return mul(view_projection, float4(position * reflection_probe_push_constants.radius_lod.x + reflection_probe_push_constants.position.xyz, 1.0));
}
