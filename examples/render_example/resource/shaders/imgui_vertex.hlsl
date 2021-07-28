struct VS_INPUT {
    float2 position : POSITION;
    float2 texcoord : TEXCOORD;
    float4 color    : COLOR;
};

struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float4 color    : COLOR;
};

struct ImguiPushConstants {
    float2 scale;
    float2 translate;
};

[[vk::push_constant]] ImguiPushConstants imgui_push_constants;

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;
    output.position = float4(input.position * imgui_push_constants.scale + imgui_push_constants.translate, 0.0, 1.0);
    output.texcoord = input.texcoord;
    output.color    = input.color;
    return output;
}
