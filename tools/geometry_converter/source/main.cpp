#include "gltf_utils.h"

#include <core/io/binary_writer.h>
#include <core/math/aabbox.h>
#include <core/math/float4x4.h>
#include <core/math/quaternion.h>
#include <core/math/transform.h>
#include <core/utils/enum_utils.h>

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

using namespace kw;

struct Vertex {
    float3 position;
    float3 normal;
    float4 tangent;
    float2 texcoord_0;
};

struct SkinnedVertex {
    std::array<uint8_t, 4> joints;
    std::array<uint8_t, 4> weights;
};

struct Skeleton {
    std::vector<uint32_t> parent_joint_indices;
    std::vector<float4x4> inverse_bind_matrices;
    std::vector<float4x4> bind_matrices;
    std::vector<std::string> joint_names;
};

struct Geometry {
    Geometry()
        : bounds(float3(FLT_MAX, FLT_MAX, FLT_MAX), float3(-FLT_MAX, -FLT_MAX, -FLT_MAX))
    {
    }

    std::vector<Vertex> vertices;
    std::vector<SkinnedVertex> skinned_vertices;
    std::vector<uint32_t> indices;
    aabbox bounds;
    Skeleton skeleton;
};

struct Animation {
    struct JointKeyframe {
        float timestamp;
        transform transform;
    };

    struct JointAnimation {
        std::vector<JointKeyframe> keyframes;
    };

    std::vector<JointAnimation> joint_animations;
};

enum class Attributes {
    NONE       = 0,
    POSITION   = 1 << 0,
    NORMAL     = 1 << 1,
    TANGENT    = 1 << 2,
    TEXCOORD_0 = 1 << 3,
    JOINTS_0   = 1 << 4,
    WEIGHTS_0  = 1 << 5,
};

KW_DEFINE_ENUM_BITMASK(Attributes);

static tinygltf::TinyGLTF gltf;
static tinygltf::Model model;
static std::string error;
static std::string warning;
static std::string filename;
static std::unordered_map<int, size_t> node_index_to_joint_index;
static std::unordered_map<size_t, int> joint_index_to_node_index;
static std::vector<int> node_parent_indices;
static std::vector<std::map<float, transform>> node_animations;
static Geometry result_geometry;
static Animation result_animation;

namespace kw::EndianUtils {

static float2 swap_le(float2 vector) {
    vector.x = swap_le(vector.x);
    vector.y = swap_le(vector.y);
    return vector;
}

static float3 swap_le(float3 vector) {
    vector.x = swap_le(vector.x);
    vector.y = swap_le(vector.y);
    vector.z = swap_le(vector.z);
    return vector;
}

static float4 swap_le(float4 vector) {
    vector.x = swap_le(vector.x);
    vector.y = swap_le(vector.y);
    vector.z = swap_le(vector.z);
    vector.w = swap_le(vector.w);
    return vector;
}

static quaternion swap_le(quaternion quaternion) {
    quaternion.x = swap_le(quaternion.x);
    quaternion.y = swap_le(quaternion.y);
    quaternion.z = swap_le(quaternion.z);
    quaternion.w = swap_le(quaternion.w);
    return quaternion;
}

static float4x4 swap_le(float4x4 matrix) {
    matrix._r0 = swap_le(matrix._r0);
    matrix._r1 = swap_le(matrix._r1);
    matrix._r2 = swap_le(matrix._r2);
    matrix._r3 = swap_le(matrix._r3);
    return matrix;
}

static Vertex swap_le(Vertex vertex) {
    vertex.position = swap_le(vertex.position);
    vertex.normal = swap_le(vertex.normal);
    vertex.tangent = swap_le(vertex.tangent);
    vertex.texcoord_0 = swap_le(vertex.texcoord_0);
    return vertex;
}

static transform swap_le(transform transform) {
    transform.translation = swap_le(transform.translation);
    transform.rotation = swap_le(transform.rotation);
    transform.scale = swap_le(transform.scale);
    return transform;
}

static Animation::JointKeyframe swap_le(Animation::JointKeyframe keyframe) {
    keyframe.timestamp = swap_le(keyframe.timestamp);
    keyframe.transform = swap_le(keyframe.transform);
    return keyframe;
}

} // namespace kw::EndianUtils

