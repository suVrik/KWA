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

    float4 inverse_transpose_model_row0 : POSITION5;
    float4 inverse_transpose_model_row1 : POSITION6;
    float4 inverse_transpose_model_row2 : POSITION7;
    float4 inverse_transpose_model_row3 : POSITION8;
};

struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float3 binormal : BINORMAL;
    float2 texcoord : TEXCOORD;
};

struct GeometryPushConstants {
    float4x4 view_projection;
};

[[vk::push_constant]] GeometryPushConstants geometry_push_constants;

VS_OUTPUT main(VS_INPUT input) {
    float4x4 model = float4x4(input.model_row0,
                              input.model_row1,
                              input.model_row2,
                              input.model_row3);

    float4x4 inverse_transpose_model = float4x4(input.inverse_transpose_model_row0,
                                                input.inverse_transpose_model_row1,
                                                input.inverse_transpose_model_row2,
                                                input.inverse_transpose_model_row3);

    VS_OUTPUT output;
    output.position = mul(geometry_push_constants.view_projection, mul(float4(input.position, 1.0), model));
    output.normal   = normalize(mul(float4(input.normal, 0.0), inverse_transpose_model).xyz);
    output.tangent  = normalize(mul(float4(input.tangent.xyz, 0.0), model).xyz);
    output.binormal = normalize(mul(float4(cross(input.normal, input.tangent.xyz) * input.tangent.w, 0.0), model).xyz);

    //float scale_x = length(float3(model[0][0], model[1][0], model[2][0]));
    //float scale_y = length(float3(model[0][1], model[1][1], model[2][1]));
    //float scale_z = length(float3(model[0][2], model[1][2], model[2][2]));
    float scale_x = length(model[0].xyz);
    float scale_y = length(model[1].xyz);
    float scale_z = length(model[2].xyz);

    if (input.normal.x == 1.0) {
        output.texcoord = float2(input.texcoord.x * scale_z, (input.texcoord.y - 1.0) * scale_y);
    } else if (input.normal.x == -1.0) {
        output.texcoord = float2((input.texcoord.x - 1.0) * scale_z, (input.texcoord.y - 1.0) * scale_y);
    } else if (input.normal.y == 1.0) {
        output.texcoord = float2((input.texcoord.x - 1.0) * scale_x, (input.texcoord.y - 1.0) * scale_z);
    } else if (input.normal.y == -1.0) {
        output.texcoord = float2(input.texcoord.x * scale_x, (input.texcoord.y - 1.0) * scale_z);
    } else if (input.normal.z == 1.0) {
        output.texcoord = float2(input.texcoord.x * scale_x, (input.texcoord.y - 1.0) * scale_y);
    } else {
        output.texcoord = float2((input.texcoord.x - 1.0) * scale_x, (input.texcoord.y - 1.0) * scale_y);
    }

    return output;
}
