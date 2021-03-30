struct GS_INPUT {
	// Vertex data.
    float4 position : POSITION;
    float4 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float2 texcoord : TEXCOORD;

    // Instancing data.
    float4 model_row0 : POSITION1;
    float4 model_row1 : POSITION2;
    float4 model_row2 : POSITION3;
    float4 model_row3 : POSITION4;
};

struct GS_OUTPUT {
    float4 position : SV_POSITION;
    float4 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float2 texcoord : TEXCOORD;
};

struct GeometryData {
    float4x4 view_projection;
};

[[vk::push_constant]] GeometryData geometry_data;

GS_OUTPUT main(GS_INPUT input) {
    // TODO: Apply transform.
    GS_OUTPUT output;
    output.position = input.position;
    output.normal = input.normal;
    output.tangent = input.tangent;
    output.texcoord = input.texcoord;
    return output;
}