constexpr uint32_t KWG_SIGNATURE = ' GWK';
constexpr uint32_t KWA_SIGNATURE = ' AWK';

static bool save_result_geometry(const char* path) {
    BinaryWriter writer(path);

    if (!writer) {
        std::cout << "Failed to open output geometry file \"" << path << "\"." << std::endl;
        return false;
    }

    writer.write_le<uint32_t>(KWG_SIGNATURE);
    writer.write_le<uint32_t>(result_geometry.vertices.size());
    writer.write_le<uint32_t>(result_geometry.skinned_vertices.size());
    writer.write_le<uint32_t>(result_geometry.indices.size());
    writer.write_le<uint32_t>(result_geometry.skeleton.inverse_bind_matrices.size());
    writer.write_le<float>(result_geometry.bounds.data, std::size(result_geometry.bounds.data));

    writer.write_le<Vertex>(result_geometry.vertices.data(), result_geometry.vertices.size());
    writer.write(result_geometry.skinned_vertices.data(), sizeof(SkinnedVertex) * result_geometry.skinned_vertices.size());

    if (result_geometry.vertices.size() < UINT16_MAX) {
        for (uint32_t index : result_geometry.indices) {
            writer.write_le<uint16_t>(index);
        }
    } else {
        writer.write_le<uint32_t>(result_geometry.indices.data(), result_geometry.indices.size());
    }

    writer.write_le<uint32_t>(result_geometry.skeleton.parent_joint_indices.data(), result_geometry.skeleton.parent_joint_indices.size());
    writer.write_le<float4x4>(result_geometry.skeleton.inverse_bind_matrices.data(), result_geometry.skeleton.inverse_bind_matrices.size());
    writer.write_le<float4x4>(result_geometry.skeleton.bind_matrices.data(), result_geometry.skeleton.bind_matrices.size());

    for (const std::string& name : result_geometry.skeleton.joint_names) {
        writer.write_le<uint32_t>(name.size());
        writer.write(name.data(), name.size());
    }

    if (!writer) {
        std::cout << "Failed to write to output geometry file \"" << path << "\"." << std::endl;
        return false;
    }

    return true;
}

static bool save_result_animation(const char* path) {
    BinaryWriter writer(path);

    if (!writer) {
        std::cout << "Failed to open output geometry file \"" << path << "\"." << std::endl;
        return false;
    }

    writer.write_le<uint32_t>(KWA_SIGNATURE);
    writer.write_le<uint32_t>(result_animation.joint_animations.size());

    for (Animation::JointAnimation& joint_animation : result_animation.joint_animations) {
        writer.write_le<uint32_t>(joint_animation.keyframes.size());
        writer.write_le<Animation::JointKeyframe>(joint_animation.keyframes.data(), joint_animation.keyframes.size());
    }

    if (!writer) {
        std::cout << "Failed to write to output animation file \"" << path << "\"." << std::endl;
        return false;
    }

    return true;
}

static transform sample_animation(const std::map<float, transform>& animation, float timestamp) {
    transform result;

    auto next_it = animation.lower_bound(timestamp);
    if (next_it != animation.end()) {
        if (next_it != animation.begin()) {
            auto prev_it = std::prev(next_it);

            float factor = (timestamp - prev_it->first) / (next_it->first - prev_it->first);

            result = lerp(prev_it->second, next_it->second, factor);
        } else {
            auto prev_it = animation.rbegin();

            float factor = next_it->first > EPSILON ? timestamp / next_it->first : 1.f;

            result = lerp(prev_it->second, next_it->second, factor);
        }
    } else {
        if (!animation.empty()) {
            result = animation.rbegin()->second;
        }
    }

    return result;
}

static std::map<float, transform> compute_joint_animation(int node_index, const std::map<float, transform>& child_animation) {
    std::map<float, transform> parent_animation = node_animations[node_index];

    std::map<float, transform> current_animations;

    for (const auto& [timestamp, _] : child_animation) {
        current_animations.emplace(timestamp, transform());
    }

    for (const auto& [timestamp, _] : parent_animation) {
        current_animations.emplace(timestamp, transform());
    }

    if (parent_animation.empty()) {
        parent_animation[0.f] = transform(get_node_transform(model.nodes[node_index]));
    }

    for (auto& [timestamp, transform_] : current_animations) {
        transform child_transform = sample_animation(child_animation, timestamp);
        transform parent_transform = sample_animation(parent_animation, timestamp);
        transform_ = child_transform * parent_transform;
    }

    int parent_index = node_parent_indices[node_index];
    if (parent_index >= 0 && node_index_to_joint_index.count(parent_index) == 0) {
        return compute_joint_animation(parent_index, current_animations);
    } else {
        return current_animations;
    }
}

