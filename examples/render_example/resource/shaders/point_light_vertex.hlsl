cbuffer LightUniformBuffer {
    float4x4 view_projection;
    float4x4 inverse_view_projection;
    float3 view_position;
    float2 texel_size;
};

struct PointLightPushConstants {
    float4 position;
    float4 luminance;

    // x goes for falloff radius.
    // y goes for light's Z near.
    // z goes for light's Z far.
    float4 radius_frustum;

    // x goes for normal bias.
    // y goes for perspective bias.
    // z goes for pcss radius.
    // w goes for pcss filter factor.
    float4 shadow_params;
};

[[vk::push_constant]] PointLightPushConstants point_light_push_constants;

float4 main(float3 position : POSITION) : SV_POSITION {
    return mul(view_projection, float4(position * point_light_push_constants.radius_frustum.x + point_light_push_constants.position.xyz, 1.0));
}
