struct VS_INPUT {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float2 texcoord : TEXCOORD;
    uint4 joints    : JOINTS;
    float4 weights  : WEIGHTS;
};

struct VS_OUTPUT {
    float4 position : SV_POSITION;
};

cbuffer ShadowUniformBuffer {
    float4x4 joint_data[32];
};

struct ShadowPushConstants {
    float4x4 view_projection;
};

[[vk::push_constant]] ShadowPushConstants shadow_push_constants;

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;
    output.position = mul(shadow_push_constants.view_projection, float4(input.position, 1.0));
    return output;
}