static bool load_animations(const tinygltf::Animation& animation) {
    for (const tinygltf::AnimationChannel& channel : animation.channels) {
        if (channel.sampler < 0 || channel.sampler >= animation.samplers.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid sampler index." << std::endl;
            return false;
        }

        const tinygltf::AnimationSampler& sampler = animation.samplers[channel.sampler];

        if (channel.target_node < 0 || channel.target_node >= model.nodes.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid node index." << std::endl;
            return false;
        }

        std::optional<std::vector<float>> input_data = load_gltf_accessor<float>(sampler.input);

        if (!input_data) {
            return false;
        }

        if (channel.target_path == "translation") {
            std::optional<std::vector<float3>> output_data = load_gltf_accessor<float3>(sampler.output);

            if (!output_data) {
                return false;
            }

            if ((*input_data).size() != (*output_data).size()) {
                std::cout << "Error in geometry file \"" << filename << "\": Mismatching sampler sizes." << std::endl;
                return false;
            }

            for (size_t i = 0; i < (*input_data).size(); i++) {
                node_animations[channel.target_node][(*input_data)[i]].translation = (*output_data)[i];

                // Convert right handed to left handed coordinate system.
                node_animations[channel.target_node][(*input_data)[i]].translation.z *= -1.f;
            }
        } else if (channel.target_path == "rotation") {
            std::optional<std::vector<quaternion>> output_data = load_gltf_accessor<quaternion, true>(sampler.output);

            if (!output_data) {
                return false;
            }

            if ((*input_data).size() != (*output_data).size()) {
                std::cout << "Error in geometry file \"" << filename << "\": Mismatching sampler sizes." << std::endl;
                return false;
            }

            for (size_t i = 0; i < (*input_data).size(); i++) {
                node_animations[channel.target_node][(*input_data)[i]].rotation = normalize((*output_data)[i]);

                // Convert right handed to left handed coordinate system.
                node_animations[channel.target_node][(*input_data)[i]].rotation.x *= -1.f;
                node_animations[channel.target_node][(*input_data)[i]].rotation.y *= -1.f;
            }
        } else if (channel.target_path == "scale") {
            std::optional<std::vector<float3>> output_data = load_gltf_accessor<float3, true>(sampler.output);

            if (!output_data) {
                return false;
            }

            if ((*input_data).size() != (*output_data).size()) {
                std::cout << "Error in geometry file \"" << filename << "\": Mismatching sampler sizes." << std::endl;
                return false;
            }

            for (size_t i = 0; i < (*input_data).size(); i++) {
                node_animations[channel.target_node][(*input_data)[i]].scale = (*output_data)[i];
            }
        }
    }

    result_animation.joint_animations.resize(result_geometry.skeleton.inverse_bind_matrices.size());

    for (size_t i = 0; i < result_animation.joint_animations.size(); i++) {
        for (const auto& [timestamp, transform_] : compute_joint_animation(joint_index_to_node_index[i], std::map<float, transform>())) {
            result_animation.joint_animations[i].keyframes.push_back(Animation::JointKeyframe{ timestamp, transform_ });
        }
    }

    return true;
}

static bool assign_joint_parents(int node_index, uint32_t parent_index, const float4x4& parent_transform) {
    const tinygltf::Node& node = model.nodes[node_index];
    
    float4x4 local_transform = get_node_transform(node);
    float4x4 transform = local_transform * parent_transform;

    auto it1 = node_index_to_joint_index.find(node_index);
    if (it1 != node_index_to_joint_index.end()) {
        parent_index = static_cast<uint32_t>(it1->second);

        result_geometry.skeleton.bind_matrices[it1->second] = transform;

        transform = float4x4();
    }

    for (int child_index : node.children) {
        if (child_index < 0 || child_index >= model.nodes.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid child index." << std::endl;
            return false;
        }

        auto it2 = node_index_to_joint_index.find(child_index);
        if (it2 != node_index_to_joint_index.end()) {
            result_geometry.skeleton.parent_joint_indices[it2->second] = parent_index;
        }

        if (!assign_joint_parents(child_index, parent_index, transform)) {
            return false;
        }
    }

    return true;
}

