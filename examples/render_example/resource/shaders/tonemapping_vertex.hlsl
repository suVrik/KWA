struct TonemappingData {
    float4x4 view_projection;
};

[[vk::push_constant]] TonemappingData tonemapping_data;

float4 main(float4 position : POSITION) : SV_POSITION {
    return position;
}
