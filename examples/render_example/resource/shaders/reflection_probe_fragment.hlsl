#define PI 3.14159265

Texture2D albedo_metalness_uniform_attachment;
Texture2D normal_roughness_uniform_attachment;
Texture2D depth_uniform_attachment;

TextureCube irradiance_uniform_texture;
TextureCube prefilter_uniform_texture;
Texture2D brdf_lookup_uniform_texture;

SamplerState sampler_uniform;

cbuffer ReflectionProbeUniformBuffer {
    float4x4 view_projection;
    float4x4 inverse_view_projection;
    float3 view_position;
    float2 texel_size;
};

struct ReflectionProbePushConstants {
    float4 position;

    float4 aabbox_min;
    float4 aabbox_max;

    float4 radius_lod;
};

[[vk::push_constant]] ReflectionProbePushConstants reflection_probe_push_constants;

float3 specular_f(float v_dot_h, float3 f0, float inverse_roughness) {
    return f0 + (max(float3(inverse_roughness, inverse_roughness, inverse_roughness), f0) - f0) * pow(1.0 - v_dot_h, 5.0);
}

float falloff(float distance, float falloff_radius) {
    float factor = distance / falloff_radius;
    float sqr_factor = factor * factor;
    float quad_factor = sqr_factor * sqr_factor;
    float result = saturate(1.0 - quad_factor * quad_factor);
    float sqr_result = result * result;
    return sqr_result;
}

float4 main(float4 position : SV_POSITION) : SV_TARGET {
    //
    // Sample G-buffer.
    //

    float2 texcoord = position.xy * texel_size;
    float2 screen_position = texcoord.xy * float2(2.0, -2.0) - float2(1.0, -1.0);

    float4 albedo_metalness_sample = albedo_metalness_uniform_attachment.Sample(sampler_uniform, texcoord);
    float4 normal_roughness_sample = normal_roughness_uniform_attachment.Sample(sampler_uniform, texcoord);
    float4 depth_sample = depth_uniform_attachment.Sample(sampler_uniform, texcoord);

    float3 albedo = albedo_metalness_sample.rgb;
    float metalness = albedo_metalness_sample.a;
    float3 normal_direction = normal_roughness_sample.rgb;
    float roughness = normal_roughness_sample.a;
    float depth = depth_sample.r;

    //
    // Reconstruct clip position and view direction.
    //

    float4 clip_position = mul(inverse_view_projection, float4(screen_position.x, screen_position.y, depth, 1.0));
    clip_position /= clip_position.w;

    float3 view_direction = normalize(view_position - clip_position.xyz);

    float n_dot_v = saturate(dot(normal_direction, view_direction));

    //
    // Compute parallax corrected reflection direction.
    //

    float3 reflection_direction = reflect(-view_direction, normal_direction);
    float3 first_plane_intersection = (reflection_probe_push_constants.aabbox_max.xyz - clip_position.xyz) / reflection_direction;
    float3 second_plane_intersection = (reflection_probe_push_constants.aabbox_min.xyz - clip_position.xyz) / reflection_direction;
    float3 furthest_plane = max(first_plane_intersection, second_plane_intersection);
    float intersection_distance = min(furthest_plane.x, min(furthest_plane.y, furthest_plane.z));
    float3 intersection_position = clip_position.xyz + reflection_direction * intersection_distance;
    reflection_direction = intersection_position - reflection_probe_push_constants.position.xyz;

    //
    // Compute diffuse part.
    //

    float3 f0 = lerp(float3(0.04, 0.04, 0.04), albedo, metalness);
    float3 f = specular_f(n_dot_v, f0, 1.0 - roughness);
    float3 irradiance = irradiance_uniform_texture.SampleLevel(sampler_uniform, normal_direction, 0.0).rgb;
    float3 diffuse = (1.0 - f) * (1.0 - metalness) * irradiance * albedo;

    //
    // Compute specular part.
    //

    float lod_count = reflection_probe_push_constants.radius_lod.y;
    float3 prefiltered_color = prefilter_uniform_texture.SampleLevel(sampler_uniform, reflection_direction, roughness * lod_count).rgb;
    float2 brdf = brdf_lookup_uniform_texture.SampleLevel(sampler_uniform, float2(n_dot_v, roughness), 0.0).xy;
    float3 specular = prefiltered_color * (f * brdf.x + brdf.y);

    //
    // Compute weight.
    //

    float distance = length(clip_position.xyz - reflection_probe_push_constants.position.xyz);
    float falloff_radius = reflection_probe_push_constants.radius_lod.x;
    float weight = falloff(distance, falloff_radius);

    return float4(diffuse + specular, 1.0) * weight;
}