static bool load_primitive(const tinygltf::Primitive& primitive, const float4x4& transform) {
    std::optional<size_t> vertex_count = std::nullopt;
    std::optional<size_t> skinned_vertex_count = std::nullopt;
    size_t vertex_offset = result_geometry.vertices.size();

    Attributes attribute_mask = Attributes::NONE;

    for (const auto& [attribute, accessor_index] : primitive.attributes) {
        if (attribute == "POSITION") {
            if ((attribute_mask & Attributes::POSITION) == Attributes::POSITION) {
                std::cout << "Error in geometry file \"" << filename << "\": POSITION is specified twice." << std::endl;
                return false;
            }

            attribute_mask |= Attributes::POSITION;

            std::optional<std::vector<float3>> data = load_gltf_accessor<float3>(accessor_index);

            if (!data) {
                return false;
            }

            if (!vertex_count) {
                vertex_count = (*data).size();
                result_geometry.vertices.resize(vertex_offset + *vertex_count);
            } else {
                if (*vertex_count != (*data).size()) {
                    std::cout << "Error in geometry file \"" << filename << "\": Mismatching vertex count." << std::endl;
                    return false;
                }
            }

            for (size_t i = 0; i < *vertex_count; i++) {
                result_geometry.vertices[vertex_offset + i].position = (*data)[i];

                // Convert right handed to left handed coordinate system.
                result_geometry.vertices[vertex_offset + i].position.z *= -1.f;
            }
        } else if (attribute == "NORMAL") {
            if ((attribute_mask & Attributes::NORMAL) == Attributes::NORMAL) {
                std::cout << "Error in geometry file \"" << filename << "\": NORMAL is specified twice." << std::endl;
                return false;
            }

            attribute_mask |= Attributes::NORMAL;

            std::optional<std::vector<float3>> data = load_gltf_accessor<float3>(accessor_index);

            if (!data) {
                return false;
            }

            if (!vertex_count) {
                vertex_count = (*data).size();
                result_geometry.vertices.resize(vertex_offset + *vertex_count);
            } else {
                if (*vertex_count != (*data).size()) {
                    std::cout << "Error in geometry file \"" << filename << "\": Mismatching vertex count." << std::endl;
                    return false;
                }
            }

            for (size_t i = 0; i < *vertex_count; i++) {
                result_geometry.vertices[vertex_offset + i].normal = (*data)[i];

                // Convert right handed to left handed coordinate system.
                result_geometry.vertices[vertex_offset + i].normal.z *= -1.f;
            }
        } else if (attribute == "TANGENT") {
            if ((attribute_mask & Attributes::TANGENT) == Attributes::TANGENT) {
                std::cout << "Error in geometry file \"" << filename << "\": TANGENT is specified twice." << std::endl;
                return false;
            }

            attribute_mask |= Attributes::TANGENT;

            std::optional<std::vector<float4>> data = load_gltf_accessor<float4>(accessor_index);

            if (!data) {
                return false;
            }

            if (!vertex_count) {
                vertex_count = (*data).size();
                result_geometry.vertices.resize(vertex_offset + *vertex_count);
            } else {
                if (*vertex_count != (*data).size()) {
                    std::cout << "Error in geometry file \"" << filename << "\": Mismatching vertex count." << std::endl;
                    return false;
                }
            }

            for (size_t i = 0; i < *vertex_count; i++) {
                result_geometry.vertices[vertex_offset + i].tangent = (*data)[i];

                // Convert right handed to left handed coordinate system.
                result_geometry.vertices[vertex_offset + i].tangent.z *= -1.f;
            }
        } else if (attribute == "TEXCOORD_0") {
            if ((attribute_mask & Attributes::TEXCOORD_0) == Attributes::TEXCOORD_0) {
                std::cout << "Error in geometry file \"" << filename << "\": TEXCOORD_0 is specified twice." << std::endl;
                return false;
            }

            attribute_mask |= Attributes::TEXCOORD_0;

            std::optional<std::vector<float2>> data = load_gltf_accessor<float2, true>(accessor_index);

            if (!data) {
                return false;
            }

            if (!vertex_count) {
                vertex_count = (*data).size();
                result_geometry.vertices.resize(vertex_offset + *vertex_count);
            } else {
                if (*vertex_count != (*data).size()) {
                    std::cout << "Error in geometry file \"" << filename << "\": Mismatching vertex count." << std::endl;
                    return false;
                }
            }

            for (size_t i = 0; i < *vertex_count; i++) {
                result_geometry.vertices[vertex_offset + i].texcoord_0 = (*data)[i];
            }
        } else if (attribute == "JOINTS_0") {
            if ((attribute_mask & Attributes::JOINTS_0) == Attributes::JOINTS_0) {
                std::cout << "Error in geometry file \"" << filename << "\": JOINTS_0 is specified twice." << std::endl;
                return false;
            }

            attribute_mask |= Attributes::JOINTS_0;

            std::optional<std::vector<std::array<uint8_t, 4>>> data = load_gltf_accessor<std::array<uint8_t, 4>>(accessor_index);

            if (!data) {
                return false;
            }

            if (!skinned_vertex_count) {
                skinned_vertex_count = (*data).size();
                result_geometry.skinned_vertices.resize(vertex_offset + *skinned_vertex_count);
            } else {
                if (*skinned_vertex_count != (*data).size()) {
                    std::cout << "Error in geometry file \"" << filename << "\": Mismatching vertex count." << std::endl;
                    return false;
                }
            }

            for (size_t i = 0; i < *skinned_vertex_count; i++) {
                result_geometry.skinned_vertices[vertex_offset + i].joints = (*data)[i];
            }
        } else if (attribute == "WEIGHTS_0") {
            if ((attribute_mask & Attributes::WEIGHTS_0) == Attributes::WEIGHTS_0) {
                std::cout << "Error in geometry file \"" << filename << "\": WEIGHTS_0 is specified twice." << std::endl;
                return false;
            }

            attribute_mask |= Attributes::WEIGHTS_0;

            std::optional<std::vector<std::array<uint8_t, 4>>> data = load_gltf_accessor<std::array<uint8_t, 4>, true>(accessor_index);

            if (!data) {
                return false;
            }

            if (!skinned_vertex_count) {
                skinned_vertex_count = (*data).size();
                result_geometry.skinned_vertices.resize(vertex_offset + *skinned_vertex_count);
            } else {
                if (*skinned_vertex_count != (*data).size()) {
                    std::cout << "Error in geometry file \"" << filename << "\": Mismatching vertex count." << std::endl;
                    return false;
                }
            }

            for (size_t i = 0; i < *skinned_vertex_count; i++) {
                result_geometry.skinned_vertices[vertex_offset + i].weights = (*data)[i];
            }
        }
    }

    Attributes required_mask = Attributes::POSITION | Attributes::NORMAL | Attributes::TANGENT | Attributes::TEXCOORD_0;
    if ((attribute_mask & required_mask) != required_mask) {
        std::string missing_attributes;
        
        if ((attribute_mask & Attributes::POSITION) != Attributes::POSITION) {
            missing_attributes += "POSITION";
        }
        
        if ((attribute_mask & Attributes::NORMAL) != Attributes::NORMAL) {
            if (!missing_attributes.empty()) {
                missing_attributes += ", ";
            }
            missing_attributes += "NORMAL";
        }

        if ((attribute_mask & Attributes::TANGENT) != Attributes::TANGENT) {
            if (!missing_attributes.empty()) {
                missing_attributes += ", ";
            }
            missing_attributes += "TANGENT";
        }

        if ((attribute_mask & Attributes::TEXCOORD_0) != Attributes::TEXCOORD_0) {
            if (!missing_attributes.empty()) {
                missing_attributes += ", ";
            }
            missing_attributes += "TEXCOORD_0";
        }

        std::cout << "Error in geometry file \"" << filename << "\": Attributes " << missing_attributes << " are missing." << std::endl;
        return false;
    }

    Attributes skinned_mask = Attributes::JOINTS_0 | Attributes::WEIGHTS_0;
    if ((attribute_mask & skinned_mask) != Attributes::NONE && (attribute_mask & skinned_mask) != skinned_mask) {
        std::cout << "Error in geometry file \"" << filename << "\": Only one skinning attribute is specified." << std::endl;
        return false;
    }

    if (!result_geometry.skinned_vertices.empty() && result_geometry.skinned_vertices.size() != result_geometry.vertices.size()) {
        std::cout << "Error in geometry file \"" << filename << "\": Mismatching vertex count." << std::endl;
        return false;
    }

    float4x4 inverse_transform = inverse(transform);

    for (size_t i = 0; i < *vertex_count; i++) {
        float3 local_position = result_geometry.vertices[vertex_offset + i].position;
        float3 position = point_transform(local_position, transform);

        float3 local_normal = result_geometry.vertices[vertex_offset + i].normal;
        float3 normal = normalize(normal_transform(local_normal, inverse_transform));

        float3 local_tangent = result_geometry.vertices[vertex_offset + i].tangent.xyz;
        float3 tangent = normalize(local_tangent * transform);

        float3 local_bitangent = cross(local_normal, local_tangent) * result_geometry.vertices[vertex_offset + i].tangent.w;
        float3 bitangent = normalize(local_bitangent * transform);

        float bitangent_factor = 1.f;
        if (dot(cross(normal, tangent), bitangent) < 0.f) {
            bitangent_factor = -1.f;
        }

        result_geometry.vertices[vertex_offset + i].position = position;
        result_geometry.vertices[vertex_offset + i].normal = normal;
        result_geometry.vertices[vertex_offset + i].tangent = float4(tangent, bitangent_factor);

        if (vertex_offset == 0 && i == 0) {
            result_geometry.bounds = aabbox(position, float3());
        } else {
            result_geometry.bounds += position;
        }
    }

    std::optional<std::vector<uint32_t>> data = load_gltf_accessor<uint32_t>(primitive.indices);

    if (!data) {
        return false;
    }

    size_t index_offset = result_geometry.indices.size();
    size_t index_count = (*data).size();
    result_geometry.indices.resize(index_offset + index_count);

    for (size_t i = 0; i < index_count; i++) {
        result_geometry.indices[index_offset + i] = vertex_offset + (*data)[i];
    }

    return true;
}

