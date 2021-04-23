#include <core/endian_utils.h>
#include <core/enum.h>
#include <core/math.h>

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

#include <fstream>
#include <iostream>

using namespace kw;

struct Vertex {
    float3 position;
    float3 normal;
    float4 tangent;
    float2 texcoord_0;
};

struct Geometry {
    Geometry()
        : bounds(float3(FLT_MAX, FLT_MAX, FLT_MAX), float3(-FLT_MAX, -FLT_MAX, -FLT_MAX))
    {
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    AABBox bounds;
};

enum class Attributes {
    NONE     = 0,
    POSITION = 1 << 0,
    NORMAL   = 1 << 1,
    TANGENT  = 1 << 2,
    TEXCOORD_0 = 1 << 3,
};

KW_DEFINE_ENUM_BITMASK(Attributes);

enum class ChunkType : uint16_t {
    VERTICES  = 0,
    INDICES16 = 10,
    INDICES32 = 11,
    BOUNDS    = 20,
};

static tinygltf::TinyGLTF gltf;
static tinygltf::Model model;
static std::string error;
static std::string warning;
static std::string filename;
static Geometry result;

template <typename T>
void swap_le(T* values, size_t size) {
    for (size_t i = 0; i < size; i++) {
        values[i] = EndianUtils::swap_le(values[i]);
    }
}

template <>
void swap_le<Vertex>(Vertex* values, size_t size) {
    for (size_t i = 0; i < size; i++) {
        swap_le(values[i].position.data, std::size(values[i].position.data));
        swap_le(values[i].normal.data, std::size(values[i].normal.data));
        swap_le(values[i].tangent.data, std::size(values[i].tangent.data));
        swap_le(values[i].texcoord_0.data, std::size(values[i].texcoord_0.data));
    }
}

template <typename T>
void write(std::ofstream& stream, T* values, size_t count) {
    swap_le(values, count);
    stream.write(reinterpret_cast<const char*>(values), sizeof(T) * count);
}

template <typename T, typename U>
void write(std::ofstream& stream, const U& uvalue) {
    T tvalue = static_cast<T>(uvalue);
    write(stream, &tvalue, 1);
}

static bool save_result(const std::string& output_filename) {
    std::ofstream stream(output_filename, std::ofstream::binary);

    if (!stream) {
        std::cout << "Failed to open output geometry file \"" << output_filename << "\"." << std::endl;
        return false;
    }

    //
    // Vertices.
    //

    {
        write<uint16_t>(stream, ChunkType::VERTICES);
        write<uint32_t>(stream, sizeof(uint32_t) + sizeof(Vertex) * result.vertices.size());

        write<uint32_t>(stream, result.vertices.size());
        write<Vertex>(stream, result.vertices.data(), result.vertices.size());
    }

    //
    // Indices.
    //

    if (result.vertices.size() < UINT16_MAX) {
        write<uint16_t>(stream, ChunkType::INDICES16);
        write<uint32_t>(stream, sizeof(uint32_t) + sizeof(uint16_t) * result.indices.size());

        write<uint32_t>(stream, result.indices.size());

        for (uint32_t index : result.indices) {
            write<uint16_t>(stream, index);
        }
    } else {
        write<uint16_t>(stream, ChunkType::INDICES32);
        write<uint32_t>(stream, sizeof(uint32_t) + sizeof(uint32_t) * result.indices.size());

        write<uint32_t>(stream, result.indices.size());
        write<uint32_t>(stream, result.indices.data(), result.indices.size());
    }

    //
    // Bounds.
    //

    {
        write<uint16_t>(stream, ChunkType::BOUNDS);
        write<uint32_t>(stream, sizeof(AABBox));

        write<float>(stream, result.bounds.data, std::size(result.bounds.data));
    }

    if (!stream) {
        std::cout << "Failed to write to output geometry file \"" << output_filename << "\"." << std::endl;
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
        }
    }

    if (attribute_mask != (Attributes::POSITION | Attributes::NORMAL | Attributes::TANGENT | Attributes::TEXCOORD_0)) {
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

        result.bounds.min.x = std::min(result.bounds.min.x, position.x);
        result.bounds.min.y = std::min(result.bounds.min.y, position.y);
        result.bounds.min.z = std::min(result.bounds.min.z, position.z);
        result.bounds.max.x = std::max(result.bounds.max.x, position.x);
        result.bounds.max.y = std::max(result.bounds.max.y, position.y);
        result.bounds.max.z = std::max(result.bounds.max.z, position.z);
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
            local_transform *= quaternion::to_matrix(quaternion(static_cast<float>(node.rotation[0]),
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

    float4x4 transform = parent_transform * local_transform;

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

    if (!save_result(argv[2])) {
        return 1;
    }

    return 0;
}
