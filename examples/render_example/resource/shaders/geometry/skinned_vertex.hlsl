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
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float3 binormal : BINORMAL;
    float2 texcoord : TEXCOORD;
};

cbuffer GeometryUniform {
    float4x4 model;
    float4x4 inverse_transpose_model;
    float4x4 joint_data[32];
};

struct GeometryPushConstants {
    float4x4 view_projection;
};

[[vk::push_constant]] GeometryPushConstants geometry_push_constants;

VS_OUTPUT main(VS_INPUT input) {
    float4x4 skinning = input.weights.x * joint_data[input.joints.x] +
                        input.weights.y * joint_data[input.joints.y] +
                        input.weights.z * joint_data[input.joints.z] +
                        input.weights.w * joint_data[input.joints.w];

    float4x4 skinned_model = mul(model, skinning);
    float4x4 skinned_inverse_transpose_model = mul(inverse_transpose_model, skinning);

    VS_OUTPUT output;
    output.position = mul(geometry_push_constants.view_projection, mul(skinned_model, float4(input.position, 1.0)));
    output.normal = normalize(mul(skinned_inverse_transpose_model, float4(input.normal, 0.0)).xyz);
    output.tangent = normalize(mul(skinned_model, float4(input.tangent.xyz, 0.0)).xyz);
    output.binormal = normalize(mul(skinned_model, float4(cross(input.normal, input.tangent.xyz) * input.tangent.w, 0.0)).xyz);
    output.texcoord = input.texcoord;
    return output;
}