static bool load_mesh(const tinygltf::Mesh& mesh, const float4x4& transform) {
    for (const tinygltf::Primitive& primitive : mesh.primitives) {
        if (primitive.mode == TINYGLTF_MODE_TRIANGLES) {
            if (!load_primitive(primitive, transform)) {
                return false;
            }
        } else {
            std::cout << "Warning in geometry file \"" << filename << "\": Only TRIANGLES primitives are supported." << std::endl;
        }
    }

    return true;
}

static bool load_node(int node_index, const float4x4& parent_transform) {
    const tinygltf::Node& node = model.nodes[node_index];

    float4x4 local_transform = get_node_transform(node);
    float4x4 transform_;

    if (node.skin >= 0) {
        size_t joint_offset = result_geometry.skeleton.inverse_bind_matrices.size();

        if (node.skin >= model.skins.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid skin index." << std::endl;
            return false;
        }

        const tinygltf::Skin& skin = model.skins[node.skin];

        if (skin.joints.empty()) {
            std::cout << "Error in geometry file \"" << filename << "\": At least one joint is required." << std::endl;
            return false;
        }

        std::optional<std::vector<float4x4>> data = load_gltf_accessor<float4x4>(skin.inverseBindMatrices);

        if (!data) {
            return false;
        }

        if ((*data).size() != skin.joints.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Mismatching joint sizes." << std::endl;
            return false;
        }

        result_geometry.skeleton.parent_joint_indices.resize(joint_offset + skin.joints.size());
        result_geometry.skeleton.inverse_bind_matrices.resize(joint_offset + skin.joints.size());
        result_geometry.skeleton.bind_matrices.resize(joint_offset + skin.joints.size());
        result_geometry.skeleton.joint_names.resize(joint_offset + skin.joints.size());

        for (size_t i = 0; i < skin.joints.size(); i++) {
            int joint_node_index = skin.joints[i];

            if (joint_node_index >= model.nodes.size()) {
                std::cout << "Error in geometry file \"" << filename << "\": Invalid joint node index." << std::endl;
                return false;
            }

            const tinygltf::Node& joint_node = model.nodes[joint_node_index];

            result_geometry.skeleton.joint_names[joint_offset + i] = joint_node.name;
            result_geometry.skeleton.inverse_bind_matrices[joint_offset + i] = (*data)[i];
            result_geometry.skeleton.parent_joint_indices[joint_offset + i] = UINT32_MAX;

            // Convert right handed to left handed coordinate system.
            float4x4& inverse_bind_matrix = result_geometry.skeleton.inverse_bind_matrices[joint_offset + i];
            inverse_bind_matrix[0][2] *= -1.f;
            inverse_bind_matrix[1][2] *= -1.f;
            inverse_bind_matrix[3][2] *= -1.f;
            inverse_bind_matrix[2][0] *= -1.f;
            inverse_bind_matrix[2][1] *= -1.f;
            inverse_bind_matrix[2][3] *= -1.f;
        }

        node_index_to_joint_index.reserve(skin.joints.size());
        joint_index_to_node_index.reserve(skin.joints.size());

        for (size_t i = 0; i < skin.joints.size(); i++) {
            node_index_to_joint_index.emplace(skin.joints[i], joint_offset + i);
            joint_index_to_node_index.emplace(joint_offset + i, skin.joints[i]);
        }
    } else {
        transform_ = local_transform * parent_transform;
    }

    if (node.mesh >= 0) {
        if (node.mesh >= model.meshes.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid mesh index." << std::endl;
            return false;
        }

        if (!load_mesh(model.meshes[node.mesh], transform_)) {
            return false;
        }
    }

    for (int child_index : node.children) {
        if (child_index < 0 || child_index >= model.nodes.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid child index." << std::endl;
            return false;
        }

        node_parent_indices[child_index] = node_index;

        if (!load_node(child_index, transform_)) {
            return false;
        }
    }

    return true;
}

