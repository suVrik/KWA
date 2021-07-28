struct VS_INPUT {
    float3 position : POSITION;
    float4 color    : COLOR;
};

struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

struct DebugDrawPushConstants {
    float4x4 view_projection;
};

[[vk::push_constant]] DebugDrawPushConstants debug_draw_push_constants;

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;
    output.position = mul(debug_draw_push_constants.view_projection, float4(input.position, 1.0));
    output.color    = input.color;
    return output;
}
