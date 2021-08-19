struct VS_INPUT {
    //
    // Vertex data.
    //

    float3 position : POSITION;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float2 texcoord : TEXCOORD;

    //
    // Instance data.
    //

    float4 model_row0 : POSITION1;
    float4 model_row1 : POSITION2;
    float4 model_row2 : POSITION3;
    float4 model_row3 : POSITION4;
};

struct VS_OUTPUT {
    float4 position : SV_POSITION;
};

struct ShadowPushConstants {
    float4x4 view_projection;
};

[[vk::push_constant]] ShadowPushConstants shadow_push_constants;

VS_OUTPUT main(VS_INPUT input) {
    float4x4 model = float4x4(input.model_row0, input.model_row1, input.model_row2, input.model_row3);

    VS_OUTPUT output;
    output.position = mul(shadow_push_constants.view_projection, mul(float4(input.position, 1.0), model));
    return output;
}
