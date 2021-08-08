struct VS_INPUT {
    float3 position : POSITION0;
};

struct VS_OUTPUT {
    float4 position : SV_POSITION;
};

cbuffer LightUniformBuffer {
    float4x4 view_projection;
    float4x4 inverse_view_projection;
    float4 view_position;
    float2 texel_size;
    float2 specular_diffuse;
};

struct SphereLightPushConstants {
    float4 position;
    float4 luminance;

    // x goes for light radius.
    // y goes for attenuation radius.
    // z goes for light's Z near.
    // w goes for light's Z far.
    float4 radius_frustum;

    // x goes for normal bias.
    // y goes for perspective bias.
    // z goes for pcss radius factor.
    // w goes for pcss filter factor.
    float4 shadow_params;
};

[[vk::push_constant]] SphereLightPushConstants sphere_light_push_constants;

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT result;
    result.position = mul(view_projection, float4(input.position * sphere_light_push_constants.radius_frustum.y + sphere_light_push_constants.position.xyz, 1.0));
    return result;
}
