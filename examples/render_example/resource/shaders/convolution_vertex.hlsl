struct VS_OUTPUT {
    float4 screen_position : SV_POSITION;
    float4 world_position  : POSITION;
};

struct ConvolutionPushConstants {
    float4x4 view_projection;
};

[[vk::push_constant]] ConvolutionPushConstants convolution_push_constants;

VS_OUTPUT main(float3 position : POSITION) {
    VS_OUTPUT output;
    output.world_position = float4(position, 1.0);
    output.screen_position = mul(convolution_push_constants.view_projection, output.world_position);
    return output;
}
