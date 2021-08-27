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
    float2 texcoord : TEXCOORD;
};

cbuffer ShadowUniformBuffer {
    float4x4 joint_data[32];
};

struct ShadowPushConstants {
    float4x4 model_view_projection;
};

[[vk::push_constant]] ShadowPushConstants shadow_push_constants;

VS_OUTPUT main(VS_INPUT input) {
    float4x4 skinning = input.weights.x * joint_data[input.joints.x] +
                        input.weights.y * joint_data[input.joints.y] +
                        input.weights.z * joint_data[input.joints.z] +
                        input.weights.w * joint_data[input.joints.w];

    VS_OUTPUT output;
    output.position = mul(shadow_push_constants.model_view_projection, mul(skinning, float4(input.position, 1.0)));
    output.texcoord = input.texcoord;
    return output;
}
