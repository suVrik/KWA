#include <core/io/binary_writer.h>
#include <core/math/aabbox.h>
#include <core/math/float4x4.h>
#include <core/math/quaternion.h>
#include <core/utils/enum_utils.h>

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

#include <fstream>
#include <iostream>
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
    uint8_t joints[4];
    uint8_t weights[4];
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
static std::unordered_map<int, size_t> joint_mapping;
static Geometry result;

namespace kw::EndianUtils {

float2 swap_le(float2 vector) {
    vector.x = swap_le(vector.x);
    vector.y = swap_le(vector.y);
    return vector;
}

float3 swap_le(float3 vector) {
    vector.x = swap_le(vector.x);
    vector.y = swap_le(vector.y);
    vector.z = swap_le(vector.z);
    return vector;
}

float4 swap_le(float4 vector) {
    vector.x = swap_le(vector.x);
    vector.y = swap_le(vector.y);
    vector.z = swap_le(vector.z);
    vector.w = swap_le(vector.w);
    return vector;
}

float4x4 swap_le(float4x4 matrix) {
    matrix._r0 = swap_le(matrix._r0);
    matrix._r1 = swap_le(matrix._r1);
    matrix._r2 = swap_le(matrix._r2);
    matrix._r3 = swap_le(matrix._r3);
    return matrix;
}

Vertex swap_le(Vertex vertex) {
    vertex.position = swap_le(vertex.position);
    vertex.normal = swap_le(vertex.normal);
    vertex.tangent = swap_le(vertex.tangent);
    vertex.texcoord_0 = swap_le(vertex.texcoord_0);
    return vertex;
}

} // namespace kw::EndianUtils

constexpr uint32_t KWG_SIGNATURE = ' GWK';

static bool save_result(const char* path) {
    BinaryWriter writer(path);

    if (!writer) {
        std::cout << "Failed to open output geometry file \"" << path << "\"." << std::endl;
        return false;
    }

    writer.write_le<uint32_t>(KWG_SIGNATURE);
    writer.write_le<uint32_t>(result.vertices.size());
    writer.write_le<uint32_t>(result.skinned_vertices.size());
    writer.write_le<uint32_t>(result.indices.size());
    writer.write_le<uint32_t>(result.skeleton.inverse_bind_matrices.size());
    writer.write_le<float>(result.bounds.data, std::size(result.bounds.data));

    writer.write_le<Vertex>(result.vertices.data(), result.vertices.size());
    writer.write(result.skinned_vertices.data(), sizeof(SkinnedVertex) * result.skinned_vertices.size());

    if (result.vertices.size() < UINT16_MAX) {
        for (uint32_t index : result.indices) {
            writer.write_le<uint16_t>(index);
        }
    } else {
        writer.write_le<uint32_t>(result.indices.data(), result.indices.size());
    }

    writer.write_le<uint32_t>(result.skeleton.parent_joint_indices.data(), result.skeleton.parent_joint_indices.size());
    writer.write_le<float4x4>(result.skeleton.inverse_bind_matrices.data(), result.skeleton.inverse_bind_matrices.size());
    writer.write_le<float4x4>(result.skeleton.bind_matrices.data(), result.skeleton.bind_matrices.size());

    for (const std::string& name : result.skeleton.joint_names) {
        writer.write_le<uint32_t>(name.size());
        writer.write(name.data(), name.size());
    }

    if (!writer) {
        std::cout << "Failed to write to output geometry file \"" << path << "\"." << std::endl;
        return false;
    }

    return true;
}

