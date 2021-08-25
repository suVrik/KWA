#define PI 3.14159265

Texture2D albedo_metalness_uniform_attachment;
Texture2D normal_roughness_uniform_attachment;
Texture2D depth_uniform_attachment;

TextureCube opaque_shadow_uniform_texture;
TextureCube translucent_shadow_uniform_texture;
Texture3D pcf_rotation_uniform_texture;

SamplerState sampler_uniform;
SamplerComparisonState shadow_sampler_uniform;

cbuffer LightUniformBuffer {
    float4x4 view_projection;
    float4x4 inverse_view_projection;
    float3 view_position;
    float2 texel_size;
};

struct PointLightPushConstants {
    float4 position;
    float4 luminance;

    // x goes for falloff radius.
    // y goes for light's Z near.
    // z goes for light's Z far.
    float4 radius_frustum;

    // x goes for normal bias.
    // y goes for perspective bias.
    // z goes for pcss radius.
    // w goes for pcss filter factor.
    float4 shadow_params;
};

[[vk::push_constant]] PointLightPushConstants point_light_push_constants;

static const float2 POISSON_DISK[16] = {
    float2(-0.94201624, -0.39906216),
    float2( 0.94558609, -0.76890725),
    float2(-0.09418410, -0.92938870),
    float2( 0.34495938,  0.29387760),
    float2(-0.91588581,  0.45771432),
    float2(-0.81544232, -0.87912464),
    float2(-0.38277543,  0.27676845),
    float2( 0.97484398,  0.75648379),
    float2( 0.44323325, -0.97511554),
    float2( 0.53742981, -0.47373420),
    float2(-0.26496911, -0.41893023),
    float2( 0.79197514,  0.19090188),
    float2(-0.24188840,  0.99706507),
    float2(-0.81409955,  0.91437590),
    float2( 0.19984126,  0.78641367),
    float2( 0.14383161, -0.14100790)
};

float3 pcss(float3 light_position, float3 clip_position, float3 clip_normal) {
    float z_near = point_light_push_constants.radius_frustum.y;
    float z_far = point_light_push_constants.radius_frustum.z;

    float normal_bias = point_light_push_constants.shadow_params.x;
    float perspective_bias = point_light_push_constants.shadow_params.y;
    float light_radius = point_light_push_constants.shadow_params.z;
    float filter_factor = point_light_push_constants.shadow_params.w;

    float3 light_clip_vector = clip_position + clip_normal * normal_bias - light_position;

    float3 abs_light_clip_vector = abs(light_clip_vector);
    float linear_depth = max(abs_light_clip_vector.x, max(abs_light_clip_vector.y, abs_light_clip_vector.z));
    float non_linear_depth = z_far / (z_far - z_near) + (z_far * z_near) / (linear_depth * (z_near - z_far));

    float3 disk_basis_z = normalize(light_clip_vector);
    float3 temp = normalize(cross(disk_basis_z, float3(0.24188840, 0.90650779, 0.34602550)));
    float3 disk_basis_x = cross(disk_basis_z, temp);
    float3 disk_basis_y = cross(disk_basis_z, disk_basis_x);

    float2 rotation = pcf_rotation_uniform_texture.Sample(sampler_uniform, clip_position * 100.0).xy;

    float light_frustum_width = z_far * 2.0;
    float light_radius_uv = light_radius / light_frustum_width;
    float search_radius = light_radius_uv * (linear_depth - z_near) / linear_depth;

    float sum = 0.0;
    int count = 0;

    for (int i = 0; i < 16; i++) {
        float poisson_disk_x = POISSON_DISK[i].x * rotation.x - POISSON_DISK[i].y * rotation.y;
        float poisson_disk_y = POISSON_DISK[i].y * rotation.y + POISSON_DISK[i].y * rotation.x;

        float3 uv = light_clip_vector + search_radius * (disk_basis_x * poisson_disk_x + disk_basis_y * poisson_disk_y);

        float non_linear_blocker_depth = opaque_shadow_uniform_texture.SampleLevel(sampler_uniform, uv, 0.0).r;
        if (non_linear_blocker_depth < non_linear_depth - perspective_bias) {
            sum += non_linear_blocker_depth;
            count++;
        }
    }

    float3 translucent = translucent_shadow_uniform_texture.SampleLevel(sampler_uniform, light_clip_vector, 0.0).xyz;

    if (count == 0) {
        return translucent;
    }

    float average_linear_blocker_depth = z_far * z_near / (z_far - sum / count * (z_far - z_near));
    float filter_radius = search_radius * saturate((linear_depth - average_linear_blocker_depth) * filter_factor);

    sum = 0;

    for (int i = 0; i < 16; i++) {
        float poisson_disk_x = POISSON_DISK[i].x * rotation.x - POISSON_DISK[i].y * rotation.y;
        float poisson_disk_y = POISSON_DISK[i].y * rotation.y + POISSON_DISK[i].y * rotation.x;

        float3 uv = light_clip_vector + filter_radius * (disk_basis_x * poisson_disk_x + disk_basis_y * poisson_disk_y);

        sum += opaque_shadow_uniform_texture.SampleCmpLevelZero(shadow_sampler_uniform, uv, non_linear_depth - perspective_bias).r;
    }

    float opaque = sum / 16.0;
    return opaque * translucent;
}

