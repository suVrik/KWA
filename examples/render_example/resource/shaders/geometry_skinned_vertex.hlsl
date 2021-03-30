#define JOINT_COUNT 32

struct GS_INPUT {
    float4 position : POSITION;
    float4 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float4 texcoord : TEXCOORD;
    uint4 joints    : JOINTS;
    float4 weights  : WEIGHTS;
};

struct GS_OUTPUT {
    float4 position : SV_POSITION;
    float4 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float4 texcoord : TEXCOORD;
};

cbuffer GeometryData {
    float4x4 joint_data[JOINT_COUNT];
    float4x4 model_view_projection;
};

GS_OUTPUT main(GS_INPUT input) {
    // TODO: Apply transform.
    GS_OUTPUT output;
    output.position = input.position;
    output.normal = input.normal;
    output.tangent = input.tangent;
    output.texcoord = input.texcoord;
    return output;
}