static bool load_primitive(const tinygltf::Primitive& primitive, const float4x4& transform) {
    size_t vertex_count = 0;
    size_t vertex_offset = result.vertices.size();

    Attributes attribute_mask = Attributes::NONE;

    for (const auto& [attribute, accessor_index] : primitive.attributes) {
        if (accessor_index < 0 || accessor_index >= model.accessors.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid attribute accessor index." << std::endl;
            return false;
        }

        const tinygltf::Accessor& accessor = model.accessors[accessor_index];

        if (accessor.sparse.isSparse) {
            std::cout << "Error in geometry file \"" << filename << "\": Sparse attribute accessors are not supported." << std::endl;
            return false;
        }

        if (accessor.count == 0) {
            std::cout << "Error in geometry file \"" << filename << "\": Empty attribute accessor." << std::endl;
            return false;
        }

        if (vertex_count == 0) {
            vertex_count = accessor.count;
            result.vertices.resize(result.vertices.size() + vertex_count);
        }

        if (accessor.count != vertex_count) {
            std::cout << "Error in geometry file \"" << filename << "\": Attribute accessor count mismatch." << std::endl;
            return false;
        }

        if (accessor.bufferView < 0 || accessor.bufferView >= model.bufferViews.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid attribute buffer view index." << std::endl;
            return false;
        }

        const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];

        if (buffer_view.target != TINYGLTF_TARGET_ARRAY_BUFFER) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid attribute buffer view target." << std::endl;
            return false;
        }

        if (buffer_view.buffer < 0 || buffer_view.buffer >= model.buffers.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid attribute buffer index." << std::endl;
            return false;
        }

        const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];

        if (accessor.byteOffset + buffer_view.byteOffset + buffer_view.byteLength > buffer.data.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid attribute buffer view range." << std::endl;
            return false;
        }

        const unsigned char* buffer_data = &buffer.data[accessor.byteOffset + buffer_view.byteOffset];

        if (attribute == "POSITION") {
            size_t byte_stride = buffer_view.byteStride == 0 ? sizeof(float) * 3 : buffer_view.byteStride;

            if (accessor.type != TINYGLTF_TYPE_VEC3) {
                std::cout << "Error in geometry file \"" << filename << "\": Only VEC3 is allowed for POSITION." << std::endl;
                return false;
            }

            if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
                std::cout << "Error in geometry file \"" << filename << "\": Only FLOAT is allowed for POSITION." << std::endl;
                return false;
            }

            if (byte_stride < sizeof(float) * 3) {
                std::cout << "Error in geometry file \"" << filename << "\": Invalid POSITION stride." << std::endl;
                return false;
            }

            if (byte_stride * (vertex_count - 1) + sizeof(float) * 3 > buffer_view.byteLength) {
                std::cout << "Error in geometry file \"" << filename << "\": Invalid POSITION range." << std::endl;
                return false;
            }

            if ((attribute_mask & Attributes::POSITION) == Attributes::POSITION) {
                std::cout << "Error in geometry file \"" << filename << "\": POSITION is specified twice." << std::endl;
                return false;
            }

            attribute_mask |= Attributes::POSITION;

            for (size_t i = 0; i < vertex_count; i++) {
                result.vertices[vertex_offset + i].position = *reinterpret_cast<const float3*>(&buffer_data[i * byte_stride]);
            }
        } else if (attribute == "NORMAL") {
            size_t byte_stride = buffer_view.byteStride == 0 ? sizeof(float) * 3 : buffer_view.byteStride;

            if (accessor.type != TINYGLTF_TYPE_VEC3) {
                std::cout << "Error in geometry file \"" << filename << "\": Only VEC3 is allowed for NORMAL." << std::endl;
                return false;
            }

            if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
                std::cout << "Error in geometry file \"" << filename << "\": Only FLOAT is allowed for NORMAL." << std::endl;
                return false;
            }

            if (byte_stride < sizeof(float) * 3) {
                std::cout << "Error in geometry file \"" << filename << "\": Invalid NORMAL stride." << std::endl;
                return false;
            }

            if (byte_stride * (vertex_count - 1) + sizeof(float) * 3 > buffer_view.byteLength) {
                std::cout << "Error in geometry file \"" << filename << "\": Invalid NORMAL range." << std::endl;
                return false;
            }

            if ((attribute_mask & Attributes::NORMAL) == Attributes::NORMAL) {
                std::cout << "Error in geometry file \"" << filename << "\": NORMAL is specified twice." << std::endl;
                return false;
            }

            attribute_mask |= Attributes::NORMAL;

            for (size_t i = 0; i < vertex_count; i++) {
                result.vertices[vertex_offset + i].normal = *reinterpret_cast<const float3*>(&buffer_data[i * byte_stride]);
            }
        } else if (attribute == "TANGENT") {
            size_t byte_stride = buffer_view.byteStride == 0 ? sizeof(float) * 4 : buffer_view.byteStride;

            if (accessor.type != TINYGLTF_TYPE_VEC4) {
                std::cout << "Error in geometry file \"" << filename << "\": Only VEC4 is allowed for TANGENT." << std::endl;
                return false;
            }

            if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
                std::cout << "Error in geometry file \"" << filename << "\": Only FLOAT is allowed for TANGENT." << std::endl;
                return false;
            }

            if (byte_stride < sizeof(float) * 4) {
                std::cout << "Error in geometry file \"" << filename << "\": Invalid TANGENT stride." << std::endl;
                return false;
            }

            if (byte_stride * (vertex_count - 1) + sizeof(float) * 4 > buffer_view.byteLength) {
                std::cout << "Error in geometry file \"" << filename << "\": Invalid TANGENT range." << std::endl;
                return false;
            }

            if ((attribute_mask & Attributes::TANGENT) == Attributes::TANGENT) {
                std::cout << "Error in geometry file \"" << filename << "\": TANGENT is specified twice." << std::endl;
                return false;
            }

            attribute_mask |= Attributes::TANGENT;

            for (size_t i = 0; i < vertex_count; i++) {
                result.vertices[vertex_offset + i].tangent = *reinterpret_cast<const float4*>(&buffer_data[i * byte_stride]);
            }
        } else if (attribute == "TEXCOORD_0") {
            if (accessor.type != TINYGLTF_TYPE_VEC2) {
                std::cout << "Error in geometry file \"" << filename << "\": Only VEC2 is allowed for TEXCOORD_0." << std::endl;
                return false;
            }

            if ((attribute_mask & Attributes::TEXCOORD_0) == Attributes::TEXCOORD_0) {
                std::cout << "Error in geometry file \"" << filename << "\": TEXCOORD_0 is specified twice." << std::endl;
                return false;
            }

            attribute_mask |= Attributes::TEXCOORD_0;

            if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                size_t byte_stride = buffer_view.byteStride == 0 ? sizeof(float) * 2 : buffer_view.byteStride;

                if (byte_stride < sizeof(float) * 2) {
                    std::cout << "Error in geometry file \"" << filename << "\": Invalid TEXCOORD_0 stride." << std::endl;
                    return false;
                }

                if (byte_stride * (vertex_count - 1) + sizeof(float) * 2 > buffer_view.byteLength) {
                    std::cout << "Error in geometry file \"" << filename << "\": Invalid TEXCOORD_0 range." << std::endl;
                    return false;
                }

                for (size_t i = 0; i < vertex_count; i++) {
                    result.vertices[vertex_offset + i].texcoord_0 = *reinterpret_cast<const float2*>(&buffer_data[i * byte_stride]);
                }
            } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                size_t byte_stride = buffer_view.byteStride == 0 ? sizeof(uint8_t) * 2 : buffer_view.byteStride;

                if (byte_stride < sizeof(uint8_t) * 2) {
                    std::cout << "Error in geometry file \"" << filename << "\": Invalid TEXCOORD_0 stride." << std::endl;
                    return false;
                }

                if (byte_stride * (vertex_count - 1) + sizeof(uint8_t) * 2 > buffer_view.byteLength) {
                    std::cout << "Error in geometry file \"" << filename << "\": Invalid TEXCOORD_0 range." << std::endl;
                    return false;
                }

                for (size_t i = 0; i < vertex_count; i++) {
                    const uint8_t* texcoord_unnormalized = reinterpret_cast<const uint8_t*>(&buffer_data[i * byte_stride]);
                    result.vertices[vertex_offset + i].texcoord_0 = float2(texcoord_unnormalized[0] / 255.f, texcoord_unnormalized[1] / 255.f);
                }
            } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                size_t byte_stride = buffer_view.byteStride == 0 ? sizeof(uint16_t) * 2 : buffer_view.byteStride;

                if (byte_stride < sizeof(uint16_t) * 2) {
                    std::cout << "Error in geometry file \"" << filename << "\": Invalid TEXCOORD_0 stride." << std::endl;
                    return false;
                }

                if (byte_stride * (vertex_count - 1) + sizeof(uint16_t) * 2 > buffer_view.byteLength) {
                    std::cout << "Error in geometry file \"" << filename << "\": Invalid TEXCOORD_0 range." << std::endl;
                    return false;
                }

                for (size_t i = 0; i < vertex_count; i++) {
                    const uint16_t* texcoord_unnormalized = reinterpret_cast<const uint16_t*>(&buffer_data[i * byte_stride]);
                    result.vertices[vertex_offset + i].texcoord_0 = float2(texcoord_unnormalized[0] / 65535.f, texcoord_unnormalized[1] / 65535.f);
                }
            } else {
                std::cout << "Error in geometry file \"" << filename << "\": Invalid TEXCOORD_0 component type." << std::endl;
                return false;
            }
        } else if (attribute == "JOINTS_0") {
            if (result.skinned_vertices.size() < result.vertices.size()) {
                result.skinned_vertices.resize(result.vertices.size());
            }

            if (accessor.type != TINYGLTF_TYPE_VEC4) {
                std::cout << "Error in geometry file \"" << filename << "\": Only VEC4 is allowed for JOINTS_0." << std::endl;
                return false;
            }

            if ((attribute_mask & Attributes::JOINTS_0) == Attributes::JOINTS_0) {
                std::cout << "Error in geometry file \"" << filename << "\": JOINTS_0 is specified twice." << std::endl;
                return false;
            }

            attribute_mask |= Attributes::JOINTS_0;

            if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                size_t byte_stride = buffer_view.byteStride == 0 ? sizeof(uint8_t) * 4 : buffer_view.byteStride;

                if (byte_stride < sizeof(uint8_t) * 4) {
                    std::cout << "Error in geometry file \"" << filename << "\": Invalid JOINTS_0 stride." << std::endl;
                    return false;
                }

                if (byte_stride * (vertex_count - 1) + sizeof(uint8_t) * 4 > buffer_view.byteLength) {
                    std::cout << "Error in geometry file \"" << filename << "\": Invalid JOINTS_0 range." << std::endl;
                    return false;
                }

                for (size_t i = 0; i < vertex_count; i++) {
                    for (size_t j = 0; j < 4; j++) {
                        uint8_t joint_index = *reinterpret_cast<const uint8_t*>(&buffer_data[i * byte_stride + j * sizeof(uint8_t)]);
                        result.skinned_vertices[vertex_offset + i].joints[j] = joint_index;
                    }
                }
            } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                size_t byte_stride = buffer_view.byteStride == 0 ? sizeof(uint16_t) * 4 : buffer_view.byteStride;

                if (byte_stride < sizeof(uint16_t) * 4) {
                    std::cout << "Error in geometry file \"" << filename << "\": Invalid JOINTS_0 stride." << std::endl;
                    return false;
                }

                if (byte_stride * (vertex_count - 1) + sizeof(uint16_t) * 4 > buffer_view.byteLength) {
                    std::cout << "Error in geometry file \"" << filename << "\": Invalid JOINTS_0 range." << std::endl;
                    return false;
                }

                for (size_t i = 0; i < vertex_count; i++) {
                    for (size_t j = 0; j < 4; j++) {
                        uint16_t joint_index = *reinterpret_cast<const uint16_t*>(&buffer_data[i * byte_stride + j * sizeof(uint16_t)]);

                        if (joint_index > UINT8_MAX) {
                            std::cout << "Error in geometry file \"" << filename << "\": Invalid joint index." << std::endl;
                            return false;
                        }

                        result.skinned_vertices[vertex_offset + i].joints[j] = static_cast<uint8_t>(joint_index);
                    }
                }
            } else {
                std::cout << "Error in geometry file \"" << filename << "\": Invalid JOINTS_0 component type." << std::endl;
                return false;
            }
        } else if (attribute == "WEIGHTS_0") {
            if (result.skinned_vertices.size() < result.vertices.size()) {
                result.skinned_vertices.resize(result.vertices.size());
            }

            if (accessor.type != TINYGLTF_TYPE_VEC4) {
                std::cout << "Error in geometry file \"" << filename << "\": Only VEC4 is allowed for WEIGHTS_0." << std::endl;
                return false;
            }

            if ((attribute_mask & Attributes::WEIGHTS_0) == Attributes::WEIGHTS_0) {
                std::cout << "Error in geometry file \"" << filename << "\": WEIGHTS_0 is specified twice." << std::endl;
                return false;
            }

            attribute_mask |= Attributes::WEIGHTS_0;

            if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                size_t byte_stride = buffer_view.byteStride == 0 ? sizeof(uint8_t) * 4 : buffer_view.byteStride;

                if (byte_stride < sizeof(uint8_t) * 4) {
                    std::cout << "Error in geometry file \"" << filename << "\": Invalid WEIGHTS_0 stride." << std::endl;
                    return false;
                }

                if (byte_stride * (vertex_count - 1) + sizeof(uint8_t) * 4 > buffer_view.byteLength) {
                    std::cout << "Error in geometry file \"" << filename << "\": Invalid WEIGHTS_0 range." << std::endl;
                    return false;
                }

                for (size_t i = 0; i < vertex_count; i++) {
                    for (size_t j = 0; j < 4; j++) {
                        result.skinned_vertices[vertex_offset + i].weights[j] = *reinterpret_cast<const uint8_t*>(&buffer_data[i * byte_stride + j]);
                    }
                }
            } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                size_t byte_stride = buffer_view.byteStride == 0 ? sizeof(uint16_t) * 4 : buffer_view.byteStride;

                if (byte_stride < sizeof(uint16_t) * 4) {
                    std::cout << "Error in geometry file \"" << filename << "\": Invalid WEIGHTS_0 stride." << std::endl;
                    return false;
                }

                if (byte_stride * (vertex_count - 1) + sizeof(uint16_t) * 4 > buffer_view.byteLength) {
                    std::cout << "Error in geometry file \"" << filename << "\": Invalid WEIGHTS_0 range." << std::endl;
                    return false;
                }

                for (size_t i = 0; i < vertex_count; i++) {
                    for (size_t j = 0; j < 4; j++) {
                        uint16_t unnormalized_weight = *reinterpret_cast<const uint16_t*>(&buffer_data[i * byte_stride + j * sizeof(uint16_t)]);
                        float normalized_weight = clamp(static_cast<float>(unnormalized_weight) / UINT16_MAX, 0.f, 1.f);
                        result.skinned_vertices[vertex_offset + i].weights[j] = static_cast<uint8_t>(std::round(normalized_weight * UINT8_MAX));
                    }
                }
            } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                size_t byte_stride = buffer_view.byteStride == 0 ? sizeof(float) * 4 : buffer_view.byteStride;

                if (byte_stride < sizeof(float) * 4) {
                    std::cout << "Error in geometry file \"" << filename << "\": Invalid WEIGHTS_0 stride." << std::endl;
                    return false;
                }

                if (byte_stride * (vertex_count - 1) + sizeof(float) * 4 > buffer_view.byteLength) {
                    std::cout << "Error in geometry file \"" << filename << "\": Invalid WEIGHTS_0 range." << std::endl;
                    return false;
                }

                for (size_t i = 0; i < vertex_count; i++) {
                    for (size_t j = 0; j < 4; j++) {
                        float normalized_weight = clamp(*reinterpret_cast<const float*>(&buffer_data[i * byte_stride + j * sizeof(float)]), 0.f, 1.f);
                        result.skinned_vertices[vertex_offset + i].weights[j] = static_cast<uint8_t>(std::round(normalized_weight * UINT8_MAX));
                    }
                }
            } else {
                std::cout << "Error in geometry file \"" << filename << "\": Invalid WEIGHTS_0 component type." << std::endl;
                return false;
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

    float4x4 inverse_transform = inverse(transform);

    for (size_t i = 0; i < vertex_count; i++) {
        float3 local_position = result.vertices[vertex_offset + i].position;
        float3 position = point_transform(local_position, transform);

        float3 local_normal = result.vertices[vertex_offset + i].normal;
        float3 normal = normalize(normal_transform(local_normal, inverse_transform));

        float3 local_tangent = result.vertices[vertex_offset + i].tangent.xyz;
        float3 tangent = normalize(local_tangent * transform);

        float3 local_bitangent = cross(local_normal, local_tangent) * result.vertices[vertex_offset + i].tangent.w;
        float3 bitangent = normalize(local_bitangent * transform);

        float bitangent_factor = 1.f;
        if (dot(cross(normal, tangent), bitangent) < 0.f) {
            bitangent_factor = -1.f;
        }

        result.vertices[vertex_offset + i].position = position;
        result.vertices[vertex_offset + i].normal = normal;
        result.vertices[vertex_offset + i].tangent = float4(tangent, bitangent_factor);

        if (vertex_offset == 0 && i == 0) {
            result.bounds = aabbox(position, float3());
        } else {
            result.bounds += position;
        }
    }

    if (primitive.indices < 0 || primitive.indices >= model.accessors.size()) {
        std::cout << "Error in geometry file \"" << filename << "\": Invalid index accessor index." << std::endl;
        return false;
    }

    const tinygltf::Accessor& accessor = model.accessors[primitive.indices];

    if (accessor.type != TINYGLTF_TYPE_SCALAR) {
        std::cout << "Error in geometry file \"" << filename << "\": Index accessor must be SCALAR." << std::endl;
        return false;
    }

    if (accessor.sparse.isSparse) {
        std::cout << "Error in geometry file \"" << filename << "\": Sparse index accessors are not supported." << std::endl;
        return false;
    }

    if (accessor.count == 0) {
        std::cout << "Error in geometry file \"" << filename << "\": Empty index accessor." << std::endl;
        return false;
    }

    size_t index_offset = result.indices.size();
    size_t index_count = accessor.count;
    result.indices.resize(result.indices.size() + index_count);

    if (accessor.bufferView < 0 || accessor.bufferView >= model.bufferViews.size()) {
        std::cout << "Error in geometry file \"" << filename << "\": Invalid index buffer view index." << std::endl;
        return false;
    }

    const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];

    if (buffer_view.target != TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER) {
        std::cout << "Error in geometry file \"" << filename << "\": Invalid index buffer view target." << std::endl;
        return false;
    }

    if (buffer_view.buffer < 0 || buffer_view.buffer >= model.buffers.size()) {
        std::cout << "Error in geometry file \"" << filename << "\": Invalid index buffer index." << std::endl;
        return false;
    }

    const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];

    if (accessor.byteOffset + buffer_view.byteOffset + buffer_view.byteLength > buffer.data.size()) {
        std::cout << "Error in geometry file \"" << filename << "\": Invalid index buffer view range." << std::endl;
        return false;
    }

    const uint8_t* buffer_data = &buffer.data[accessor.byteOffset + buffer_view.byteOffset];

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        size_t byte_stride = buffer_view.byteStride == 0 ? sizeof(uint8_t) : buffer_view.byteStride;

        if (byte_stride < sizeof(uint8_t)) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid index stride." << std::endl;
            return false;
        }

        if (byte_stride * (index_count - 1) + sizeof(uint8_t) > buffer_view.byteLength) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid index range." << std::endl;
            return false;
        }

        for (size_t i = 0; i < index_count; i++) {
            result.indices[index_offset + i] = vertex_offset + *reinterpret_cast<const uint8_t*>(&buffer_data[i * byte_stride]);
        }
    } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        size_t byte_stride = buffer_view.byteStride == 0 ? sizeof(uint16_t) : buffer_view.byteStride;

        if (byte_stride < sizeof(uint16_t)) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid index stride." << std::endl;
            return false;
        }

        if (byte_stride * (index_count - 1) + sizeof(uint16_t) > buffer_view.byteLength) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid index range." << std::endl;
            return false;
        }

        for (size_t i = 0; i < index_count; i++) {
            result.indices[index_offset + i] = vertex_offset + *reinterpret_cast<const uint16_t*>(&buffer_data[i * byte_stride]);
        }
    } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
        size_t byte_stride = buffer_view.byteStride == 0 ? sizeof(uint32_t) : buffer_view.byteStride;

        if (byte_stride < sizeof(uint32_t)) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid index stride." << std::endl;
            return false;
        }

        if (byte_stride * (index_count - 1) + sizeof(uint32_t) > buffer_view.byteLength) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid index range." << std::endl;
            return false;
        }

        for (size_t i = 0; i < index_count; i++) {
            result.indices[index_offset + i] = vertex_offset + *reinterpret_cast<const uint32_t*>(&buffer_data[i * byte_stride]);
        }
    } else {
        std::cout << "Error in geometry file \"" << filename << "\": Invalid index component type." << std::endl;
        return false;
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

static bool assign_joint_parents(int node_index, uint32_t parent_index, const float4x4& parent_transform,
                                 const std::unordered_map<int, size_t>& joint_nodes)
{
    const tinygltf::Node& node = model.nodes[node_index];
    
    float4x4 local_transform;

    if (node.matrix.size() == 16) {
        local_transform = float4x4(node.matrix[0],  node.matrix[1],  node.matrix[2],  node.matrix[3],
                                    node.matrix[4],  node.matrix[5],  node.matrix[6],  node.matrix[7],
                                    node.matrix[8],  node.matrix[9],  node.matrix[10], node.matrix[11],
                                    node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]);
    } else {
        if (node.scale.size() == 3) {
            local_transform *= float4x4::scale(float3(static_cast<float>(node.scale[0]),
                                                        static_cast<float>(node.scale[1]),
                                                        static_cast<float>(node.scale[2])));
        }
        
        if (node.rotation.size() == 4) {
            local_transform *= float4x4(quaternion(static_cast<float>(node.rotation[0]),
                                                    static_cast<float>(node.rotation[1]),
                                                    static_cast<float>(node.rotation[2]),
                                                    static_cast<float>(node.rotation[3])));
        }

        if (node.translation.size() == 3) {
            local_transform *= float4x4::translation(float3(static_cast<float>(node.translation[0]),
                                                            static_cast<float>(node.translation[1]),
                                                            static_cast<float>(node.translation[2])));
        }
    }

    float4x4 transform = local_transform * parent_transform;

    auto it1 = joint_nodes.find(node_index);
    if (it1 != joint_nodes.end()) {
        parent_index = static_cast<uint32_t>(it1->second);

        result.skeleton.bind_matrices[it1->second] = transform;

        transform = float4x4();
    }

    for (int child_index : node.children) {
        if (child_index < 0 || child_index >= model.nodes.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid child index." << std::endl;
            return false;
        }

        auto it2 = joint_nodes.find(child_index);
        if (it2 != joint_nodes.end()) {
            result.skeleton.parent_joint_indices[it2->second] = parent_index;
        }

        if (!assign_joint_parents(child_index, parent_index, transform, joint_nodes)) {
            return false;
        }
    }

    return true;
}

