#define SAMPLE_COUNT 13u
#define BOX_COUNT 5u

struct FS_INPUT {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

Texture2D input_uniform_attachment;
SamplerState sampler_uniform;

struct DownsamplingPushConstants {
    float4 texel_size;
};

[[vk::push_constant]] DownsamplingPushConstants downsampling_push_constants;

static const float2 SAMPLES[SAMPLE_COUNT] = {
    float2(-1.0, -1.0),
    float2( 0.0, -1.0),
    float2( 1.0, -1.0),
    float2(-0.5, -0.5),
    float2( 0.5, -0.5),
    float2(-1.0,  0.0),
    float2( 0.0,  0.0),
    float2( 1.0,  0.0),
    float2(-0.5,  0.5),
    float2( 0.5,  0.5),
    float2(-1.0,  1.0),
    float2( 0.0,  1.0),
    float2( 1.0,  1.0),
};

struct Box {
    uint4 samples;
    float weight;
};

static const Box BOXES[BOX_COUNT] = {
    { uint4(0, 1,  5,  6), 0.125 },
    { uint4(1, 2,  6,  7), 0.125 },
    { uint4(3, 4,  8,  9),   0.5 },
    { uint4(6, 7, 11, 12), 0.125 },
    { uint4(7, 8, 12, 13), 0.125 },
};

float rgb_to_luma(float3 color) {
    return dot(color, float3(0.299, 0.587, 0.114));
}

float4 main(FS_INPUT input) : SV_TARGET {
    float3 samples[SAMPLE_COUNT];

    for (uint i = 0; i < SAMPLE_COUNT; i++) {
        samples[i] = input_uniform_attachment.Sample(sampler_uniform, input.texcoord + SAMPLES[i].xy * downsampling_push_constants.texel_size.xy).xyz;
    }

    float3 result = 0.0;
    
    for (uint i = 0; i < BOX_COUNT; i++) {
        float4 sum = 0.0;
        for (uint j = 0; j < 4; j++) {
            float3 color = samples[BOXES[i].samples[j]];
            float luma = rgb_to_luma(color);
            sum += rcp(1.0 + luma) * float4(color, 1.0);
        }
        result += sum.xyz / sum.w;
    }

    return float4(result, 1.0);
}
