struct FS_INPUT {
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

float4 main(FS_INPUT input) : SV_TARGET {
    return input.color;
}
