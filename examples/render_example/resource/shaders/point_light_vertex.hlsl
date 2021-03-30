struct PointLightData {
    float4x4 model_view_projection;
    float4 intensity;
};

[[vk::push_constant]] PointLightData point_light_data;

float4 main(float4 position : POSITION) : SV_POSITION {
    // TODO: Apply transform.
    return position;
}