float specular_d(float n_dot_h, float alpha) {
    float sqr_alpha = alpha * alpha;
    float denominator = n_dot_h * n_dot_h * (sqr_alpha - 1.0) + 1.0;
    denominator = PI * denominator * denominator;
    return sqr_alpha / denominator;
}

float3 specular_f(float v_dot_h, float3 f0) {
    return f0 + (1.0 - f0) * pow(1.0 - v_dot_h, 5.0);
}

float specular_g1(float n_dot_x, float k) {
    return n_dot_x / (n_dot_x * (1.0 - k) + k);
}

float specular_g(float n_dot_l, float n_dot_v, float roughness) {
    float alpha = (roughness + 1.0) / 2.0;
    float k = alpha * alpha / 2.0;
    return specular_g1(n_dot_l, k) * specular_g1(n_dot_v, k);
}

float falloff(float distance, float attenuation_radius) {
    float factor = distance / attenuation_radius;
    float sqr_factor = factor * factor;
    float result = saturate(1.0 - sqr_factor * sqr_factor);
    float sqr_result = result * result;
    return sqr_result;
}

float4 main(float4 position : SV_POSITION) : SV_TARGET {
    float2 texcoord = position.xy * texel_size;
    float2 screen_position = texcoord.xy * float2(2.0, -2.0) - float2(1.0, -1.0);

    float4 albedo_metalness_sample = albedo_metalness_uniform_attachment.Sample(sampler_uniform, texcoord);
    float4 normal_roughness_sample = normal_roughness_uniform_attachment.Sample(sampler_uniform, texcoord);
    float4 depth_sample = depth_uniform_attachment.Sample(sampler_uniform, texcoord);

    float3 albedo = albedo_metalness_sample.rgb;
    float metalness = albedo_metalness_sample.a;
    float3 normal_direction = normal_roughness_sample.rgb;
    float roughness = max(normal_roughness_sample.a, 0.075);
    float depth = depth_sample.r;

    float4 clip_position = mul(inverse_view_projection, float4(screen_position.x, screen_position.y, depth, 1.0));
    clip_position /= clip_position.w;

    float3 light_position = point_light_push_constants.position.xyz;
    float3 light_luminance = point_light_push_constants.luminance.xyz;
    float falloff_radius = point_light_push_constants.radius_frustum.x;

    float3 view_direction = normalize(view_position - clip_position.xyz);

    float3 light_direction = light_position - clip_position.xyz;
    float light_distance = length(light_direction);
    light_direction = light_direction / light_distance;

    float3 halfway_direction = normalize(view_direction + light_direction);

    float n_dot_l = saturate(dot(normal_direction, light_direction));
    float n_dot_v = saturate(dot(normal_direction, view_direction));
    float n_dot_h = saturate(dot(normal_direction, halfway_direction));
    float v_dot_h = saturate(dot(view_direction, halfway_direction));

    float alpha = roughness * roughness;
    float3 f0 = lerp(float3(0.04, 0.04, 0.04), albedo, metalness);

    float  d = specular_d(n_dot_h, alpha);
    float3 f = specular_f(v_dot_h, f0);
    float  g = specular_g(n_dot_l, n_dot_v, roughness);

    float3 specular = d * f * g / (4.0 * n_dot_l * n_dot_v + 1e-5);
    float3 diffuse = (1.0 - f) * (1.0 - metalness) * (albedo / PI);

    float attenuation = falloff(light_distance, falloff_radius) / (light_distance * light_distance + 1.0);

    float3 shadow_sample = pcss(light_position, clip_position.xyz, normal_direction);
    
    return float4((specular + diffuse) * n_dot_l * attenuation * light_luminance * shadow_sample, 1.0);
}