bool image_loader_dummy(tinygltf::Image*, const int, std::string*, std::string*, int, int, const unsigned char*, int, void* user_pointer) {
    std::cout << "Warning in geometry file \"" << filename << "\": Texture is not used. Prefer to exclude textures and materials from geometry files." << std::endl;
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Geometry converter requires at two command line arguments: input *.GLB file and output *.KWG file." << std::endl;
        return 1;
    }

    filename = argv[1];

    gltf.SetImageLoader(image_loader_dummy, nullptr);

    bool is_loaded = gltf.LoadBinaryFromFile(&model, &error, &warning, filename);

    if (!error.empty()) {
        std::cout << "Error in geometry file \"" << filename << "\": " << error << std::endl;
    }
    
    if (!warning.empty()) {
        std::cout << "Warning in geometry file \"" << filename << "\": " << warning << std::endl;
    }

    if (!is_loaded) {
        std::cout << "Error in geometry file \"" << filename << "\": Failed to load." << std::endl;
        return 1;
    }

    node_parent_indices.resize(model.nodes.size(), -1);
    node_animations.resize(model.nodes.size());

    if (model.defaultScene < 0 || model.defaultScene >= model.scenes.size()) {
        std::cout << "Error in geometry file \"" << filename << "\": Invalid default scene." << std::endl;
        return 1;
    }

    const tinygltf::Scene& scene = model.scenes[model.defaultScene];

    for (int node_index : scene.nodes) {
        if (node_index < 0 || node_index >= model.nodes.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid node index." << std::endl;
            return 1;
        }

        if (!load_node(node_index, float4x4())) {
            return 1;
        }
    }

    for (int node_index : scene.nodes) {
        if (!node_index_to_joint_index.empty() && !assign_joint_parents(node_index, UINT32_MAX, float4x4())) {
            return 1;
        }
    }

    // If geometry file has any animations, export animation only.
    if (!model.animations.empty()) {
        if (!load_animations(model.animations[0])) {
            return 1;
        }

        if (!save_result_animation(argv[2])) {
            return 1;
        }
    } else {
        if (!save_result_geometry(argv[2])) {
            return 1;
        }
    }

    return 0;
}
