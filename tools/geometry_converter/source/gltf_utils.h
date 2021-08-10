#include <core/math/float4x4.h>
#include <core/math/quaternion.h>

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

#include <array>
#include <optional>
#include <type_traits>
#include <vector>

using namespace kw;

template <typename T, size_t Size>
struct is_array_vector : std::false_type {
};

template <typename T, size_t Size>
struct is_array_vector<std::array<T, Size>, Size> : std::true_type {
};

template <typename T, size_t Size>
static constexpr bool is_array_vector_v = is_array_vector<T, Size>::value;

template <typename T, size_t Size>
struct is_array_matrix : std::false_type {
};

template <typename T, size_t Size>
struct is_array_matrix<std::array<std::array<T, Size>, Size>, Size> : std::true_type {
};

template <typename T, size_t Size>
static constexpr bool is_array_matrix_v = is_array_matrix<T, Size>::value;

template <typename T>
static int get_gltf_type() {
    if constexpr (std::is_same_v<T, float2> || (is_array_vector_v<T, 2> && !is_array_matrix_v<T, 2>)) {
        return TINYGLTF_TYPE_VEC2;
    } else if constexpr (std::is_same_v<T, float3> || (is_array_vector_v<T, 3> && !is_array_matrix_v<T, 3>)) {
        return TINYGLTF_TYPE_VEC3;
    } else if constexpr (std::is_same_v<T, float4> || (is_array_vector_v<T, 4> && !is_array_matrix_v<T, 4>)) {
        return TINYGLTF_TYPE_VEC4;
    } else if constexpr (std::is_same_v<T, float2x2> || is_array_matrix_v<T, 2>) {
        return TINYGLTF_TYPE_MAT2;
    } else if constexpr (std::is_same_v<T, float3x3> || is_array_matrix_v<T, 3>) {
        return TINYGLTF_TYPE_MAT3;
    } else if constexpr (std::is_same_v<T, float4x4> || is_array_matrix_v<T, 4>) {
        return TINYGLTF_TYPE_MAT4;
    } else if constexpr (std::is_same_v<T, quaternion>) {
        return TINYGLTF_TYPE_VEC4;
    } else {
        return TINYGLTF_TYPE_SCALAR;
    }
}

template <typename T, bool IsNormalized = false>
static bool check_gltf_component_type(int component_type) {
    if constexpr (std::is_same_v<T, float>    ||
                  std::is_same_v<T, float2>   ||
                  std::is_same_v<T, float3>   ||
                  std::is_same_v<T, float4>   ||
                  std::is_same_v<T, float2x2> ||
                  std::is_same_v<T, float3x3> ||
                  std::is_same_v<T, float4x4> ||
                  std::is_same_v<T, quaternion>)
    {
        if constexpr (IsNormalized) {
            return true;
        } else {
            return component_type == TINYGLTF_COMPONENT_TYPE_FLOAT;
        }
    } else {
        if constexpr (IsNormalized) {
            return true;
        } else {
            return component_type != TINYGLTF_COMPONENT_TYPE_FLOAT &&
                   component_type != TINYGLTF_COMPONENT_TYPE_DOUBLE;
        }
    }
}

static size_t get_gltf_type_size(int type) {
    switch (type) {
    case TINYGLTF_TYPE_VEC2:
        return 2;
    case TINYGLTF_TYPE_VEC3:
        return 3;
    case TINYGLTF_TYPE_VEC4:
        return 4;
    case TINYGLTF_TYPE_MAT2:
        return 4;
    case TINYGLTF_TYPE_MAT3:
        return 9;
    case TINYGLTF_TYPE_MAT4:
        return 16;
    default:
        return 1;
    }
}

static size_t get_gltf_component_type_size(int component_type) {
    switch (component_type) {
    case TINYGLTF_COMPONENT_TYPE_BYTE:
        return sizeof(int8_t);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        return sizeof(uint8_t);
    case TINYGLTF_COMPONENT_TYPE_SHORT:
        return sizeof(int16_t);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        return sizeof(uint16_t);
    case TINYGLTF_COMPONENT_TYPE_INT:
        return sizeof(int32_t);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        return sizeof(uint32_t);
    case TINYGLTF_COMPONENT_TYPE_FLOAT:
        return sizeof(float);
    case TINYGLTF_COMPONENT_TYPE_DOUBLE:
        return sizeof(double);
    default:
        return 0;
    }
}

template <typename T>
struct get_item_component_type {
    typedef T type;
};

template <>
struct get_item_component_type<float2> {
    typedef float type;
};

template <>
struct get_item_component_type<float3> {
    typedef float type;
};

template <>
struct get_item_component_type<float4> {
    typedef float type;
};

template <>
struct get_item_component_type<float2x2> {
    typedef float type;
};

template <>
struct get_item_component_type<float3x3> {
    typedef float type;
};

template <>
struct get_item_component_type<float4x4> {
    typedef float type;
};

template <>
struct get_item_component_type<quaternion> {
    typedef float type;
};

template <typename T, size_t N>
struct get_item_component_type<std::array<T, N>> {
    typedef typename get_item_component_type<T>::type type;
};

template <typename T>
using get_item_component_type_t = typename get_item_component_type<T>::type;