static bool load_node(const tinygltf::Node& node, const float4x4& parent_transform) {
    float4x4 local_transform;

    if (node.matrix.size() == 16) {
        local_transform = float4x4(node.matrix[0],  node.matrix[1],  node.matrix[2],  node.matrix[3],
                                   node.matrix[4],  node.matrix[5],  node.matrix[6],  node.matrix[7],
                                   node.matrix[8],  node.matrix[9],  node.matrix[10], node.matrix[11],
                                   node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]);
    } else {
        if (node.scale.size() == 3) {
            local_transform *= float4x4::scale(float3(static_cast<float>(node.scale[0]),
                                                      static_cast<float>(node.scale[1]),
                                                      static_cast<float>(node.scale[2])));
        }
        
        if (node.rotation.size() == 4) {
            local_transform *= float4x4(quaternion(static_cast<float>(node.rotation[0]),
                                                   static_cast<float>(node.rotation[1]),
                                                   static_cast<float>(node.rotation[2]),
                                                   static_cast<float>(node.rotation[3])));
        }

        if (node.translation.size() == 3) {
            local_transform *= float4x4::translation(float3(static_cast<float>(node.translation[0]),
                                                            static_cast<float>(node.translation[1]),
                                                            static_cast<float>(node.translation[2])));
        }
    }

    float4x4 transform;

    if (node.skin >= 0) {
        if (!result.skeleton.inverse_bind_matrices.empty()) {
            std::cout << "Error in geometry file \"" << filename << "\": Multiple skins are not allowed." << std::endl;
            return false;
        }

        if (node.skin >= model.skins.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid skin index." << std::endl;
            return false;
        }

        const tinygltf::Skin& skin = model.skins[node.skin];

        if (skin.joints.empty()) {
            std::cout << "Error in geometry file \"" << filename << "\": At least one joint is required." << std::endl;
            return false;
        }

        result.skeleton.parent_joint_indices.resize(skin.joints.size());
        result.skeleton.inverse_bind_matrices.resize(skin.joints.size());
        result.skeleton.bind_matrices.resize(skin.joints.size());
        result.skeleton.joint_names.resize(skin.joints.size());

        if (skin.inverseBindMatrices >= model.accessors.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid accessor index." << std::endl;
            return false;
        }

        const tinygltf::Accessor& accessor = model.accessors[skin.inverseBindMatrices];

        if (accessor.bufferView >= model.bufferViews.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid buffer view index." << std::endl;
            return false;
        }

        if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid accessor component type." << std::endl;
            return false;
        }

        if (accessor.type != TINYGLTF_TYPE_MAT4) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid accessor type." << std::endl;
            return false;
        }

        if (accessor.count < skin.joints.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid accessor count." << std::endl;
            return false;
        }

        const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];

        if (buffer_view.buffer >= model.buffers.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid buffer index." << std::endl;
            return false;
        }

        const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];

        if (buffer_view.byteLength < skin.joints.size() * sizeof(float4x4) ||
            accessor.byteOffset + buffer_view.byteOffset + buffer_view.byteLength > buffer.data.size())
        {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid buffer view size." << std::endl;
            return false;
        }

        if (buffer_view.byteStride > 0 && buffer_view.byteStride < sizeof(float4x4)) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid buffer view byte stride." << std::endl;
            return false;
        }

        const unsigned char* data = &buffer.data[accessor.byteOffset + buffer_view.byteOffset];
        size_t byte_stride = buffer_view.byteStride == 0 ? sizeof(float4x4) : buffer_view.byteStride;

        for (size_t i = 0; i < skin.joints.size(); i++) {
            int joint_node_index = skin.joints[i];

            if (joint_node_index >= model.nodes.size()) {
                std::cout << "Error in geometry file \"" << filename << "\": Invalid joint node index." << std::endl;
                return false;
            }

            const tinygltf::Node& joint_node = model.nodes[joint_node_index];

            result.skeleton.joint_names[i] = joint_node.name;
            result.skeleton.inverse_bind_matrices[i] = *reinterpret_cast<const float4x4*>(data + byte_stride * i);
            result.skeleton.parent_joint_indices[i] = UINT32_MAX;
        }

        joint_mapping.reserve(skin.joints.size());

        for (size_t i = 0; i < skin.joints.size(); i++) {
            joint_mapping.emplace(skin.joints[i], i);
        }
    } else {
        transform = local_transform * parent_transform;
    }

    if (node.mesh >= 0) {
        if (node.mesh >= model.meshes.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid mesh index." << std::endl;
            return false;
        }

        if (!load_mesh(model.meshes[node.mesh], transform)) {
            return false;
        }
    }

    for (int child_index : node.children) {
        if (child_index < 0 || child_index >= model.nodes.size()) {
            std::cout << "Error in geometry file \"" << filename << "\": Invalid child index." << std::endl;
            return false;
        }

        if (!load_node(model.nodes[child_index], transform)) {
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

        if (!load_node(model.nodes[node_index], float4x4())) {
            return 1;
        }
    }

    for (int node_index : scene.nodes) {
        if (!joint_mapping.empty() && !assign_joint_parents(node_index, UINT32_MAX, float4x4(), joint_mapping)) {
            return 1;
        }
    }

    if (!save_result(argv[2])) {
        return 1;
    }

    return 0;
}
