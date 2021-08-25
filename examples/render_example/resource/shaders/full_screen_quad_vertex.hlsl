struct VS_INPUT {
    float2 position : POSITION;
    float2 texcoord : TEXCOORD;
};

struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT result;
    result.position = float4(input.position, 0.0, 1.0);
    result.texcoord = input.texcoord;
    return result;
}