template <typename T>
static get_item_component_type_t<T>& get_item_component(T& item, size_t component_index) {
    return reinterpret_cast<get_item_component_type_t<T>*>(&item)[component_index];
}

template <typename T, bool IsNormalized, typename U>
static get_item_component_type_t<T> convert_item_component(U component) {
    typedef get_item_component_type_t<T> V;

    if constexpr (IsNormalized) {
        if constexpr (std::is_floating_point_v<V>) {
            if constexpr (std::is_floating_point_v<U>) {
                return static_cast<V>(component);
            } else {
                return static_cast<V>(component) / std::numeric_limits<U>::max();
            }
        } else {
            if constexpr (std::is_floating_point_v<U>) {
                return static_cast<V>(static_cast<float>(component) * std::numeric_limits<V>::max());
            } else {
                return static_cast<V>(static_cast<float>(component) / std::numeric_limits<U>::max() * std::numeric_limits<V>::max());
            }
        }
    } else {
        return static_cast<V>(component);
    }
}

template <typename T, bool IsNormalized = false>
static std::optional<std::vector<T>> load_gltf_accessor(int accessor_index) {
    if (accessor_index < 0 || accessor_index >= model.accessors.size()) {
        std::cout << "Error in geometry file \"" << filename << "\": Invalid accessor index." << std::endl;
        return std::nullopt;
    }

    const tinygltf::Accessor& accessor = model.accessors[accessor_index];

    if (accessor.sparse.isSparse) {
        std::cout << "Error in geometry file \"" << filename << "\": Sparse accessors are not supported." << std::endl;
        return std::nullopt;
    }

    if (accessor.bufferView >= model.bufferViews.size()) {
        std::cout << "Error in geometry file \"" << filename << "\": Invalid buffer view index." << std::endl;
        return std::nullopt;
    }

    if (!check_gltf_component_type<T, IsNormalized>(accessor.componentType)) {
        std::cout << "Error in geometry file \"" << filename << "\": Invalid accessor component type." << std::endl;
        return std::nullopt;
    }

    if (accessor.type != get_gltf_type<T>()) {
        std::cout << "Error in geometry file \"" << filename << "\": Invalid accessor type." << std::endl;
        return std::nullopt;
    }

    const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];

    if (buffer_view.buffer >= model.buffers.size()) {
        std::cout << "Error in geometry file \"" << filename << "\": Invalid buffer index." << std::endl;
        return std::nullopt;
    }

    size_t type_size = get_gltf_type_size(accessor.type);
    size_t component_type_size = get_gltf_component_type_size(accessor.componentType);
    size_t item_size = type_size * component_type_size;

    const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];

    if (buffer_view.byteLength < accessor.count * item_size ||
        accessor.byteOffset + buffer_view.byteOffset + buffer_view.byteLength > buffer.data.size())
    {
        std::cout << "Error in geometry file \"" << filename << "\": Invalid buffer view size." << std::endl;
        return std::nullopt;
    }

    if (buffer_view.byteStride > 0 && buffer_view.byteStride < item_size) {
        std::cout << "Error in geometry file \"" << filename << "\": Invalid buffer view byte stride." << std::endl;
        return std::nullopt;
    }

    size_t byte_stride = buffer_view.byteStride == 0 ? item_size : buffer_view.byteStride;

    if (accessor.count > 0 && byte_stride * (accessor.count - 1) + item_size > buffer_view.byteLength) {
        std::cout << "Error in geometry file \"" << filename << "\": Invalid buffer view stride." << std::endl;
        return std::nullopt;
    }

    std::vector<T> result(accessor.count);

    for (size_t i = 0; i < accessor.count; i++) {
        for (size_t j = 0; j < type_size; j++) {
            get_item_component_type_t<T>& target_component = get_item_component(result[i], j);
            const unsigned char* source_component = &buffer.data[accessor.byteOffset + buffer_view.byteOffset + i * byte_stride + j * component_type_size];

            switch (accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_BYTE:
                target_component = convert_item_component<T, IsNormalized>(*reinterpret_cast<const int8_t*>(source_component));
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                target_component = convert_item_component<T, IsNormalized>(*reinterpret_cast<const uint8_t*>(source_component));
                break;
            case TINYGLTF_COMPONENT_TYPE_SHORT:
                target_component = convert_item_component<T, IsNormalized>(*reinterpret_cast<const int16_t*>(source_component));
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                target_component = convert_item_component<T, IsNormalized>(*reinterpret_cast<const uint16_t*>(source_component));
                break;
            case TINYGLTF_COMPONENT_TYPE_INT:
                target_component = convert_item_component<T, IsNormalized>(*reinterpret_cast<const int32_t*>(source_component));
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                target_component = convert_item_component<T, IsNormalized>(*reinterpret_cast<const uint32_t*>(source_component));
                break;
            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                target_component = convert_item_component<T, IsNormalized>(*reinterpret_cast<const float*>(source_component));
                break;
            case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                target_component = convert_item_component<T, IsNormalized>(*reinterpret_cast<const double*>(source_component));
                break;
            }
        }
    }

    return result;
}

static float4x4 get_node_transform(const tinygltf::Node& node) {
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

    return local_transform;
}
