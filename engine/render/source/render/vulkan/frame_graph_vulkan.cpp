#include "render/vulkan/frame_graph_vulkan.h"
#include "render/vulkan/render_vulkan.h"
#include "render/vulkan/timeline_semaphore.h"
#include "render/vulkan/vulkan_utils.h"

#include <system/window.h>

#include <core/filesystem_utils.h>
#include <core/math.h>

#include <concurrency/thread_pool.h>

#include <debug/assert.h>
#include <debug/log.h>

#include <SDL2/SDL_vulkan.h>

#include <algorithm>
#include <numeric>
#include <thread>

namespace kw {

static const char* SEMANTIC_STRINGS[] = {
    "POSITION",
    "COLOR",
    "TEXCOORD",
    "NORMAL",
    "BINORMAL",
    "TANGENT",
    "JOINTS",
    "WEIGHTS",
};

static_assert(std::size(SEMANTIC_STRINGS) == SEMANTIC_COUNT);

static const VkPrimitiveTopology PRIMITIVE_TOPOLOGY_MAPPING[] = {
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,  // TRIANGLE_LIST
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // TRIANGLE_STRIP
    VK_PRIMITIVE_TOPOLOGY_LINE_LIST,      // LINE_LIST
    VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,     // LINE_STRIP
    VK_PRIMITIVE_TOPOLOGY_POINT_LIST,     // POINT_LIST
};

static_assert(std::size(PRIMITIVE_TOPOLOGY_MAPPING) == PRIMITIVE_TOPOLOGY_COUNT);

static const VkPolygonMode FILL_MODE_MAPPING[] = {
    VK_POLYGON_MODE_FILL,  // FILL
    VK_POLYGON_MODE_LINE,  // LINE
    VK_POLYGON_MODE_POINT, // POINT
};

static_assert(std::size(FILL_MODE_MAPPING) == FILL_MODE_COUNT);

static const VkCullModeFlags CULL_MODE_MAPPING[] = {
    VK_CULL_MODE_BACK_BIT,  // BACK
    VK_CULL_MODE_FRONT_BIT, // FRONT
    VK_CULL_MODE_NONE,      // NONE
};

static_assert(std::size(CULL_MODE_MAPPING) == CULL_MODE_COUNT);

static const VkFrontFace FRONT_FACE_MAPPING[] = {
    VK_FRONT_FACE_COUNTER_CLOCKWISE, // COUNTER_CLOCKWISE
    VK_FRONT_FACE_CLOCKWISE,         // CLOCKWISE
};

static_assert(std::size(FRONT_FACE_MAPPING) == FRONT_FACE_COUNT);

static const VkStencilOp STENCIL_OP_MAPPING[] = {
    VK_STENCIL_OP_KEEP,                // KEEP
    VK_STENCIL_OP_ZERO,                // ZERO
    VK_STENCIL_OP_REPLACE,             // REPLACE
    VK_STENCIL_OP_INCREMENT_AND_CLAMP, // INCREMENT_AND_CLAMP
    VK_STENCIL_OP_DECREMENT_AND_CLAMP, // DECREMENT_AND_CLAMP
    VK_STENCIL_OP_INVERT,              // INVERT
    VK_STENCIL_OP_INCREMENT_AND_WRAP,  // INCREMENT_AND_WRAP
    VK_STENCIL_OP_DECREMENT_AND_WRAP,  // DECREMENT_AND_WRAP
};

static_assert(std::size(STENCIL_OP_MAPPING) == STENCIL_OP_COUNT);

static const VkCompareOp COMPARE_OP_MAPPING[] = {
    VK_COMPARE_OP_NEVER,            // NEVER
    VK_COMPARE_OP_LESS,             // LESS
    VK_COMPARE_OP_EQUAL,            // EQUAL
    VK_COMPARE_OP_LESS_OR_EQUAL,    // LESS_OR_EQUAL
    VK_COMPARE_OP_GREATER,          // GREATER
    VK_COMPARE_OP_NOT_EQUAL,        // NOT_EQUAL
    VK_COMPARE_OP_GREATER_OR_EQUAL, // GREATER_OR_EQUAL
    VK_COMPARE_OP_ALWAYS,           // ALWAYS
};

static_assert(std::size(COMPARE_OP_MAPPING) == COMPARE_OP_COUNT);

static const VkBlendFactor BLEND_FACTOR_MAPPING[] = {
    VK_BLEND_FACTOR_ZERO,                // ZERO
    VK_BLEND_FACTOR_ONE,                 // ONE
    VK_BLEND_FACTOR_SRC_COLOR,           // SOURCE_COLOR
    VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, // SOURCE_INVERSE_COLOR
    VK_BLEND_FACTOR_SRC_ALPHA,           // SOURCE_ALPHA
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // SOURCE_INVERSE_ALPHA
    VK_BLEND_FACTOR_DST_COLOR,           // DESTINATION_COLOR
    VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR, // DESTINATION_INVERSE_COLOR
    VK_BLEND_FACTOR_DST_ALPHA,           // DESTINATION_ALPHA
    VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, // DESTINATION_INVERSE_ALPHA
};

static_assert(std::size(BLEND_FACTOR_MAPPING) == BLEND_FACTOR_COUNT);

static const VkBlendOp BLEND_OP_MAPPING[] = {
    VK_BLEND_OP_ADD,              // ADD
    VK_BLEND_OP_SUBTRACT,         // SUBTRACT
    VK_BLEND_OP_REVERSE_SUBTRACT, // REVERSE_SUBTRACT
    VK_BLEND_OP_MIN,              // MIN
    VK_BLEND_OP_MAX,              // MAX
};

static_assert(std::size(BLEND_OP_MAPPING) == BLEND_OP_COUNT);

static const VkShaderStageFlags SHADER_VISILITY_MAPPING[] = {
    VK_SHADER_STAGE_ALL_GRAPHICS, // ALL
    VK_SHADER_STAGE_VERTEX_BIT,   // VERTEX
    VK_SHADER_STAGE_FRAGMENT_BIT, // FRAGMENT
};

static_assert(std::size(SHADER_VISILITY_MAPPING) == SHADER_VISILITY_COUNT);

static const VkFilter FILTER_MAPPING[] = {
    VK_FILTER_LINEAR,  // LINEAR
    VK_FILTER_NEAREST, // NEAREST
};

static_assert(std::size(FILTER_MAPPING) == FILTER_COUNT);

static const VkSamplerMipmapMode MIP_FILTER_MAPPING[] = {
    VK_SAMPLER_MIPMAP_MODE_LINEAR,  // LINEAR
    VK_SAMPLER_MIPMAP_MODE_NEAREST, // NEAREST
};

static_assert(std::size(MIP_FILTER_MAPPING) == FILTER_COUNT);

static const VkSamplerAddressMode ADDRESS_MODE_MAPPING[] = {
    VK_SAMPLER_ADDRESS_MODE_REPEAT,          // WRAP
    VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, // MIRROR
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,   // CLAMP
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, // BORDER
};

static_assert(std::size(ADDRESS_MODE_MAPPING) == ADDRESS_MODE_COUNT);

static const VkBorderColor BORDER_COLOR_MAPPING[] = {
    VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK, // FLOAT_TRANSPARENT_BLACK
    VK_BORDER_COLOR_INT_TRANSPARENT_BLACK,   // INT_TRANSPARENT_BLACK
    VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,      // FLOAT_OPAQUE_BLACK
    VK_BORDER_COLOR_INT_OPAQUE_BLACK,        // INT_OPAQUE_BLACK
    VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,      // FLOAT_OPAQUE_WHITE
    VK_BORDER_COLOR_INT_OPAQUE_WHITE,        // INT_OPAQUE_WHITE
};

static_assert(std::size(BORDER_COLOR_MAPPING) == BORDER_COLOR_COUNT);

static const VkAttachmentLoadOp LOAD_OP_MAPPING[] = {
    VK_ATTACHMENT_LOAD_OP_CLEAR,     // CLEAR
    VK_ATTACHMENT_LOAD_OP_DONT_CARE, // DONT_CARE
    VK_ATTACHMENT_LOAD_OP_LOAD,      // LOAD
};

static_assert(std::size(LOAD_OP_MAPPING) == LOAD_OP_COUNT);

static const VkDynamicState DYNAMIC_STATES[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
    VK_DYNAMIC_STATE_STENCIL_REFERENCE,
};

FrameGraphVulkan::GraphicsPipelineData::GraphicsPipelineData(MemoryResource& memory_resource)
    : samplers(memory_resource)
    , attachment_indices(memory_resource) {
}

FrameGraphVulkan::RenderPassData::RenderPassData(MemoryResource& memory_resource)
    : graphics_pipeline_data(memory_resource)
    , attachment_indices(memory_resource) {
}

FrameGraphVulkan::CommandPoolData::CommandPoolData(MemoryResource& memory_resource)
    : command_buffers(memory_resource) {
}

static void* spv_calloc(void* context, size_t count, size_t size) {
    MemoryResource* memory_resource = static_cast<MemoryResource*>(context);
    KW_ASSERT(memory_resource != nullptr);

    void* result = memory_resource->allocate(count * size, 1);
    std::memset(result, 0, count * size);
    return result;
}

static void spv_free(void* context, void* memory) {
    MemoryResource* memory_resource = static_cast<MemoryResource*>(context);
    KW_ASSERT(memory_resource != nullptr);

    memory_resource->deallocate(memory);
}

#ifdef KW_FRAME_GRAPH_LOG

static std::unordered_map<TextureFormat, const char*> TEXTURE_FORMAT_STRING_MAPPING = {
    { TextureFormat::UNKNOWN,              "UNKNOWN"              },
    { TextureFormat::R8_SINT,              "R8_SINT"              },
    { TextureFormat::R8_SNORM,             "R8_SNORM"             },
    { TextureFormat::R8_UINT,              "R8_UINT"              },
    { TextureFormat::R8_UNORM,             "R8_UNORM"             },
    { TextureFormat::RG8_SINT,             "RG8_SINT"             },
    { TextureFormat::RG8_SNORM,            "RG8_SNORM"            },
    { TextureFormat::RG8_UINT,             "RG8_UINT"             },
    { TextureFormat::RG8_UNORM,            "RG8_UNORM"            },
    { TextureFormat::RGBA8_SINT,           "RGBA8_SINT"           },
    { TextureFormat::RGBA8_SNORM,          "RGBA8_SNORM"          },
    { TextureFormat::RGBA8_UINT,           "RGBA8_UINT"           },
    { TextureFormat::RGBA8_UNORM,          "RGBA8_UNORM"          },
    { TextureFormat::RGBA8_UNORM_SRGB,     "RGBA8_UNORM_SRGB"     },
    { TextureFormat::R16_FLOAT,            "R16_FLOAT"            },
    { TextureFormat::R16_SINT,             "R16_SINT"             },
    { TextureFormat::R16_SNORM,            "R16_SNORM"            },
    { TextureFormat::R16_UINT,             "R16_UINT"             },
    { TextureFormat::R16_UNORM,            "R16_UNORM"            },
    { TextureFormat::RG16_FLOAT,           "RG16_FLOAT"           },
    { TextureFormat::RG16_SINT,            "RG16_SINT"            },
    { TextureFormat::RG16_SNORM,           "RG16_SNORM"           },
    { TextureFormat::RG16_UINT,            "RG16_UINT"            },
    { TextureFormat::RG16_UNORM,           "RG16_UNORM"           },
    { TextureFormat::RGBA16_FLOAT,         "RGBA16_FLOAT"         },
    { TextureFormat::RGBA16_SINT,          "RGBA16_SINT"          },
    { TextureFormat::RGBA16_SNORM,         "RGBA16_SNORM"         },
    { TextureFormat::RGBA16_UINT,          "RGBA16_UINT"          },
    { TextureFormat::RGBA16_UNORM,         "RGBA16_UNORM"         },
    { TextureFormat::R32_FLOAT,            "R32_FLOAT"            },
    { TextureFormat::R32_SINT,             "R32_SINT"             },
    { TextureFormat::R32_UINT,             "R32_UINT"             },
    { TextureFormat::RG32_FLOAT,           "RG32_FLOAT"           },
    { TextureFormat::RG32_SINT,            "RG32_SINT"            },
    { TextureFormat::RG32_UINT,            "RG32_UINT"            },
    { TextureFormat::RGBA32_FLOAT,         "RGBA32_FLOAT"         },
    { TextureFormat::RGBA32_SINT,          "RGBA32_SINT"          },
    { TextureFormat::RGBA32_UINT,          "RGBA32_UINT"          },
    { TextureFormat::BGRA8_UNORM,          "BGRA8_UNORM"          },
    { TextureFormat::BGRA8_UNORM_SRGB,     "BGRA8_UNORM_SRGB"     },
    { TextureFormat::D16_UNORM,            "D16_UNORM"            },
    { TextureFormat::D24_UNORM_S8_UINT,    "D24_UNORM_S8_UINT"    },
    { TextureFormat::D32_FLOAT,            "D32_FLOAT"            },
    { TextureFormat::D32_FLOAT_S8X24_UINT, "D32_FLOAT_S8X24_UINT" },
    { TextureFormat::BC1_UNORM,            "BC1_UNORM"            },
    { TextureFormat::BC1_UNORM_SRGB,       "BC1_UNORM_SRGB"       },
    { TextureFormat::BC2_UNORM,            "BC2_UNORM"            },
    { TextureFormat::BC2_UNORM_SRGB,       "BC2_UNORM_SRGB"       },
    { TextureFormat::BC3_UNORM,            "BC3_UNORM"            },
    { TextureFormat::BC3_UNORM_SRGB,       "BC3_UNORM_SRGB"       },
    { TextureFormat::BC4_SNORM,            "BC4_SNORM"            },
    { TextureFormat::BC4_UNORM,            "BC4_UNORM"            },
    { TextureFormat::BC5_SNORM,            "BC5_SNORM"            },
    { TextureFormat::BC5_UNORM,            "BC5_UNORM"            },
    { TextureFormat::BC6H_SF16,            "BC6H_SF16"            },
    { TextureFormat::BC6H_UF16,            "BC6H_UF16"            },
    { TextureFormat::BC7_UNORM,            "BC7_UNORM"            },
    { TextureFormat::BC7_UNORM_SRGB,       "BC7_UNORM_SRGB"       },
};

static std::unordered_map<SizeClass, const char*> SIZE_CLASS_STRING_MAPPING = {
    { SizeClass::RELATIVE, "RELATIVE" },
    { SizeClass::ABSOLUTE, "ABSOLUTE" },
};

static std::unordered_map<LoadOp, const char*> LOAD_OP_STRING_MAPPING = {
    { LoadOp::CLEAR,     "CLEAR"     },
    { LoadOp::DONT_CARE, "DONT_CARE" },
    { LoadOp::LOAD,      "LOAD"      },
};

static std::unordered_map<VkAttachmentLoadOp, const char*> ATTACHMENT_LOAD_OP_STRING_MAPPING = {
    { VK_ATTACHMENT_LOAD_OP_LOAD,      "LOAD"      },
    { VK_ATTACHMENT_LOAD_OP_CLEAR,     "CLEAR"     },
    { VK_ATTACHMENT_LOAD_OP_DONT_CARE, "DONT_CARE" },
};


static std::unordered_map<VkAttachmentStoreOp, const char*> ATTACHMENT_STORE_OP_STRING_MAPPING = {
    { VK_ATTACHMENT_STORE_OP_STORE,     "STORE"     },
    { VK_ATTACHMENT_STORE_OP_DONT_CARE, "DONT_CARE" },
};

static std::unordered_map<VkImageLayout, const char*> IMAGE_LAYOUT_STRING_MAPPING = {
    { VK_IMAGE_LAYOUT_UNDEFINED,                                  "UNDEFINED"                                  },
    { VK_IMAGE_LAYOUT_GENERAL,                                    "GENERAL"                                    },
    { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,                   "COLOR_ATTACHMENT_OPTIMAL"                   },
    { VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,           "DEPTH_STENCIL_ATTACHMENT_OPTIMAL"           },
    { VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,            "DEPTH_STENCIL_READ_ONLY_OPTIMAL"            },
    { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,                   "SHADER_READ_ONLY_OPTIMAL"                   },
    { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,                       "TRANSFER_SRC_OPTIMAL"                       },
    { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,                       "TRANSFER_DST_OPTIMAL"                       },
    { VK_IMAGE_LAYOUT_PREINITIALIZED,                             "PREINITIALIZED"                             },
    { VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL, "DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL" },
    { VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL, "DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL" },
    { VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,                   "DEPTH_ATTACHMENT_OPTIMAL"                   },
    { VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL,                    "DEPTH_READ_ONLY_OPTIMAL"                    },
    { VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL,                 "STENCIL_ATTACHMENT_OPTIMAL"                 },
    { VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL,                  "STENCIL_READ_ONLY_OPTIMAL"                  },
    { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                            "PRESENT_SRC_KHR"                            },
    { VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR,                         "SHARED_PRESENT_KHR"                         },
    { VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV,                    "SHADING_RATE_OPTIMAL_NV"                    },
    { VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT,           "FRAGMENT_DENSITY_MAP_OPTIMAL_EXT"           },
};

static std::unordered_map<SpvReflectDescriptorType, const char*> DESCRIPTOR_TYPE_STRING_MAPPING = {
    { SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER,                    "SAMPLER"                    },
    { SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,     "COMBINED_IMAGE_SAMPLER"     },
    { SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE,              "SAMPLED_IMAGE"              },
    { SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE,              "STORAGE_IMAGE"              },
    { SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,       "UNIFORM_TEXEL_BUFFER"       },
    { SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,       "STORAGE_TEXEL_BUFFER"       },
    { SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             "UNIFORM_BUFFER"             },
    { SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER,             "STORAGE_BUFFER"             },
    { SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,     "UNIFORM_BUFFER_DYNAMIC"     },
    { SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,     "STORAGE_BUFFER_DYNAMIC"     },
    { SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,           "INPUT_ATTACHMENT"           },
    { SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, "ACCELERATION_STRUCTURE_KHR" },
};

static std::unordered_map<SpvDim, const char*> DIMENSION_STRING_MAPPING = {
    { SpvDim1D,          "1D"          },
    { SpvDim2D,          "2D"          },
    { SpvDim3D,          "3D"          },
    { SpvDimCube,        "Cube"        },
    { SpvDimRect,        "Rect"        },
    { SpvDimBuffer,      "Buffer"      },
    { SpvDimSubpassData, "SubpassData" },
};

static std::unordered_map<SpvReflectFormat, const char*> FORMAT_STRING_MAPPING = {
    { SPV_REFLECT_FORMAT_UNDEFINED,           "UNDEFINED"     },
    { SPV_REFLECT_FORMAT_R32_UINT,            "R32_UINT"      },
    { SPV_REFLECT_FORMAT_R32_SINT,            "R32_SINT"      },
    { SPV_REFLECT_FORMAT_R32_SFLOAT,          "R32_SFLOAT"    },
    { SPV_REFLECT_FORMAT_R32G32_UINT,         "RG32_UINT"     },
    { SPV_REFLECT_FORMAT_R32G32_SINT,         "RG32_SINT"     },
    { SPV_REFLECT_FORMAT_R32G32_SFLOAT,       "RG32_SFLOAT"   },
    { SPV_REFLECT_FORMAT_R32G32B32_UINT,      "RGB32_UINT"    },
    { SPV_REFLECT_FORMAT_R32G32B32_SINT,      "RGB32_SINT"    },
    { SPV_REFLECT_FORMAT_R32G32B32_SFLOAT,    "RGB32_SFLOAT"  },
    { SPV_REFLECT_FORMAT_R32G32B32A32_UINT,   "RGBA32_UINT"   },
    { SPV_REFLECT_FORMAT_R32G32B32A32_SINT,   "RGBA32_SINT"   },
    { SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT, "RGBA32_SFLOAT" },
    { SPV_REFLECT_FORMAT_R64_UINT,            "R64_UINT"      },
    { SPV_REFLECT_FORMAT_R64_SINT,            "R64_SINT"      },
    { SPV_REFLECT_FORMAT_R64_SFLOAT,          "R64_SFLOAT"    },
    { SPV_REFLECT_FORMAT_R64G64_UINT,         "RG64_UINT"     },
    { SPV_REFLECT_FORMAT_R64G64_SINT,         "RG64_SINT"     },
    { SPV_REFLECT_FORMAT_R64G64_SFLOAT,       "RG64_SFLOAT"   },
    { SPV_REFLECT_FORMAT_R64G64B64_UINT,      "RGB64_UINT"    },
    { SPV_REFLECT_FORMAT_R64G64B64_SINT,      "RGB64_SINT"    },
    { SPV_REFLECT_FORMAT_R64G64B64_SFLOAT,    "RGB64_SFLOAT"  },
    { SPV_REFLECT_FORMAT_R64G64B64A64_UINT,   "RGBA64_UINT"   },
    { SPV_REFLECT_FORMAT_R64G64B64A64_SINT,   "RGBA64_SINT"   },
    { SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT, "RGBA64_SFLOAT" },
};

template <typename T>
const char* enum_to_string(T value);

#define DEFINE_ENUM_TO_STRING(mapping)                                                       \
template <>                                                                                  \
const char* enum_to_string<decltype(mapping)::key_type>(decltype(mapping)::key_type value) { \
    auto it = mapping.find(value);                                                           \
    if (it != mapping.end()) {                                                               \
        return it->second;                                                                   \
    }                                                                                        \
    return "undefined";                                                                      \
}

DEFINE_ENUM_TO_STRING(TEXTURE_FORMAT_STRING_MAPPING)
DEFINE_ENUM_TO_STRING(SIZE_CLASS_STRING_MAPPING)
DEFINE_ENUM_TO_STRING(LOAD_OP_STRING_MAPPING)
DEFINE_ENUM_TO_STRING(ATTACHMENT_LOAD_OP_STRING_MAPPING)
DEFINE_ENUM_TO_STRING(ATTACHMENT_STORE_OP_STRING_MAPPING)
DEFINE_ENUM_TO_STRING(IMAGE_LAYOUT_STRING_MAPPING)
DEFINE_ENUM_TO_STRING(DESCRIPTOR_TYPE_STRING_MAPPING)
DEFINE_ENUM_TO_STRING(DIMENSION_STRING_MAPPING)
DEFINE_ENUM_TO_STRING(FORMAT_STRING_MAPPING)

#undef DEFINE_ENUM_TO_STRING

static void log_shader_reflection(SpvReflectShaderModule& shader_module, MemoryResource& memory_resource) {
    uint32_t descriptor_binding_count;
    SPV_ERROR(
        spvReflectEnumerateDescriptorBindings(&shader_module, &descriptor_binding_count, nullptr),
        "Failed to enumerate descriptor bindings."
    );

    Vector<SpvReflectDescriptorBinding*> descriptor_bindings(descriptor_binding_count, memory_resource);
    SPV_ERROR(
        spvReflectEnumerateDescriptorBindings(&shader_module, &descriptor_binding_count, descriptor_bindings.data()),
        "Failed to enumerate descriptor bindings."
    );

    Log::print("[Frame Graph]     * Descriptor Bindings:");

    for (SpvReflectDescriptorBinding* descriptor_binding : descriptor_bindings) {
        Log::print("[Frame Graph]       * Name: \"%s\"", descriptor_binding->name);
        Log::print("[Frame Graph]         Set: %u", descriptor_binding->set);
        Log::print("[Frame Graph]         Binding: %u", descriptor_binding->binding);
        Log::print("[Frame Graph]         Descriptor type: %s", enum_to_string(descriptor_binding->descriptor_type));

        if (descriptor_binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
            descriptor_binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
            descriptor_binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
            Log::print("[Frame Graph]         Image dimensions: %s", enum_to_string(descriptor_binding->image.dim));
        }

        if (descriptor_binding->array.dims_count > 0) {
            Log::print("[Frame Graph]         Array dimensions:");
            for (uint32_t dimension = 0; dimension < descriptor_binding->array.dims_count; dimension++) {
                Log::print("[Frame Graph]         * %u", descriptor_binding->array.dims[dimension]);
            }
        }
    }

    uint32_t input_variable_count;
    SPV_ERROR(
        spvReflectEnumerateInputVariables(&shader_module, &input_variable_count, nullptr),
        "Failed to enumerate input variables."
    );

    Vector<SpvReflectInterfaceVariable*> input_variables(input_variable_count, memory_resource);
    SPV_ERROR(
        spvReflectEnumerateInputVariables(&shader_module, &input_variable_count, input_variables.data()),
        "Failed to enumerate input variables."
    );

    Log::print("[Frame Graph]       Input variables:");

    for (SpvReflectInterfaceVariable* input_variable : input_variables) {
        Log::print("[Frame Graph]       * Location: %u", input_variable->location);
        Log::print("[Frame Graph]         Semantic: \"%s\"", input_variable->semantic);
        Log::print("[Frame Graph]         Format: %s", enum_to_string(input_variable->format));
    }

    uint32_t output_variable_count;
    SPV_ERROR(
        spvReflectEnumerateOutputVariables(&shader_module, &output_variable_count, nullptr),
        "Failed to enumerate output variables."
    );

    Vector<SpvReflectInterfaceVariable*> output_variables(output_variable_count, memory_resource);
    SPV_ERROR(
        spvReflectEnumerateOutputVariables(&shader_module, &output_variable_count, output_variables.data()),
        "Failed to enumerate output variables."
    );

    Log::print("[Frame Graph]       Output variables:");

    for (SpvReflectInterfaceVariable* output_variable : output_variables) {
        Log::print("[Frame Graph]       * Location: %u", output_variable->location);
        Log::print("[Frame Graph]         Semantic: \"%s\"", output_variable->semantic);
        Log::print("[Frame Graph]         Format: %s", enum_to_string(output_variable->format));
    }
}

#endif

FrameGraphVulkan::FrameGraphVulkan(const FrameGraphDescriptor& descriptor)
    : m_render(static_cast<RenderVulkan*>(descriptor.render))
    , m_window(descriptor.window)
    , m_thread_pool(descriptor.thread_pool)
    , m_attachment_data(m_render->persistent_memory_resource)
    , m_attachment_descriptors(m_render->persistent_memory_resource)
    , m_allocation_data(m_render->persistent_memory_resource)
    , m_render_pass_data(m_render->persistent_memory_resource)
    , m_parallel_block_data(m_render->persistent_memory_resource)
    , m_command_pool_data{
        Vector<CommandPoolData>(m_render->persistent_memory_resource),
        Vector<CommandPoolData>(m_render->persistent_memory_resource),
        Vector<CommandPoolData>(m_render->persistent_memory_resource),
    }
{
    create_lifetime_resources(descriptor);
    create_temporary_resources();
}

FrameGraphVulkan::~FrameGraphVulkan() {
    VK_ERROR(vkDeviceWaitIdle(m_render->device), "Failed to wait idle.");
    
    destroy_temporary_resources();
    destroy_lifetime_resources();
}

void FrameGraphVulkan::render() {
    //
    // Check whether there's a swapchain to render to.
    //

    if (m_swapchain == VK_NULL_HANDLE) {
        recreate_swapchain();

        if (m_swapchain == VK_NULL_HANDLE) {
            // Most likely the window is minimized.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return;
        }
    }

    //
    // Wait until command buffers are available for new submission.
    //

    size_t semaphore_index = m_semaphore_index++ % SWAPCHAIN_IMAGE_COUNT;
    uint64_t semaphore_value = m_render_finished_timeline_semaphores[semaphore_index]->value;

    VkSemaphoreWaitInfo semaphore_wait_info{};
    semaphore_wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    semaphore_wait_info.flags = 0;
    semaphore_wait_info.semaphoreCount = 1;
    semaphore_wait_info.pSemaphores = &m_render_finished_timeline_semaphores[semaphore_index]->semaphore;
    semaphore_wait_info.pValues = &semaphore_value;

    VK_ERROR(
        m_render->wait_semaphores(m_render->device, &semaphore_wait_info, UINT64_MAX),
        "Failed to wait for a frame semaphore %zu.", semaphore_index
    );

    //
    // Wait until swapchain image is available for render.
    //

    uint32_t swapchain_image_index;

    VkResult acquire_result = vkAcquireNextImageKHR(m_render->device, m_swapchain, UINT64_MAX, m_image_acquired_binary_semaphores[semaphore_index], VK_NULL_HANDLE, &swapchain_image_index);
    if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain();

        // Semaphore wasn't signaled, so we'd need another acquire.
        return;
    } else if (acquire_result != VK_SUBOPTIMAL_KHR) {
        VK_ERROR(acquire_result, "Failed to acquire a swapchain image.");
    }

    //
    // Increment timeline semaphore value, which provides a guarantee that no resources available right now
    // will be destroyed until the frame execution on device is finished.
    //

    m_render_finished_timeline_semaphores[semaphore_index]->value++;

    //
    // Reset command pools.
    //

    for (size_t thread_index = 0; thread_index < m_command_pool_data[semaphore_index].size(); thread_index++) {
        CommandPoolData& command_pool_data = m_command_pool_data[semaphore_index][thread_index];

        KW_ASSERT(command_pool_data.command_pool != VK_NULL_HANDLE);
        VK_ERROR(
            vkResetCommandPool(m_render->device, command_pool_data.command_pool, 0),
            "Failed to reset frame command pool %zu-%zu.", semaphore_index, thread_index
        );
    }

    //
    // Execute render passes in parallel.
    //

    // Current command buffer index for each render pass.
    Vector<size_t> command_buffer_indices(m_thread_pool->get_count(), m_render->transient_memory_resource);

    // Assign command buffer to each render pass in parallel and then collect them into a single submit.
    Vector<VkCommandBuffer> render_pass_command_buffers(m_render_pass_data.size(), m_render->transient_memory_resource);

    m_thread_pool->parallel_for([&](size_t render_pass_index, size_t thread_index) {
        RenderPassData& render_pass_data = m_render_pass_data[render_pass_index];
        KW_ASSERT(render_pass_data.render_pass != VK_NULL_HANDLE);
        KW_ASSERT(render_pass_data.parallel_block_index < m_parallel_block_data.size());

        //
        // Find or create a command buffer.
        //

        KW_ASSERT(thread_index < command_buffer_indices.size());
        size_t command_buffer_index = command_buffer_indices[thread_index]++;

        KW_ASSERT(thread_index < m_command_pool_data[semaphore_index].size());
        CommandPoolData& command_pool_data = m_command_pool_data[semaphore_index][thread_index];

        // When one thread is performing too long, other threads may need to process more render passes than they were
        // expecting. Extra command buffers may be required to do that.
        if (command_buffer_index >= command_pool_data.command_buffers.size()) {
            KW_ASSERT(command_buffer_index == command_pool_data.command_buffers.size());

            VkCommandBufferAllocateInfo command_buffer_allocate_info{};
            command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            command_buffer_allocate_info.commandPool = command_pool_data.command_pool;
            command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            command_buffer_allocate_info.commandBufferCount = 1;

            VkCommandBuffer command_buffer;
            VK_ERROR(
                vkAllocateCommandBuffers(m_render->device, &command_buffer_allocate_info, &command_buffer),
                "Failed to allocate frame command buffer %zu-%zu-%zu.", semaphore_index, thread_index, command_buffer_index
            );
            VK_NAME(m_render, command_buffer, "Frame command buffer %zu-%zu-%zu", semaphore_index, thread_index, command_buffer_index);

            command_pool_data.command_buffers.push_back(command_buffer);
        }

        VkCommandBuffer command_buffer = command_pool_data.command_buffers[command_buffer_index];
        KW_ASSERT(command_buffer != VK_NULL_HANDLE);

        KW_ASSERT(render_pass_command_buffers[render_pass_index] == VK_NULL_HANDLE);
        render_pass_command_buffers[render_pass_index] = command_buffer;

        VkCommandBufferBeginInfo command_buffer_begin_info{};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_ERROR(
            vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info),
            "Failed to begin frame command buffer %zu-%zu-%zu.", semaphore_index, thread_index, command_buffer_index
        );

        //
        // Perform synchronization between render passes.
        //

        if (render_pass_index == 0) {
            // For the very first `render` call attachment layouts must be set.
            if (m_frame_index == 0) {
                Vector<VkImageMemoryBarrier> image_memory_barriers(m_attachment_data.size(), m_render->transient_memory_resource);
                for (size_t attachment_index = 0; attachment_index < m_attachment_data.size(); attachment_index++) {
                    const AttachmentDescriptor& attachment_descriptor = m_attachment_descriptors[attachment_index];
                    AttachmentData& attachment_data = m_attachment_data[attachment_index];

                    VkAccessFlags initial_access_mask = attachment_data.initial_access_mask;
                    VkImageLayout initial_layout = attachment_data.initial_layout;
                    if (initial_layout == VK_IMAGE_LAYOUT_UNDEFINED) {
                        if (attachment_index == 0) {
                            // Swapchain attachment is never written, just present garbage.
                            initial_access_mask = 0;
                            initial_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                        } else {
                            // This happens only to attachments that are never read and written.
                            initial_access_mask = VK_ACCESS_SHADER_READ_BIT;
                            initial_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        }
                    }

                    VkImage attachment_image = attachment_data.image;
                    if (attachment_index == 0) {
                        attachment_image = m_swapchain_images[swapchain_image_index];
                    }
                    KW_ASSERT(attachment_image != VK_NULL_HANDLE);

                    VkImageAspectFlags aspect_mask;
                    if (attachment_descriptor.format == TextureFormat::D24_UNORM_S8_UINT || attachment_descriptor.format == TextureFormat::D32_FLOAT_S8X24_UINT) {
                        aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
                    } else if (attachment_descriptor.format == TextureFormat::D16_UNORM || attachment_descriptor.format == TextureFormat::D32_FLOAT) {
                        aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
                    } else {
                        aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
                    }

                    VkImageSubresourceRange image_subresource_range{};
                    image_subresource_range.aspectMask = aspect_mask;
                    image_subresource_range.baseMipLevel = 0;
                    image_subresource_range.levelCount = VK_REMAINING_MIP_LEVELS;
                    image_subresource_range.baseArrayLayer = 0;
                    image_subresource_range.layerCount = VK_REMAINING_ARRAY_LAYERS;

                    VkImageMemoryBarrier& image_memory_barrier = image_memory_barriers[attachment_index];
                    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    image_memory_barrier.srcAccessMask = 0;
                    image_memory_barrier.dstAccessMask = initial_access_mask;
                    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    image_memory_barrier.newLayout = initial_layout;
                    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    image_memory_barrier.image = attachment_image;
                    image_memory_barrier.subresourceRange = image_subresource_range;
                }

                vkCmdPipelineBarrier(
                    command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0,
                    0, nullptr, 0, nullptr, static_cast<uint32_t>(image_memory_barriers.size()), image_memory_barriers.data()
                );
            }
        } else if (m_render_pass_data[render_pass_index - 1].parallel_block_index != render_pass_data.parallel_block_index) {
            ParallelBlockData& parallel_block_data = m_parallel_block_data[render_pass_data.parallel_block_index];

            VkMemoryBarrier memory_barrier{};
            memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            memory_barrier.srcAccessMask = parallel_block_data.source_access_mask;
            memory_barrier.dstAccessMask = parallel_block_data.destination_access_mask;

            // First render passes in their parallel blocks require pipeline barriers.
            vkCmdPipelineBarrier(
                command_buffer, parallel_block_data.source_stage_mask, parallel_block_data.destination_stage_mask, 0,
                1, &memory_barrier, 0, nullptr, 0, nullptr
            );
        }

        //
        // Begin render pass.
        //

        VkFramebuffer framebuffer;
        if (render_pass_data.framebuffers[1] == VK_NULL_HANDLE) {
            framebuffer = render_pass_data.framebuffers[0];
        } else {
            framebuffer = render_pass_data.framebuffers[swapchain_image_index];
        }
        KW_ASSERT(framebuffer != VK_NULL_HANDLE);

        VkRect2D render_area{};
        render_area.offset.x = 0;
        render_area.offset.y = 0;
        render_area.extent.width = render_pass_data.framebuffer_width;
        render_area.extent.height = render_pass_data.framebuffer_height;

        Vector<VkClearValue> clear_values(render_pass_data.attachment_indices.size(), m_render->transient_memory_resource);
        for (size_t i = 0; i < render_pass_data.attachment_indices.size(); i++) {
            size_t attachment_index = render_pass_data.attachment_indices[i];
            KW_ASSERT(attachment_index < m_attachment_descriptors.size());

            AttachmentDescriptor& attachment_descriptor = m_attachment_descriptors[attachment_index];

            clear_values[i].color.float32[0] = attachment_descriptor.clear_color[0];
            clear_values[i].color.float32[1] = attachment_descriptor.clear_color[1];
            clear_values[i].color.float32[2] = attachment_descriptor.clear_color[2];
            clear_values[i].color.float32[3] = attachment_descriptor.clear_color[3];
            clear_values[i].depthStencil.depth = attachment_descriptor.clear_depth;
            clear_values[i].depthStencil.stencil = attachment_descriptor.clear_stencil;
        }

        VkRenderPassBeginInfo render_pass_begin_info{};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.renderPass = render_pass_data.render_pass;
        render_pass_begin_info.framebuffer = framebuffer;
        render_pass_begin_info.renderArea = render_area;
        render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
        render_pass_begin_info.pClearValues = clear_values.data();

        vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        // TODO: Draw calls.

        vkCmdEndRenderPass(command_buffer);

        vkEndCommandBuffer(command_buffer);
    }, m_render_pass_data.size());

    //
    // Before submitting render passes, submit all copy commands (new could be added in render passes),
    // which may create an execution dependency between transfer and graphics queues.
    //

    m_render->flush();

    //
    // Submit.
    //

    const uint64_t wait_semaphore_values[] = {
        0,
        m_render->semaphore->value,
    };

    const uint64_t signal_semaphore_values[] = {
        0,
        m_render_finished_timeline_semaphores[semaphore_index]->value,
    };

    VkTimelineSemaphoreSubmitInfo timeline_semaphore_submit_info{};
    timeline_semaphore_submit_info.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    timeline_semaphore_submit_info.waitSemaphoreValueCount = static_cast<uint32_t>(std::size(wait_semaphore_values));
    timeline_semaphore_submit_info.pWaitSemaphoreValues = wait_semaphore_values;
    timeline_semaphore_submit_info.signalSemaphoreValueCount = static_cast<uint32_t>(std::size(signal_semaphore_values));
    timeline_semaphore_submit_info.pSignalSemaphoreValues = signal_semaphore_values;

    VkPipelineStageFlags wait_stage_masks[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
    };

    const VkSemaphore wait_semaphores[] = {
        m_image_acquired_binary_semaphores[semaphore_index],
        m_render->semaphore->semaphore,
    };

    const VkSemaphore signal_semaphores[] = {
        m_render_finished_binary_semaphores[semaphore_index],
        m_render_finished_timeline_semaphores[semaphore_index]->semaphore,
    };

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = &timeline_semaphore_submit_info;
    submit_info.waitSemaphoreCount = static_cast<uint32_t>(std::size(wait_semaphores));
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stage_masks;
    submit_info.commandBufferCount = static_cast<uint32_t>(render_pass_command_buffers.size());
    submit_info.pCommandBuffers = render_pass_command_buffers.data();
    submit_info.signalSemaphoreCount = static_cast<uint32_t>(std::size(signal_semaphores));
    submit_info.pSignalSemaphores = signal_semaphores;

    {
        std::lock_guard<Spinlock> lock(*m_render->graphics_queue_spinlock);

        VK_ERROR(vkQueueSubmit(m_render->graphics_queue, 1, &submit_info, VK_NULL_HANDLE), "Failed to submit.");
    }

    //
    // Present.
    //

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &m_render_finished_binary_semaphores[semaphore_index];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &m_swapchain;
    present_info.pImageIndices = &swapchain_image_index;
    present_info.pResults = nullptr;

    VkResult present_result;
    {
        std::lock_guard<Spinlock> lock(*m_render->graphics_queue_spinlock);

        present_result = vkQueuePresentKHR(m_render->graphics_queue, &present_info);
    }

    if (present_result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain();

        // Avoid `m_frame_index` increment, which will cause attachment images hanging with undefined layout.
        return;
    } else if (present_result != VK_SUBOPTIMAL_KHR) {
        VK_ERROR(present_result, "Failed to present.");
    }

    m_frame_index++;
}

void FrameGraphVulkan::recreate_swapchain() {
    VK_ERROR(vkDeviceWaitIdle(m_render->device), "Failed to wait idle.");

    destroy_temporary_resources();
    create_temporary_resources();
}

void FrameGraphVulkan::create_lifetime_resources(const FrameGraphDescriptor& descriptor) {
    CreateContext create_context{
        descriptor,
        UnorderedMap<StringView, size_t>(m_render->transient_memory_resource),
        Vector<AttachmentAccess>(m_render->transient_memory_resource),
        LinearMemoryResource(m_render->transient_memory_resource, 4 * 1024 * 1024)
    };

    create_surface(create_context);
    compute_present_mode(create_context);

    compute_attachment_descriptors(create_context);
    compute_attachment_mapping(create_context);
    compute_attachment_access(create_context);
    compute_parallel_block_indices(create_context);
    compute_parallel_blocks(create_context);
    compute_attachment_ranges(create_context);
    compute_attachment_usage_mask(create_context);
    compute_attachment_layouts(create_context);

    create_render_passes(create_context);

    create_command_pools(create_context);
    create_synchronization(create_context);
}

void FrameGraphVulkan::destroy_lifetime_resources() {
    for (size_t swapchain_image_index = 0; swapchain_image_index < SWAPCHAIN_IMAGE_COUNT; swapchain_image_index++) {
        m_render_finished_timeline_semaphores[swapchain_image_index].reset();
    }

    for (size_t swapchain_image_index = 0; swapchain_image_index < SWAPCHAIN_IMAGE_COUNT; swapchain_image_index++) {
        vkDestroySemaphore(m_render->device, m_render_finished_binary_semaphores[swapchain_image_index], &m_render->allocation_callbacks);
        m_render_finished_binary_semaphores[swapchain_image_index] = VK_NULL_HANDLE;
    }

    for (size_t swapchain_image_index = 0; swapchain_image_index < SWAPCHAIN_IMAGE_COUNT; swapchain_image_index++) {
        vkDestroySemaphore(m_render->device, m_image_acquired_binary_semaphores[swapchain_image_index], &m_render->allocation_callbacks);
        m_image_acquired_binary_semaphores[swapchain_image_index] = VK_NULL_HANDLE;
    }

    for (size_t swapchain_image_index = 0; swapchain_image_index < SWAPCHAIN_IMAGE_COUNT; swapchain_image_index++) {
        for (CommandPoolData& command_pool_data : m_command_pool_data[swapchain_image_index]) {
            vkFreeCommandBuffers(m_render->device, command_pool_data.command_pool, static_cast<uint32_t>(command_pool_data.command_buffers.size()), command_pool_data.command_buffers.data());
            vkDestroyCommandPool(m_render->device, command_pool_data.command_pool, &m_render->allocation_callbacks);
        }
        m_command_pool_data[swapchain_image_index].clear();
    }

    m_parallel_block_data.clear();
    m_attachment_data.clear();

    for (RenderPassData& render_pass_data : m_render_pass_data) {
        for (GraphicsPipelineData& graphics_pipeline_data : render_pass_data.graphics_pipeline_data) {
            for (VkSampler& sampler : graphics_pipeline_data.samplers) {
                vkDestroySampler(m_render->device, sampler, &m_render->allocation_callbacks);
            }
            vkDestroyPipeline(m_render->device, graphics_pipeline_data.pipeline, &m_render->allocation_callbacks);
            vkDestroyPipelineLayout(m_render->device, graphics_pipeline_data.pipeline_layout, &m_render->allocation_callbacks);
            vkDestroyDescriptorSetLayout(m_render->device, graphics_pipeline_data.descriptor_set_layout, &m_render->allocation_callbacks);
            vkDestroyShaderModule(m_render->device, graphics_pipeline_data.fragment_shader_module, &m_render->allocation_callbacks);
            vkDestroyShaderModule(m_render->device, graphics_pipeline_data.vertex_shader_module, &m_render->allocation_callbacks);
        }
        vkDestroyRenderPass(m_render->device, render_pass_data.render_pass, &m_render->allocation_callbacks);
    }
    m_render_pass_data.clear();

    for (AttachmentDescriptor& attachment_descriptor : m_attachment_descriptors) {
        m_render->persistent_memory_resource.deallocate(const_cast<char*>(attachment_descriptor.name));
    }

    vkDestroySurfaceKHR(m_render->instance, m_surface, nullptr);
    m_surface = VK_NULL_HANDLE;
}

void FrameGraphVulkan::create_surface(CreateContext& create_context) {
    KW_ASSERT(m_surface == VK_NULL_HANDLE);
    SDL_ERROR(
        SDL_Vulkan_CreateSurface(m_window->get_sdl_window(), m_render->instance, &m_surface),
        "Failed to create Vulkan surface."
    );

    VkBool32 supported;
    vkGetPhysicalDeviceSurfaceSupportKHR(m_render->physical_device, m_render->graphics_queue_family_index, m_surface, &supported);
    KW_ERROR(
        supported == VK_TRUE,
        "Graphics queue doesn't support presentation."
    );
}

void FrameGraphVulkan::compute_present_mode(CreateContext& create_context) {
    if (!create_context.frame_graph_descriptor.is_vsync_enabled) {
        uint32_t present_mode_count;
        VK_ERROR(
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_render->physical_device, m_surface, &present_mode_count, nullptr),
            "Failed to query surface present mode count."
        );

        Vector<VkPresentModeKHR> present_modes(present_mode_count, m_render->transient_memory_resource);
        VK_ERROR(
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_render->physical_device, m_surface, &present_mode_count, present_modes.data()),
            "Failed to query surface present modes."
        );

        for (VkPresentModeKHR present_mode : present_modes) {
            if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR || (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR && m_present_mode != VK_PRESENT_MODE_MAILBOX_KHR)) {
                m_present_mode = present_mode;
            }
        }

        if (m_present_mode == VK_PRESENT_MODE_FIFO_KHR) {
            Log::print("[WARNING] Failed to turn VSync on.");
        }
    }
}

void FrameGraphVulkan::compute_attachment_descriptors(CreateContext& create_context) {
    const FrameGraphDescriptor& frame_graph_descriptor = create_context.frame_graph_descriptor;

    //
    // Calculate attachment count to avoid extra allocations.
    //

    size_t attachment_count = 1; // One for swapchain attachment.

    for (size_t i = 0; i < frame_graph_descriptor.color_attachment_descriptor_count; i++) {
        size_t count = std::max(frame_graph_descriptor.color_attachment_descriptors[i].count, static_cast<size_t>(1));
        attachment_count += count;
    }

    for (size_t i = 0; i < frame_graph_descriptor.depth_stencil_attachment_descriptor_count; i++) {
        size_t count = std::max(frame_graph_descriptor.depth_stencil_attachment_descriptors[i].count, static_cast<size_t>(1));
        attachment_count += count;
    }

    m_attachment_descriptors.reserve(attachment_count);

    //
    // Store all the attachments.
    //

    AttachmentDescriptor swapchain_attachment_descriptor{};
    swapchain_attachment_descriptor.name = frame_graph_descriptor.swapchain_attachment_name;
    swapchain_attachment_descriptor.load_op = LoadOp::DONT_CARE;
    swapchain_attachment_descriptor.format = TextureFormat::BGRA8_UNORM;
    m_attachment_descriptors.push_back(swapchain_attachment_descriptor);

    for (size_t i = 0; i < frame_graph_descriptor.color_attachment_descriptor_count; i++) {
        size_t count = std::max(frame_graph_descriptor.color_attachment_descriptors[i].count, static_cast<size_t>(1));
        for (size_t j = 0; j < count; j++) {
            m_attachment_descriptors.push_back(frame_graph_descriptor.color_attachment_descriptors[i]);
        }
    }

    for (size_t i = 0; i < frame_graph_descriptor.depth_stencil_attachment_descriptor_count; i++) {
        size_t count = std::max(frame_graph_descriptor.depth_stencil_attachment_descriptors[i].count, static_cast<size_t>(1));
        for (size_t j = 0; j < count; j++) {
            m_attachment_descriptors.push_back(frame_graph_descriptor.depth_stencil_attachment_descriptors[i]);
        }
    }

    //
    // Names specified in descriptors can become corrupted after constructor execution. Normalize width, height and count.
    //

    for (AttachmentDescriptor& attachment_descriptor : m_attachment_descriptors) {
        size_t length = std::strlen(attachment_descriptor.name);

        char* copy = static_cast<char*>(m_render->persistent_memory_resource.allocate(length + 1, 1));
        std::memcpy(copy, attachment_descriptor.name, length + 1);

        attachment_descriptor.name = copy;

        if (attachment_descriptor.width == 0.f) {
            attachment_descriptor.width = 1.f;
        }

        if (attachment_descriptor.height == 0.f) {
            attachment_descriptor.height = 1.f;
        }

        attachment_descriptor.count = std::max(attachment_descriptor.count, static_cast<size_t>(1));
    }

    //
    // Print all attachment descriptors to log.
    //

#ifdef KW_FRAME_GRAPH_LOG
    Log::print("[Frame Graph] Attachment descriptors:");

    for (size_t attachment_index = 0; attachment_index < m_attachment_descriptors.size(); attachment_index++) {
        const AttachmentDescriptor& attachment_descriptor = m_attachment_descriptors[attachment_index];

        Log::print("[Frame Graph] * Index: %zu", attachment_index);
        Log::print("[Frame Graph]   Name: \"%s\"", attachment_descriptor.name);
        Log::print("[Frame Graph]   Format: %s", enum_to_string(attachment_descriptor.format));
        Log::print("[Frame Graph]   Size class: %s", enum_to_string(attachment_descriptor.size_class));
        Log::print("[Frame Graph]   Width: %.1f", attachment_descriptor.width);
        Log::print("[Frame Graph]   Height: %.1f", attachment_descriptor.height);
        Log::print("[Frame Graph]   Count: %zu", attachment_descriptor.count);
        Log::print("[Frame Graph]   Load op: %s", enum_to_string(attachment_descriptor.load_op));

        if (attachment_descriptor.load_op == LoadOp::CLEAR) {
            if (TextureFormatUtils::is_depth_stencil(attachment_descriptor.format)) {
                Log::print("[Frame Graph]   Clear depth: %.1f", attachment_descriptor.clear_depth);
                Log::print("[Frame Graph]   Clear stencil: %u", attachment_descriptor.clear_stencil);
            } else {
                Log::print(
                    "[Frame Graph]   Clear color: %.1f %.1f %.1f %.1f",
                    attachment_descriptor.clear_color[0], attachment_descriptor.clear_color[1],
                    attachment_descriptor.clear_color[2], attachment_descriptor.clear_color[3]
                );
            }
        }
    }
#endif
}

void FrameGraphVulkan::compute_attachment_mapping(CreateContext& create_context) {
    KW_ASSERT(create_context.attachment_mapping.empty());
    create_context.attachment_mapping.reserve(m_attachment_descriptors.size());

    for (size_t attachment_index = 0; attachment_index < m_attachment_descriptors.size(); attachment_index++) {
        // Attachment with `count` > 1 may fail on this one.
        create_context.attachment_mapping.emplace(StringView(m_attachment_descriptors[attachment_index].name), attachment_index);
    }
}

void FrameGraphVulkan::compute_attachment_access(CreateContext& create_context) {
    const FrameGraphDescriptor& frame_graph_descriptor = create_context.frame_graph_descriptor;

    KW_ASSERT(create_context.attachment_access_matrix.empty());
    create_context.attachment_access_matrix.resize(frame_graph_descriptor.render_pass_descriptor_count * m_attachment_descriptors.size());

    for (size_t render_pass_index = 0; render_pass_index < frame_graph_descriptor.render_pass_descriptor_count; render_pass_index++) {
        const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];

        for (size_t color_attachment_index = 0; color_attachment_index < render_pass_descriptor.color_attachment_name_count; color_attachment_index++) {
            const char* color_attachment_name = render_pass_descriptor.color_attachment_names[color_attachment_index];
            KW_ASSERT(create_context.attachment_mapping.count(color_attachment_name) > 0);

            size_t attachment_index = create_context.attachment_mapping[color_attachment_name];
            KW_ASSERT(attachment_index < m_attachment_descriptors.size());

            size_t attachment_count = m_attachment_descriptors[attachment_index].count;
            KW_ASSERT(attachment_index + attachment_count <= m_attachment_descriptors.size());

            for (size_t offset = 0; offset < attachment_count; offset++) {
                size_t access_index = render_pass_index * m_attachment_descriptors.size() + attachment_index + offset;
                KW_ASSERT(access_index < create_context.attachment_access_matrix.size());

                create_context.attachment_access_matrix[access_index] |= AttachmentAccess::WRITE | AttachmentAccess::ATTACHMENT | AttachmentAccess::LOAD | AttachmentAccess::STORE;
            }
        }

        for (size_t graphics_pipeline_index = 0; graphics_pipeline_index < render_pass_descriptor.graphics_pipeline_descriptor_count; graphics_pipeline_index++) {
            const GraphicsPipelineDescriptor& graphics_pipeline_descriptor = render_pass_descriptor.graphics_pipeline_descriptors[graphics_pipeline_index];

            for (size_t attachment_blend_index = 0; attachment_blend_index < graphics_pipeline_descriptor.attachment_blend_descriptor_count; attachment_blend_index++) {
                const AttachmentBlendDescriptor& attachment_blend_descriptor = graphics_pipeline_descriptor.attachment_blend_descriptors[attachment_blend_index];

                const char* attachment_name = attachment_blend_descriptor.attachment_name;
                KW_ASSERT(create_context.attachment_mapping.count(attachment_name) > 0);

                size_t attachment_index = create_context.attachment_mapping[attachment_name];
                KW_ASSERT(attachment_index < m_attachment_descriptors.size());

                size_t attachment_count = m_attachment_descriptors[attachment_index].count;
                KW_ASSERT(attachment_index + attachment_count <= m_attachment_descriptors.size());

                for (size_t offset = 0; offset < attachment_count; offset++) {
                    size_t access_index = render_pass_index * m_attachment_descriptors.size() + attachment_index + offset;
                    KW_ASSERT(access_index < create_context.attachment_access_matrix.size());

                    create_context.attachment_access_matrix[access_index] |= AttachmentAccess::BLEND;
                }
            }
        }

        if (render_pass_descriptor.depth_stencil_attachment_name != nullptr) {
            const char* depth_stencil_attachment_name = render_pass_descriptor.depth_stencil_attachment_name;
            KW_ASSERT(create_context.attachment_mapping.count(depth_stencil_attachment_name) > 0);

            size_t attachment_index = create_context.attachment_mapping[depth_stencil_attachment_name];
            KW_ASSERT(attachment_index < m_attachment_descriptors.size());

            size_t attachment_count = m_attachment_descriptors[attachment_index].count;
            KW_ASSERT(attachment_index + attachment_count <= m_attachment_descriptors.size());

            AttachmentAccess attachment_access = AttachmentAccess::READ;

            for (size_t graphics_pipeline_index = 0; graphics_pipeline_index < render_pass_descriptor.graphics_pipeline_descriptor_count; graphics_pipeline_index++) {
                const GraphicsPipelineDescriptor& graphics_pipeline_descriptor = render_pass_descriptor.graphics_pipeline_descriptors[graphics_pipeline_index];

                if ((graphics_pipeline_descriptor.is_depth_test_enabled && graphics_pipeline_descriptor.is_depth_write_enabled) ||
                    (graphics_pipeline_descriptor.is_stencil_test_enabled && graphics_pipeline_descriptor.stencil_write_mask != 0)) {
                    attachment_access = AttachmentAccess::WRITE;
                    break;
                }
            }

            for (size_t offset = 0; offset < attachment_count; offset++) {
                size_t access_index = render_pass_index * m_attachment_descriptors.size() + attachment_index + offset;
                KW_ASSERT(access_index < create_context.attachment_access_matrix.size());

                create_context.attachment_access_matrix[access_index] |= attachment_access | AttachmentAccess::ATTACHMENT | AttachmentAccess::LOAD | AttachmentAccess::STORE;
            }
        }

        for (size_t graphics_pipeline_index = 0; graphics_pipeline_index < render_pass_descriptor.graphics_pipeline_descriptor_count; graphics_pipeline_index++) {
            const GraphicsPipelineDescriptor& graphics_pipeline_descriptor = render_pass_descriptor.graphics_pipeline_descriptors[graphics_pipeline_index];

            for (size_t uniform_attachment_index = 0; uniform_attachment_index < graphics_pipeline_descriptor.uniform_attachment_descriptor_count; uniform_attachment_index++) {
                const UniformAttachmentDescriptor& uniform_attachment_descriptor = graphics_pipeline_descriptor.uniform_attachment_descriptors[uniform_attachment_index];
                KW_ASSERT(create_context.attachment_mapping.count(uniform_attachment_descriptor.attachment_name) > 0);

                size_t attachment_index = create_context.attachment_mapping[uniform_attachment_descriptor.attachment_name];
                KW_ASSERT(attachment_index < m_attachment_descriptors.size());

                size_t attachment_count = m_attachment_descriptors[attachment_index].count;
                KW_ASSERT(attachment_index + attachment_count <= m_attachment_descriptors.size());

                AttachmentAccess shader_access = AttachmentAccess::NONE;

                switch (uniform_attachment_descriptor.visibility) {
                case ShaderVisibility::ALL:
                    shader_access = AttachmentAccess::VERTEX_SHADER | AttachmentAccess::FRAGMENT_SHADER;
                    break;
                case ShaderVisibility::VERTEX:
                    shader_access = AttachmentAccess::VERTEX_SHADER;
                    break;
                case ShaderVisibility::FRAGMENT:
                    shader_access = AttachmentAccess::FRAGMENT_SHADER;
                    break;
                }

                for (size_t offset = 0; offset < attachment_count; offset++) {
                    size_t access_index = render_pass_index * m_attachment_descriptors.size() + attachment_index + offset;
                    KW_ASSERT(access_index < create_context.attachment_access_matrix.size());

                    create_context.attachment_access_matrix[access_index] |= AttachmentAccess::READ | shader_access;
                }
            }
        }
    }

    for (size_t attachment_index = 0; attachment_index < m_attachment_descriptors.size(); attachment_index++) {
        const AttachmentDescriptor& attachment_descriptor = m_attachment_descriptors[attachment_index];
        if (attachment_descriptor.load_op != LoadOp::LOAD) {
            size_t min_read_render_pass_index = SIZE_MAX;
            size_t max_read_render_pass_index = SIZE_MAX;
            size_t min_write_render_pass_index = SIZE_MAX;
            size_t max_write_render_pass_index = SIZE_MAX;

            for (size_t render_pass_index = 0; render_pass_index < frame_graph_descriptor.render_pass_descriptor_count; render_pass_index++) {
                size_t access_index = render_pass_index * m_attachment_descriptors.size() + attachment_index;
                KW_ASSERT(access_index < create_context.attachment_access_matrix.size());

                AttachmentAccess& attachment_access = create_context.attachment_access_matrix[access_index];

                if ((attachment_access & AttachmentAccess::READ) == AttachmentAccess::READ) {
                    if (min_read_render_pass_index == SIZE_MAX) {
                        min_read_render_pass_index = render_pass_index;
                    }

                    max_read_render_pass_index = render_pass_index;
                }

                if ((attachment_access & AttachmentAccess::WRITE) == AttachmentAccess::WRITE) {
                    if (min_write_render_pass_index == SIZE_MAX) {
                        min_write_render_pass_index = render_pass_index;

                        attachment_access &= ~AttachmentAccess::LOAD;
                    }

                    max_write_render_pass_index = render_pass_index;
                }
            }

            if (attachment_index == 0) {
                // This restriction allows the last write render pass to transition the attachment image to present layout.
                KW_ERROR(
                    max_read_render_pass_index == SIZE_MAX || (max_write_render_pass_index != SIZE_MAX && min_read_render_pass_index > min_write_render_pass_index && max_read_render_pass_index < max_write_render_pass_index),
                    "Swapchain image must not be read before the first write nor after the last write."
                );
            }

            if (max_write_render_pass_index != SIZE_MAX) {
                size_t access_index = max_write_render_pass_index * m_attachment_descriptors.size() + attachment_index;
                KW_ASSERT(access_index < create_context.attachment_access_matrix.size());

                AttachmentAccess& attachment_access = create_context.attachment_access_matrix[access_index];

                if (max_read_render_pass_index != SIZE_MAX) {
                    if (min_read_render_pass_index > min_write_render_pass_index && max_read_render_pass_index < max_write_render_pass_index) {
                        // All read accesses are between write accesses, so the last write access is followed by a write access that doesn't load.
                        attachment_access &= ~AttachmentAccess::STORE;
                    }
                } else {
                    // Only write accesses, the last write access shouldn't store because it is followed by a write access that doesn't load.
                    attachment_access &= ~AttachmentAccess::STORE;
                }
            }
        }
    }

#ifdef KW_FRAME_GRAPH_LOG
    int attachment_name_length = 0;

    for (size_t attachment_index = 0; attachment_index < m_attachment_descriptors.size(); attachment_index++) {
        attachment_name_length = std::max(static_cast<int>(strlen(m_attachment_descriptors[attachment_index].name)), attachment_name_length);
    }

    constexpr size_t ACCESS_BUFFER_LENGTH = 5;
    String line_buffer(frame_graph_descriptor.render_pass_descriptor_count * ACCESS_BUFFER_LENGTH, ' ', m_render->transient_memory_resource);

    Log::print("[Frame Graph] Attachment access matrix:");

    for (size_t attachment_index = 0; attachment_index < m_attachment_descriptors.size(); attachment_index++) {
        const AttachmentDescriptor& attachment_descriptor = m_attachment_descriptors[attachment_index];

        for (size_t render_pass_index = 0; render_pass_index < frame_graph_descriptor.render_pass_descriptor_count; render_pass_index++) {
            size_t access_index = render_pass_index * m_attachment_descriptors.size() + attachment_index;
            AttachmentAccess attachment_access = create_context.attachment_access_matrix[access_index];
            
            char* access_buffer = line_buffer.data() + render_pass_index * ACCESS_BUFFER_LENGTH;
            std::fill(access_buffer, access_buffer + ACCESS_BUFFER_LENGTH, ' ');

            if ((attachment_access & AttachmentAccess::WRITE) == AttachmentAccess::WRITE) {
                *(access_buffer++) = 'W';
                
                if ((attachment_access & AttachmentAccess::BLEND) == AttachmentAccess::BLEND) {
                    *(access_buffer++) = 'B';
                }

                if ((attachment_access & AttachmentAccess::LOAD) == AttachmentAccess::LOAD) {
                    *(access_buffer++) = 'L';
                }
                
                if ((attachment_access & AttachmentAccess::STORE) == AttachmentAccess::STORE) {
                    *(access_buffer++) = 'S';
                }
            } else if ((attachment_access & AttachmentAccess::READ) == AttachmentAccess::READ) {
                *(access_buffer++) = 'R';

                if ((attachment_access & AttachmentAccess::ATTACHMENT) == AttachmentAccess::ATTACHMENT) {
                    *(access_buffer++) = 'A';
                }

                if ((attachment_access & AttachmentAccess::VERTEX_SHADER) == AttachmentAccess::VERTEX_SHADER) {
                    *(access_buffer++) = 'V';
                }

                if ((attachment_access & AttachmentAccess::FRAGMENT_SHADER) == AttachmentAccess::FRAGMENT_SHADER) {
                    *(access_buffer++) = 'F';
                }
            }
        }

        Log::print("[Frame Graph] %*s %s", attachment_name_length, attachment_descriptor.name, line_buffer.c_str());
    }
#endif
}

void FrameGraphVulkan::compute_parallel_block_indices(CreateContext& create_context) {
    const FrameGraphDescriptor& frame_graph_descriptor = create_context.frame_graph_descriptor;

    KW_ASSERT(m_render_pass_data.empty());
    m_render_pass_data.resize(frame_graph_descriptor.render_pass_descriptor_count, RenderPassData(m_render->persistent_memory_resource));

    // Keep accesses to each attachment in current parallel block. Once they conflict, move attachment to a new parallel block.
    Vector<AttachmentAccess> previous_accesses(m_attachment_descriptors.size(), m_render->transient_memory_resource);
    size_t parallel_block_index = 0;

    for (size_t render_pass_index = 0; render_pass_index < frame_graph_descriptor.render_pass_descriptor_count; render_pass_index++) {
        for (size_t attachment_index = 0; attachment_index < m_attachment_descriptors.size(); attachment_index++) {
            size_t access_index = render_pass_index * m_attachment_descriptors.size() + attachment_index;
            KW_ASSERT(access_index < create_context.attachment_access_matrix.size());

            AttachmentAccess previous_access = previous_accesses[attachment_index];
            AttachmentAccess current_access = create_context.attachment_access_matrix[access_index];

            if (((current_access & AttachmentAccess::WRITE) == AttachmentAccess::WRITE && previous_access != AttachmentAccess::NONE) ||
                (current_access != AttachmentAccess::NONE && (previous_access & AttachmentAccess::WRITE) == AttachmentAccess::WRITE)) {
                std::fill(previous_accesses.begin(), previous_accesses.end(), AttachmentAccess::NONE);
                parallel_block_index++;
                break;
            }
        }

        m_render_pass_data[render_pass_index].parallel_block_index = parallel_block_index;

        for (size_t attachment_index = 0; attachment_index < m_attachment_descriptors.size(); attachment_index++) {
            size_t access_index = render_pass_index * m_attachment_descriptors.size() + attachment_index;
            KW_ASSERT(access_index < create_context.attachment_access_matrix.size());

            AttachmentAccess& previous_access = previous_accesses[attachment_index];
            AttachmentAccess current_access = create_context.attachment_access_matrix[access_index];

            if (previous_access == AttachmentAccess::NONE) {
                previous_access = current_access;
            } else {
                // Not possible otherwise because for this kind of conflict previous loop clears the `previous_accesses` array.
                KW_ASSERT(current_access == AttachmentAccess::NONE || previous_access == current_access);
            }
        }
    }

#ifdef KW_FRAME_GRAPH_LOG
    Log::print("[Frame Graph] Render pass parallel indices:");

    for (size_t render_pass_index = 0; render_pass_index < frame_graph_descriptor.render_pass_descriptor_count; render_pass_index++) {
        const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];
        const RenderPassData& render_pass_data = m_render_pass_data[render_pass_index];

        Log::print("[Frame Graph] * Name: \"%s\"", render_pass_descriptor.name);
        Log::print("[Frame Graph]   Parallel index: %zu", render_pass_data.parallel_block_index);
    }
#endif
}

void FrameGraphVulkan::compute_parallel_blocks(CreateContext& create_context) {
    const FrameGraphDescriptor& frame_graph_descriptor = create_context.frame_graph_descriptor;

    KW_ASSERT(m_parallel_block_data.empty());
    m_parallel_block_data.resize(m_render_pass_data.empty() ? 0 : m_render_pass_data.back().parallel_block_index + 1);

    for (size_t render_pass_index = 0; render_pass_index < m_render_pass_data.size(); render_pass_index++) {
        RenderPassData& render_pass_data = m_render_pass_data[render_pass_index];
        KW_ASSERT(render_pass_data.parallel_block_index < m_parallel_block_data.size());

        ParallelBlockData& parallel_block_data = m_parallel_block_data[render_pass_data.parallel_block_index];

        for (size_t attachment_index = 0; attachment_index < m_attachment_descriptors.size(); attachment_index++) {
            const AttachmentDescriptor& attachment_descriptor = m_attachment_descriptors[attachment_index];

            size_t access_index = render_pass_index * m_attachment_descriptors.size() + attachment_index;
            KW_ASSERT(access_index < create_context.attachment_access_matrix.size());

            AttachmentAccess attachment_access = create_context.attachment_access_matrix[access_index];

            if (attachment_access != AttachmentAccess::NONE) {
                AttachmentAccess previous_attachment_access = AttachmentAccess::NONE;

                for (size_t offset = 1; offset <= render_pass_index; offset++) {
                    size_t another_attachment_index = render_pass_index - offset;

                    RenderPassData& another_render_pass_data = m_render_pass_data[another_attachment_index];
                    KW_ASSERT(another_render_pass_data.parallel_block_index < m_parallel_block_data.size());

                    if (another_render_pass_data.parallel_block_index < render_pass_data.parallel_block_index) {
                        size_t another_access_index = another_attachment_index * m_attachment_descriptors.size() + attachment_index;
                        KW_ASSERT(another_access_index < create_context.attachment_access_matrix.size());

                        AttachmentAccess another_attachment_access = create_context.attachment_access_matrix[another_access_index];

                        if (another_attachment_access != AttachmentAccess::NONE) {
                            previous_attachment_access = another_attachment_access;
                            break;
                        }
                    }
                }

                if ((previous_attachment_access & AttachmentAccess::READ) == AttachmentAccess::READ) {
                    if ((attachment_access & AttachmentAccess::WRITE) == AttachmentAccess::WRITE) {
                        if (TextureFormatUtils::is_depth_stencil(attachment_descriptor.format)) {
                            if ((previous_attachment_access & AttachmentAccess::FRAGMENT_SHADER) == AttachmentAccess::FRAGMENT_SHADER) {
                                parallel_block_data.source_stage_mask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                            } else if ((previous_attachment_access & AttachmentAccess::ATTACHMENT) == AttachmentAccess::ATTACHMENT) {
                                parallel_block_data.source_stage_mask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                            } else {
                                KW_ASSERT((previous_attachment_access & AttachmentAccess::VERTEX_SHADER) == AttachmentAccess::VERTEX_SHADER);
                                parallel_block_data.source_stage_mask |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
                            }
                        } else {
                            if ((previous_attachment_access & AttachmentAccess::FRAGMENT_SHADER) == AttachmentAccess::FRAGMENT_SHADER) {
                                parallel_block_data.source_stage_mask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                            } else {
                                KW_ASSERT((previous_attachment_access & AttachmentAccess::VERTEX_SHADER) == AttachmentAccess::VERTEX_SHADER);
                                parallel_block_data.source_stage_mask |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
                            }
                        }
                    } else {
                        parallel_block_data.source_stage_mask |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    }
                } else if ((previous_attachment_access & AttachmentAccess::WRITE) == AttachmentAccess::WRITE) {
                    if (TextureFormatUtils::is_depth_stencil(attachment_descriptor.format)) {
                        parallel_block_data.source_stage_mask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                    } else {
                        parallel_block_data.source_stage_mask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    }
                }
                
                if ((previous_attachment_access & AttachmentAccess::WRITE) == AttachmentAccess::WRITE) {
                    if (TextureFormatUtils::is_depth_stencil(attachment_descriptor.format)) {
                        parallel_block_data.source_access_mask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    } else {
                        parallel_block_data.source_access_mask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    }
                }

                if ((attachment_access & AttachmentAccess::READ) == AttachmentAccess::READ) {
                    if ((previous_attachment_access & AttachmentAccess::WRITE) == AttachmentAccess::WRITE) {
                        if (TextureFormatUtils::is_depth_stencil(attachment_descriptor.format)) {
                            if ((attachment_access & AttachmentAccess::VERTEX_SHADER) == AttachmentAccess::VERTEX_SHADER) {
                                parallel_block_data.destination_stage_mask |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
                            } else if ((attachment_access & AttachmentAccess::ATTACHMENT) == AttachmentAccess::ATTACHMENT) {
                                parallel_block_data.destination_stage_mask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                            } else {
                                KW_ASSERT((attachment_access & AttachmentAccess::FRAGMENT_SHADER) == AttachmentAccess::FRAGMENT_SHADER);
                                parallel_block_data.destination_stage_mask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                            }
                        } else {
                            if ((attachment_access & AttachmentAccess::VERTEX_SHADER) == AttachmentAccess::VERTEX_SHADER) {
                                parallel_block_data.destination_stage_mask |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
                            } else {
                                KW_ASSERT((attachment_access & AttachmentAccess::FRAGMENT_SHADER) == AttachmentAccess::FRAGMENT_SHADER);
                                parallel_block_data.destination_stage_mask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                            }
                        }
                    } else {
                        parallel_block_data.destination_stage_mask |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                    }
                } else if ((attachment_access & AttachmentAccess::WRITE) == AttachmentAccess::WRITE) {
                    if (TextureFormatUtils::is_depth_stencil(attachment_descriptor.format)) {
                        parallel_block_data.destination_stage_mask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                    } else {
                        parallel_block_data.destination_stage_mask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    }
                }

                if ((attachment_access & AttachmentAccess::READ) == AttachmentAccess::READ) {
                    if (TextureFormatUtils::is_depth_stencil(attachment_descriptor.format)) {
                        if ((attachment_access & AttachmentAccess::ATTACHMENT) == AttachmentAccess::ATTACHMENT) {
                            parallel_block_data.destination_access_mask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                        }

                        if ((attachment_access & (AttachmentAccess::VERTEX_SHADER | AttachmentAccess::FRAGMENT_SHADER)) != AttachmentAccess::NONE) {
                            parallel_block_data.destination_access_mask |= VK_ACCESS_SHADER_READ_BIT;
                        }
                    } else {
                        parallel_block_data.destination_access_mask |= VK_ACCESS_SHADER_READ_BIT;
                    }
                } else if ((attachment_access & AttachmentAccess::WRITE) == AttachmentAccess::WRITE) {
                    if (TextureFormatUtils::is_depth_stencil(attachment_descriptor.format)) {
                        parallel_block_data.destination_access_mask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                        if ((attachment_access & AttachmentAccess::LOAD) == AttachmentAccess::LOAD) {
                            parallel_block_data.destination_access_mask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                        }
                    } else {
                        parallel_block_data.destination_access_mask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                        if ((attachment_access & (AttachmentAccess::LOAD | AttachmentAccess::BLEND)) != AttachmentAccess::NONE) {
                            parallel_block_data.destination_access_mask |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                        }
                    }
                }
            }
        }
    }

#ifdef KW_FRAME_GRAPH_LOG
    Log::print("[Frame Graph] Parallel blocks:");

    for (const ParallelBlockData& parallel_block_data : m_parallel_block_data) {
        Log::print("[Frame Graph] * Source stage mask: 0x%x", parallel_block_data.source_stage_mask);
        Log::print("[Frame Graph]   Destination stage mask: 0x%x", parallel_block_data.destination_stage_mask);
        Log::print("[Frame Graph]   Source access mask: 0x%x", parallel_block_data.source_access_mask);
        Log::print("[Frame Graph]   Destination access mask: 0x%x", parallel_block_data.destination_access_mask);
    }
#endif
}

void FrameGraphVulkan::compute_attachment_ranges(CreateContext& create_context) {
    const FrameGraphDescriptor& frame_graph_descriptor = create_context.frame_graph_descriptor;

    KW_ASSERT(m_attachment_data.empty());
    m_attachment_data.resize(m_attachment_descriptors.size());

    for (size_t attachment_index = 0; attachment_index < m_attachment_data.size(); attachment_index++) {
        const AttachmentDescriptor& attachment_descriptor = m_attachment_descriptors[attachment_index];
        AttachmentData& attachment_data = m_attachment_data[attachment_index];

        // Load attachments must be never aliased.
        if (!frame_graph_descriptor.is_aliasing_enabled || attachment_descriptor.load_op == LoadOp::LOAD) {
            attachment_data.min_parallel_block_index = 0;
            attachment_data.max_parallel_block_index = m_render_pass_data.back().parallel_block_index;
        } else {
            size_t min_render_pass_index = SIZE_MAX;
            size_t max_render_pass_index = 0;

            for (size_t render_pass_index = 0; render_pass_index < frame_graph_descriptor.render_pass_descriptor_count; render_pass_index++) {
                size_t access_index = render_pass_index * m_attachment_descriptors.size() + attachment_index;
                KW_ASSERT(access_index < create_context.attachment_access_matrix.size());

                if ((create_context.attachment_access_matrix[access_index] & AttachmentAccess::WRITE) == AttachmentAccess::WRITE) {
                    min_render_pass_index = std::min(min_render_pass_index, render_pass_index);
                    max_render_pass_index = std::max(max_render_pass_index, render_pass_index);
                }
            }

            if (min_render_pass_index == SIZE_MAX) {
                // This is rather a weird scenario, this attachment is never written. Avoid aliasing such attachment
                // because there's no render pass that would convert its layout from `VK_IMAGE_LAYOUT_UNDEFINED` to
                // `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`.
                attachment_data.min_parallel_block_index = 0;
                attachment_data.max_parallel_block_index = m_render_pass_data.back().parallel_block_index;
            } else {
                size_t previous_read_render_pass_index = SIZE_MAX;

                for (size_t offset = frame_graph_descriptor.render_pass_descriptor_count; offset > 0; offset--) {
                    size_t render_pass_index = (min_render_pass_index + offset) % frame_graph_descriptor.render_pass_descriptor_count;

                    size_t access_index = render_pass_index * m_attachment_descriptors.size() + attachment_index;
                    KW_ASSERT(access_index < create_context.attachment_access_matrix.size());

                    if ((create_context.attachment_access_matrix[access_index] & AttachmentAccess::READ) == AttachmentAccess::READ) {
                        previous_read_render_pass_index = render_pass_index;
                        break;
                    }
                }

                if (previous_read_render_pass_index != SIZE_MAX) {
                    if (previous_read_render_pass_index > min_render_pass_index) {
                        // Previous read render pass was on previous frame.
                        // Compute non-looped range 000011110000 where min <= max.

                        max_render_pass_index = std::max(max_render_pass_index, previous_read_render_pass_index);
                        KW_ASSERT(m_render_pass_data[min_render_pass_index].parallel_block_index <= m_render_pass_data[max_render_pass_index].parallel_block_index);
                    } else {
                        // Previous read render pass was on the same frame before first write render pass.
                        // Compute looped range 111100001111 where min > max.
                        
                        // Previous read render pass parallel index is always less than first write render pass's
                        // parallel index (so we won't face min = max meaning all render pass range).

                        max_render_pass_index = previous_read_render_pass_index;
                        KW_ASSERT(m_render_pass_data[min_render_pass_index].parallel_block_index > m_render_pass_data[max_render_pass_index].parallel_block_index);
                    }
                }
            }

            if (min_render_pass_index != SIZE_MAX) {
                attachment_data.min_parallel_block_index = m_render_pass_data[min_render_pass_index].parallel_block_index;
                attachment_data.max_parallel_block_index = m_render_pass_data[max_render_pass_index].parallel_block_index;
            }
        }
    }

#ifdef KW_FRAME_GRAPH_LOG
    Log::print("[Frame Graph] Attachment parallel block indices:");

    for (size_t attachment_index = 0; attachment_index < m_attachment_descriptors.size(); attachment_index++) {
        const AttachmentDescriptor& attachment_descriptor = m_attachment_descriptors[attachment_index];
        const AttachmentData& attachment_data = m_attachment_data[attachment_index];

        Log::print("[Frame Graph] * Name: \"%s\"", attachment_descriptor.name);
        Log::print("[Frame Graph]   Min parallel block index: %zu", attachment_data.min_parallel_block_index);
        Log::print("[Frame Graph]   Max parallel block index: %zu", attachment_data.max_parallel_block_index);
    }
#endif
}

void FrameGraphVulkan::compute_attachment_usage_mask(CreateContext& create_context) {
    const FrameGraphDescriptor& frame_graph_descriptor = create_context.frame_graph_descriptor;

    for (size_t attachment_index = 0; attachment_index < m_attachment_descriptors.size(); attachment_index++) {
        AttachmentDescriptor& attachment_descriptor = m_attachment_descriptors[attachment_index];
        AttachmentData& attachment_data = m_attachment_data[attachment_index];

        for (size_t render_pass_index = 0; render_pass_index < frame_graph_descriptor.render_pass_descriptor_count; render_pass_index++) {
            size_t access_index = render_pass_index * m_attachment_descriptors.size() + attachment_index;
            KW_ASSERT(access_index < create_context.attachment_access_matrix.size());

            AttachmentAccess attachment_access = create_context.attachment_access_matrix[access_index];

            if ((attachment_access & AttachmentAccess::READ) == AttachmentAccess::READ) {
                if (TextureFormatUtils::is_depth_stencil(attachment_descriptor.format)) {
                    if ((attachment_access & AttachmentAccess::ATTACHMENT) == AttachmentAccess::ATTACHMENT) {
                        attachment_data.usage_mask |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                    }

                    if ((attachment_access & (AttachmentAccess::VERTEX_SHADER | AttachmentAccess::FRAGMENT_SHADER)) != AttachmentAccess::NONE) {
                        attachment_data.usage_mask |= VK_IMAGE_USAGE_SAMPLED_BIT;
                    }
                } else {
                    attachment_data.usage_mask |= VK_IMAGE_USAGE_SAMPLED_BIT;
                }
            } else if ((attachment_access & AttachmentAccess::WRITE) == AttachmentAccess::WRITE) {
                if (TextureFormatUtils::is_depth_stencil(attachment_descriptor.format)) {
                    attachment_data.usage_mask |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                } else {
                    attachment_data.usage_mask |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                }
            }
        }
    }

#ifdef KW_FRAME_GRAPH_LOG
    Log::print("[Frame Graph] Attachment usage mask:");

    for (size_t attachment_index = 0; attachment_index < m_attachment_descriptors.size(); attachment_index++) {
        const AttachmentDescriptor& attachment_descriptor = m_attachment_descriptors[attachment_index];
        const AttachmentData& attachment_data = m_attachment_data[attachment_index];

        Log::print("[Frame Graph] * Name: \"%s\"", attachment_descriptor.name);
        Log::print("[Frame Graph]   Usage mask: 0x%x", attachment_data.usage_mask);
    }
#endif
}

void FrameGraphVulkan::compute_attachment_layouts(CreateContext& create_context) {
    const FrameGraphDescriptor& frame_graph_descriptor = create_context.frame_graph_descriptor;

    for (size_t attachment_index = 0; attachment_index < m_attachment_descriptors.size(); attachment_index++) {
        AttachmentDescriptor& attachment_descriptor = m_attachment_descriptors[attachment_index];
        AttachmentData& attachment_data = m_attachment_data[attachment_index];

        for (size_t render_pass_index = 0; render_pass_index < frame_graph_descriptor.render_pass_descriptor_count; render_pass_index++) {
            size_t access_index = render_pass_index * m_attachment_descriptors.size() + attachment_index;
            KW_ASSERT(access_index < create_context.attachment_access_matrix.size());

            AttachmentAccess attachment_access = create_context.attachment_access_matrix[access_index];

            if ((attachment_access & AttachmentAccess::READ) == AttachmentAccess::READ) {
                if (TextureFormatUtils::is_depth_stencil(attachment_descriptor.format)) {
                    if ((attachment_access & AttachmentAccess::ATTACHMENT) == AttachmentAccess::ATTACHMENT) {
                        attachment_data.initial_access_mask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                    }

                    if ((attachment_access & (AttachmentAccess::VERTEX_SHADER | AttachmentAccess::FRAGMENT_SHADER)) != AttachmentAccess::NONE) {
                        attachment_data.initial_access_mask |= VK_ACCESS_SHADER_READ_BIT;
                    }

                    attachment_data.initial_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                } else {
                    attachment_data.initial_access_mask = VK_ACCESS_SHADER_READ_BIT;
                    attachment_data.initial_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }
                break;
            } else if ((attachment_access & AttachmentAccess::WRITE) == AttachmentAccess::WRITE) {
                if (TextureFormatUtils::is_depth_stencil(attachment_descriptor.format)) {
                    if ((attachment_access & AttachmentAccess::LOAD) == AttachmentAccess::LOAD) {
                        attachment_data.initial_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                    } else {
                        attachment_data.initial_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    }

                    attachment_data.initial_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                } else {
                    if ((attachment_access & (AttachmentAccess::LOAD | AttachmentAccess::BLEND)) != AttachmentAccess::NONE) {
                        attachment_data.initial_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                    } else {
                        attachment_data.initial_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    }

                    attachment_data.initial_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
                break;
            }
        }
    }

#ifdef KW_FRAME_GRAPH_LOG
    Log::print("[Frame Graph] Attachment initial access and layout:");

    for (size_t attachment_index = 0; attachment_index < m_attachment_descriptors.size(); attachment_index++) {
        const AttachmentDescriptor& attachment_descriptor = m_attachment_descriptors[attachment_index];
        const AttachmentData& attachment_data = m_attachment_data[attachment_index];

        Log::print("[Frame Graph] * Name: \"%s\"", attachment_descriptor.name);
        Log::print("[Frame Graph]   Initial access mask: 0x%x", attachment_data.initial_access_mask);
        Log::print("[Frame Graph]   Initial layout: %s", enum_to_string(attachment_data.initial_layout));
    }
#endif
}

void FrameGraphVulkan::create_render_passes(CreateContext& create_context) {
    const FrameGraphDescriptor& frame_graph_descriptor = create_context.frame_graph_descriptor;

#ifdef KW_FRAME_GRAPH_LOG
    Log::print("[Frame Graph] Render passes:");
#endif

    for (size_t render_pass_index = 0; render_pass_index < frame_graph_descriptor.render_pass_descriptor_count; render_pass_index++) {
        const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];
        RenderPassData& render_pass_data = m_render_pass_data[render_pass_index];

#ifdef KW_FRAME_GRAPH_LOG
        Log::print("[Frame Graph] * Name: \"%s\"", render_pass_descriptor.name);
#endif

        create_render_pass(create_context, render_pass_index);

#ifdef KW_FRAME_GRAPH_LOG
        Log::print("[Frame Graph]   Graphics pipelines:");
#endif

        KW_ASSERT(render_pass_data.graphics_pipeline_data.empty());
        render_pass_data.graphics_pipeline_data.resize(render_pass_descriptor.graphics_pipeline_descriptor_count, GraphicsPipelineData(m_render->persistent_memory_resource));

        for (size_t graphics_pipeline_index = 0; graphics_pipeline_index < render_pass_descriptor.graphics_pipeline_descriptor_count; graphics_pipeline_index++) {
            create_context.graphics_pipeline_memory_resource.reset();

            create_graphics_pipeline(create_context, render_pass_index, graphics_pipeline_index);
        }
    }
}

void FrameGraphVulkan::create_render_pass(CreateContext& create_context, size_t render_pass_index) {
    const FrameGraphDescriptor& frame_graph_descriptor = create_context.frame_graph_descriptor;

    const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];
    RenderPassData& render_pass_data = m_render_pass_data[render_pass_index];

    const VkPhysicalDeviceProperties& properties = m_render->physical_device_properties;
    const VkPhysicalDeviceLimits& limits = properties.limits;

    //
    // Compute the total number of attachments in this render pass.
    //

    size_t attachment_count = render_pass_descriptor.color_attachment_name_count;
    if (render_pass_descriptor.depth_stencil_attachment_name != nullptr) {
        attachment_count++;
    }

    //
    // Compute attachment descriptions: load and store operations, initial and final layouts.
    //

    Vector<VkAttachmentDescription> attachment_descriptions(attachment_count, m_render->transient_memory_resource);

    KW_ASSERT(render_pass_data.attachment_indices.empty());
    render_pass_data.attachment_indices.resize(attachment_count);

#ifdef KW_FRAME_GRAPH_LOG
    Log::print("[Frame Graph]   Attachments:");
#endif

    for (size_t i = 0; i < attachment_descriptions.size(); i++) {
        size_t attachment_index;
        VkImageLayout layout_attachment_optimal;
        VkImageLayout layout_read_only_optimal;

        if (i == render_pass_descriptor.color_attachment_name_count) {
            KW_ASSERT(create_context.attachment_mapping.count(render_pass_descriptor.depth_stencil_attachment_name) > 0);
            attachment_index = create_context.attachment_mapping[render_pass_descriptor.depth_stencil_attachment_name];
            KW_ASSERT(attachment_index < m_attachment_descriptors.size());

            layout_attachment_optimal = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            layout_read_only_optimal = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        } else {
            KW_ASSERT(create_context.attachment_mapping.count(render_pass_descriptor.color_attachment_names[i]) > 0);
            attachment_index = create_context.attachment_mapping[render_pass_descriptor.color_attachment_names[i]];
            KW_ASSERT(attachment_index < m_attachment_descriptors.size());

            layout_attachment_optimal = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            layout_read_only_optimal = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        const AttachmentDescriptor& attachment_descriptor = m_attachment_descriptors[attachment_index];
        VkAttachmentDescription& attachment_description = attachment_descriptions[i];

        attachment_description.flags = 0;
        attachment_description.format = TextureFormatUtils::convert_format_vulkan(attachment_descriptor.format);
        attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;

        size_t access_index = render_pass_index * m_attachment_descriptors.size() + attachment_index;
        KW_ASSERT(access_index < create_context.attachment_access_matrix.size());

        AttachmentAccess attachment_access = create_context.attachment_access_matrix[access_index];

        if ((attachment_access & AttachmentAccess::LOAD) == AttachmentAccess::NONE) {
            attachment_description.loadOp = LOAD_OP_MAPPING[static_cast<size_t>(attachment_descriptor.load_op)];
        } else {
            attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        }

        if (attachment_description.loadOp != VK_ATTACHMENT_LOAD_OP_LOAD) {
            // Clear and don't care render passes always start with undefined initial layout.
            attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        } else {
            bool previous_attachment_access_is_read = (attachment_access & AttachmentAccess::READ) == AttachmentAccess::READ;

            for (size_t offset = 1; offset < frame_graph_descriptor.render_pass_descriptor_count; offset++) {
                size_t previous_render_pass_index = (render_pass_index + frame_graph_descriptor.render_pass_descriptor_count - offset) % frame_graph_descriptor.render_pass_descriptor_count;
                
                size_t previous_access_index = previous_render_pass_index * m_attachment_descriptors.size() + attachment_index;
                KW_ASSERT(previous_access_index < create_context.attachment_access_matrix.size());
                
                AttachmentAccess previous_attachment_access = create_context.attachment_access_matrix[previous_access_index];

                if ((previous_attachment_access & AttachmentAccess::READ) == AttachmentAccess::READ) {
                    previous_attachment_access_is_read = true;
                    break;
                } else if ((previous_attachment_access & AttachmentAccess::WRITE) == AttachmentAccess::WRITE) {
                    previous_attachment_access_is_read = false;
                    break;
                }
            }

            if (previous_attachment_access_is_read) {
                // Previous render pass read this attachment.
                attachment_description.initialLayout = layout_read_only_optimal;
            } else {
                if ((attachment_access & AttachmentAccess::READ) == AttachmentAccess::READ) {
                    // Previous render pass written this attachment and changed its layout to read only optimal to
                    // avoid this render pass to do that (because read render passes are not allowed to change layout).
                    attachment_description.initialLayout = layout_read_only_optimal;
                } else {
                    // Previous render pass written this attachment.
                    attachment_description.initialLayout = layout_attachment_optimal;
                }
            }
        }

        if ((attachment_access & AttachmentAccess::STORE) == AttachmentAccess::STORE) {
            attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        } else {
            attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }

        if (attachment_description.storeOp != VK_ATTACHMENT_STORE_OP_STORE) {
            if (attachment_index == 0) {
                // Swapchain attachment must be transitioned to present image layout before present.
                attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            } else {
                // Don't care render passes are always write render passes, the next render pass always ignores the
                // attachment layout, so we can just keep our current layout.
                attachment_description.finalLayout = layout_attachment_optimal;
            }
        } else {
            bool next_attachment_access_is_read = (attachment_access & AttachmentAccess::READ) == AttachmentAccess::READ;

            for (size_t offset = 1; offset < frame_graph_descriptor.render_pass_descriptor_count; offset++) {
                size_t next_render_pass_index = (render_pass_index + offset) % frame_graph_descriptor.render_pass_descriptor_count;
                
                size_t next_access_index = next_render_pass_index * m_attachment_descriptors.size() + attachment_index;
                KW_ASSERT(next_access_index < create_context.attachment_access_matrix.size());

                AttachmentAccess next_attachment_access = create_context.attachment_access_matrix[next_access_index];

                if ((next_attachment_access & AttachmentAccess::READ) == AttachmentAccess::READ) {
                    next_attachment_access_is_read = true;
                    break;
                } else if ((next_attachment_access & AttachmentAccess::WRITE) == AttachmentAccess::WRITE) {
                    next_attachment_access_is_read = false;
                    break;
                }
            }

            if (next_attachment_access_is_read) {
                // This render pass is followed by a render pass that reads this attachment.
                attachment_description.finalLayout = layout_read_only_optimal;
            } else {
                if ((attachment_access & AttachmentAccess::READ) == AttachmentAccess::READ) {
                    // Next render pass is a write render pass, but we're not allowed to change attachment layout
                    // in this render pass because it's read only.
                    attachment_description.finalLayout = layout_read_only_optimal;
                } else {
                    // Next render pass is a write render pass, so just keep our current layout.
                    attachment_description.finalLayout = layout_attachment_optimal;
                }
            }
        }

        attachment_description.stencilLoadOp = attachment_description.loadOp;
        attachment_description.stencilStoreOp = attachment_description.storeOp;

        KW_ASSERT(render_pass_data.attachment_indices[i] == 0);
        render_pass_data.attachment_indices[i] = attachment_index;

#ifdef KW_FRAME_GRAPH_LOG
        Log::print("[Frame Graph]   * Name: \"%s\"", attachment_descriptor.name);
        Log::print("[Frame Graph]     Load op: %s", enum_to_string(attachment_description.loadOp));
        Log::print("[Frame Graph]     Store op: %s", enum_to_string(attachment_description.storeOp));
        Log::print("[Frame Graph]     Initial layout: %s", enum_to_string(attachment_description.initialLayout));
        Log::print("[Frame Graph]     Final layout: %s", enum_to_string(attachment_description.finalLayout));
#endif
    }

    //
    // Set up attachment references.
    //

    Vector<VkAttachmentReference> color_attachment_references(render_pass_descriptor.color_attachment_name_count, m_render->transient_memory_resource);
    for (size_t i = 0; i < color_attachment_references.size(); i++) {
        color_attachment_references[i].attachment = i;
        color_attachment_references[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkAttachmentReference depth_stencil_attachment_reference{};
    depth_stencil_attachment_reference.attachment = static_cast<uint32_t>(render_pass_descriptor.color_attachment_name_count);
    depth_stencil_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    // Check whether depth stencil attachment is actually written by this render pass.
    if (render_pass_descriptor.depth_stencil_attachment_name != nullptr) {
        KW_ASSERT(create_context.attachment_mapping.count(render_pass_descriptor.depth_stencil_attachment_name) > 0);
        size_t attachment_index = create_context.attachment_mapping[render_pass_descriptor.depth_stencil_attachment_name];
        KW_ASSERT(attachment_index < m_attachment_descriptors.size());

        size_t access_index = render_pass_index * m_attachment_descriptors.size() + attachment_index;
        KW_ASSERT(access_index < create_context.attachment_access_matrix.size());

        AttachmentAccess attachment_access = create_context.attachment_access_matrix[access_index];

        if ((attachment_access & AttachmentAccess::WRITE) == AttachmentAccess::WRITE) {
            depth_stencil_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
    }

    //
    // Set up subpass and create the render pass.
    //

    KW_ERROR(
        color_attachment_references.size() < limits.maxColorAttachments,
        "Too many color attachments. Max %u, got %zu.", limits.maxColorAttachments, color_attachment_references.size()
    );
    
    VkSubpassDescription subpass_description{};
    subpass_description.flags = 0;
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = nullptr;
    subpass_description.colorAttachmentCount = static_cast<uint32_t>(color_attachment_references.size());
    subpass_description.pColorAttachments = color_attachment_references.data();
    subpass_description.pResolveAttachments = nullptr;
    if (render_pass_descriptor.depth_stencil_attachment_name != nullptr) {
        subpass_description.pDepthStencilAttachment = &depth_stencil_attachment_reference;
    }
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = nullptr;

    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.flags = 0;
    render_pass_create_info.attachmentCount = static_cast<uint32_t>(attachment_descriptions.size());
    render_pass_create_info.pAttachments = attachment_descriptions.data();
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_description;
    render_pass_create_info.dependencyCount = 0;
    render_pass_create_info.pDependencies = nullptr;

    KW_ASSERT(render_pass_data.render_pass == VK_NULL_HANDLE);
    VK_ERROR(
        vkCreateRenderPass(m_render->device, &render_pass_create_info, &m_render->allocation_callbacks, &render_pass_data.render_pass),
        "Failed to create render pass \"%s\".", render_pass_descriptor.name
    );
    VK_NAME(m_render, render_pass_data.render_pass, "Render pass \"%s\"", render_pass_descriptor.name);
}

void FrameGraphVulkan::create_graphics_pipeline(CreateContext& create_context, size_t render_pass_index, size_t graphics_pipeline_index) {
    const FrameGraphDescriptor& frame_graph_descriptor = create_context.frame_graph_descriptor;
    const RenderPassDescriptor& render_pass_descriptor = frame_graph_descriptor.render_pass_descriptors[render_pass_index];
    const GraphicsPipelineDescriptor& graphics_pipeline_descriptor = render_pass_descriptor.graphics_pipeline_descriptors[graphics_pipeline_index];

    RenderPassData& render_pass_data = m_render_pass_data[render_pass_index];
    GraphicsPipelineData& graphics_pipeline_data = render_pass_data.graphics_pipeline_data[graphics_pipeline_index];

    const VkPhysicalDeviceProperties& properties = m_render->physical_device_properties;
    const VkPhysicalDeviceLimits& limits = properties.limits;

    //
    // Calculate the number of pipeline stages.
    //

    uint32_t stage_count = 1;
    if (graphics_pipeline_descriptor.fragment_shader_filename != nullptr) {
        stage_count++;
    }

    //
    // Read shaders from file system and query their reflection.
    //

    SpvReflectShaderModule vertex_shader_reflection;
    SpvReflectShaderModule fragment_shader_reflection;

    SpvAllocator spv_allocator{};
    spv_allocator.calloc = &spv_calloc;
    spv_allocator.free = &spv_free;
    spv_allocator.context = &create_context.graphics_pipeline_memory_resource;

    {
        String relative_path(graphics_pipeline_descriptor.vertex_shader_filename, create_context.graphics_pipeline_memory_resource);
        relative_path.replace(relative_path.find(".hlsl"), 5, ".spv");

        Vector<std::byte> shader_data = FilesystemUtils::read_file(create_context.graphics_pipeline_memory_resource, relative_path);

        SPV_ERROR(
            spvReflectCreateShaderModule(shader_data.size(), shader_data.data(), &vertex_shader_reflection, &spv_allocator),
            "Failed to create shader module from \"%s\".", graphics_pipeline_descriptor.vertex_shader_filename
        );

        KW_ERROR(
            spvReflectGetEntryPoint(&vertex_shader_reflection, "main") != nullptr,
            "Shader \"%s\" must have entry point \"main\".", graphics_pipeline_descriptor.vertex_shader_filename
        );
    }

    if (graphics_pipeline_descriptor.fragment_shader_filename != nullptr) {
        String relative_path(graphics_pipeline_descriptor.fragment_shader_filename, create_context.graphics_pipeline_memory_resource);
        relative_path.replace(relative_path.find(".hlsl"), 5, ".spv");

        Vector<std::byte> shader_data = FilesystemUtils::read_file(create_context.graphics_pipeline_memory_resource, relative_path);

        SPV_ERROR(
            spvReflectCreateShaderModule(shader_data.size(), shader_data.data(), &fragment_shader_reflection, &spv_allocator),
            "Failed to create shader module from \"%s\".", graphics_pipeline_descriptor.fragment_shader_filename
        );

        KW_ERROR(
            spvReflectGetEntryPoint(&fragment_shader_reflection, "main") != nullptr,
            "Shader \"%s\" must have entry point \"main\".", graphics_pipeline_descriptor.fragment_shader_filename
        );
    }

    //
    // Log original shader reflection.
    //

#ifdef KW_FRAME_GRAPH_LOG
    Log::print("[Frame Graph]   * Original vertex shader:");

    log_shader_reflection(vertex_shader_reflection, create_context.graphics_pipeline_memory_resource);

    if (graphics_pipeline_descriptor.fragment_shader_filename != nullptr) {
        Log::print("[Frame Graph]     Original fragment shader:");

        log_shader_reflection(fragment_shader_reflection, create_context.graphics_pipeline_memory_resource);
    }
#endif

    //
    // Assign descriptor binding numbers.
    //

    UnorderedMap<StringView, uint32_t> shared_descriptor_mapping(create_context.graphics_pipeline_memory_resource);
    UnorderedMap<StringView, uint32_t> exclusive_descriptor_mapping(create_context.graphics_pipeline_memory_resource);
    uint32_t descriptor_binding_number = 0;

    for (size_t uniform_attachment_index = 0; uniform_attachment_index < graphics_pipeline_descriptor.uniform_attachment_descriptor_count; uniform_attachment_index++) {
        const UniformAttachmentDescriptor& uniform_attachment_descriptor = graphics_pipeline_descriptor.uniform_attachment_descriptors[uniform_attachment_index];
        if (uniform_attachment_descriptor.visibility == ShaderVisibility::ALL) {
            shared_descriptor_mapping.emplace(StringView(uniform_attachment_descriptor.variable_name), descriptor_binding_number++);
        } else if (uniform_attachment_descriptor.visibility == ShaderVisibility::VERTEX) {
            exclusive_descriptor_mapping.emplace(StringView(uniform_attachment_descriptor.variable_name), descriptor_binding_number++);
        } else {
            KW_ASSERT(uniform_attachment_descriptor.visibility == ShaderVisibility::FRAGMENT);
            exclusive_descriptor_mapping.emplace(StringView(uniform_attachment_descriptor.variable_name), descriptor_binding_number++);
        }
    }

    for (size_t uniform_buffer_index = 0; uniform_buffer_index < graphics_pipeline_descriptor.uniform_buffer_descriptor_count; uniform_buffer_index++) {
        const UniformDescriptor& uniform_descriptor = graphics_pipeline_descriptor.uniform_buffer_descriptors[uniform_buffer_index];
        if (uniform_descriptor.visibility == ShaderVisibility::ALL) {
            shared_descriptor_mapping.emplace(StringView(uniform_descriptor.variable_name), descriptor_binding_number++);
        } else if (uniform_descriptor.visibility == ShaderVisibility::VERTEX) {
            exclusive_descriptor_mapping.emplace(StringView(uniform_descriptor.variable_name), descriptor_binding_number++);
        } else {
            KW_ASSERT(uniform_descriptor.visibility == ShaderVisibility::FRAGMENT);
            exclusive_descriptor_mapping.emplace(StringView(uniform_descriptor.variable_name), descriptor_binding_number++);
        }
    }

    for (size_t texture_index = 0; texture_index < graphics_pipeline_descriptor.texture_descriptor_count; texture_index++) {
        const UniformDescriptor& uniform_descriptor = graphics_pipeline_descriptor.texture_descriptors[texture_index];
        if (uniform_descriptor.visibility == ShaderVisibility::ALL) {
            shared_descriptor_mapping.emplace(StringView(uniform_descriptor.variable_name), descriptor_binding_number++);
        } else if (uniform_descriptor.visibility == ShaderVisibility::VERTEX) {
            exclusive_descriptor_mapping.emplace(StringView(uniform_descriptor.variable_name), descriptor_binding_number++);
        } else {
            KW_ASSERT(uniform_descriptor.visibility == ShaderVisibility::FRAGMENT);
            exclusive_descriptor_mapping.emplace(StringView(uniform_descriptor.variable_name), descriptor_binding_number++);
        }
    }

    for (size_t sampler_index = 0; sampler_index < graphics_pipeline_descriptor.sampler_descriptor_count; sampler_index++) {
        const SamplerDescriptor& sampler_descriptor = graphics_pipeline_descriptor.sampler_descriptors[sampler_index];
        if (sampler_descriptor.visibility == ShaderVisibility::ALL) {
            shared_descriptor_mapping.emplace(StringView(sampler_descriptor.variable_name), descriptor_binding_number++);
        } else if (sampler_descriptor.visibility == ShaderVisibility::VERTEX) {
            exclusive_descriptor_mapping.emplace(StringView(sampler_descriptor.variable_name), descriptor_binding_number++);
        } else {
            KW_ASSERT(sampler_descriptor.visibility == ShaderVisibility::FRAGMENT);
            exclusive_descriptor_mapping.emplace(StringView(sampler_descriptor.variable_name), descriptor_binding_number++);
        }
    }

    //
    // Change descriptor binding numbers in SPIR-V code.
    //

    {
        for (uint32_t i = 0; i < vertex_shader_reflection.descriptor_binding_count; i++) {
            const SpvReflectDescriptorBinding& descriptor_binding = vertex_shader_reflection.descriptor_bindings[i];

            KW_ERROR(
                descriptor_binding.name != nullptr,
                "Invalid descriptor binding name in \"%s\".", graphics_pipeline_descriptor.vertex_shader_filename
            );

            StringView descriptor_binding_name(descriptor_binding.name);

            auto shared_descriptor = shared_descriptor_mapping.find(descriptor_binding_name);
            if (shared_descriptor != shared_descriptor_mapping.end()) {
                SPV_ERROR(
                    spvReflectChangeDescriptorBindingNumbers(&vertex_shader_reflection, &descriptor_binding, shared_descriptor->second, 0, &spv_allocator),
                    "Failed to change descriptor binding \"%s\" number in \"%s\".", descriptor_binding_name.data(), graphics_pipeline_descriptor.vertex_shader_filename
                );
            } else {
                auto exclusive_descriptor = exclusive_descriptor_mapping.find(descriptor_binding_name);
                
                KW_ERROR(
                    exclusive_descriptor != exclusive_descriptor_mapping.end(),
                    "Unbound descriptor binding \"%s\".", descriptor_binding.name
                );

                SPV_ERROR(
                    spvReflectChangeDescriptorBindingNumbers(&vertex_shader_reflection, &descriptor_binding, exclusive_descriptor->second, 0, &spv_allocator),
                    "Failed to change descriptor binding \"%s\" number in \"%s\".", descriptor_binding_name.data(), graphics_pipeline_descriptor.vertex_shader_filename
                );
            }
        }
    }

    if (graphics_pipeline_descriptor.fragment_shader_filename != nullptr) {
        for (uint32_t i = 0; i < fragment_shader_reflection.descriptor_binding_count; i++) {
            const SpvReflectDescriptorBinding& descriptor_binding = fragment_shader_reflection.descriptor_bindings[i];

            KW_ERROR(
                descriptor_binding.name != nullptr,
                "Invalid descriptor binding name in \"%s\".", graphics_pipeline_descriptor.fragment_shader_filename
            );

            StringView descriptor_binding_name(descriptor_binding.name);

            auto shared_descriptor = shared_descriptor_mapping.find(descriptor_binding_name);
            if (shared_descriptor != shared_descriptor_mapping.end()) {
                SPV_ERROR(
                    spvReflectChangeDescriptorBindingNumbers(&fragment_shader_reflection, &descriptor_binding, shared_descriptor->second, 0, &spv_allocator),
                    "Failed to change descriptor binding \"%s\" number in \"%s\".", descriptor_binding_name.data(), graphics_pipeline_descriptor.fragment_shader_filename
                );
            } else {
                auto exclusive_descriptor = exclusive_descriptor_mapping.find(descriptor_binding_name);

                KW_ERROR(
                    exclusive_descriptor != exclusive_descriptor_mapping.end(),
                    "Unbound descriptor binding \"%s\".", descriptor_binding.name
                );

                SPV_ERROR(
                    spvReflectChangeDescriptorBindingNumbers(&fragment_shader_reflection, &descriptor_binding, exclusive_descriptor->second, 0, &spv_allocator),
                    "Failed to change descriptor binding \"%s\" number in \"%s\".", descriptor_binding_name.data(), graphics_pipeline_descriptor.fragment_shader_filename
                );
            }
        }
    }

    //
    // Link vertex output variables to fragment input variables.
    //

    if (graphics_pipeline_descriptor.fragment_shader_filename != nullptr) {
        KW_ERROR(
            vertex_shader_reflection.output_variable_count == fragment_shader_reflection.input_variable_count,
            "Mismatching number of variables between shader stages in \"%s\" and \"%s\"", graphics_pipeline_descriptor.vertex_shader_filename, graphics_pipeline_descriptor.fragment_shader_filename
        );

        for (size_t i = 0; i < vertex_shader_reflection.output_variable_count; i++) {
            const SpvReflectInterfaceVariable* output_variable = vertex_shader_reflection.output_variables[i];
            
            KW_ERROR(
                output_variable != nullptr,
                "Invalid output variable in \"%s\".", graphics_pipeline_descriptor.vertex_shader_filename
            );
            
            KW_ERROR(
                output_variable->semantic != nullptr,
                "Invalid output variable semantic in \"%s\".", graphics_pipeline_descriptor.vertex_shader_filename
            );

            const SpvReflectInterfaceVariable* input_variable = spvReflectGetInputVariableBySemantic(&fragment_shader_reflection, output_variable->semantic, nullptr);

            KW_ERROR(
                input_variable != nullptr,
                "Failed to find fragment shader input variable \"%s\" in \"%s\".", output_variable->semantic, graphics_pipeline_descriptor.fragment_shader_filename
            );

            if (output_variable->location != input_variable->location) {
                SPV_ERROR(
                    spvReflectChangeInputVariableLocation(&fragment_shader_reflection, input_variable, output_variable->location),
                    "Failed to change fragment shader input variable \"%s\" location in \"%s\".", input_variable->location, graphics_pipeline_descriptor.fragment_shader_filename
                );
            }
        }
    }

    //
    // Log patched shader reflection.
    //

#ifdef KW_FRAME_GRAPH_LOG
    Log::print("[Frame Graph]     Patched vertex shader:");

    log_shader_reflection(vertex_shader_reflection, create_context.graphics_pipeline_memory_resource);

    if (graphics_pipeline_descriptor.fragment_shader_filename != nullptr) {
        Log::print("[Frame Graph]     Patched fragment shader:");

        log_shader_reflection(fragment_shader_reflection, create_context.graphics_pipeline_memory_resource);
    }
#endif

    //
    // Create shader modules for `VkPipelineShaderStageCreateInfo`.
    //

    {
        VkShaderModuleCreateInfo vertex_shader_module_create_info{};
        vertex_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vertex_shader_module_create_info.flags = 0;
        vertex_shader_module_create_info.codeSize = spvReflectGetCodeSize(&vertex_shader_reflection);
        vertex_shader_module_create_info.pCode = spvReflectGetCode(&vertex_shader_reflection);

        KW_ASSERT(graphics_pipeline_data.vertex_shader_module == VK_NULL_HANDLE);
        VK_ERROR(
            vkCreateShaderModule(m_render->device, &vertex_shader_module_create_info, &m_render->allocation_callbacks, &graphics_pipeline_data.vertex_shader_module),
            "Failed to create vertex shader module from \"%s\".", graphics_pipeline_descriptor.vertex_shader_filename
        );
        VK_NAME(
            m_render, graphics_pipeline_data.vertex_shader_module,
            "Vertex shader \"%s\"", graphics_pipeline_descriptor.name
        );
    }

    if (graphics_pipeline_descriptor.fragment_shader_filename != nullptr) {
        VkShaderModuleCreateInfo fragment_shader_module_create_info{};
        fragment_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        fragment_shader_module_create_info.flags = 0;
        fragment_shader_module_create_info.codeSize = spvReflectGetCodeSize(&fragment_shader_reflection);
        fragment_shader_module_create_info.pCode = spvReflectGetCode(&fragment_shader_reflection);

        KW_ASSERT(graphics_pipeline_data.fragment_shader_module == VK_NULL_HANDLE);
        VK_ERROR(
            vkCreateShaderModule(m_render->device, &fragment_shader_module_create_info, &m_render->allocation_callbacks, &graphics_pipeline_data.fragment_shader_module),
            "Failed to create fragment shader module from \"%s\".", graphics_pipeline_descriptor.fragment_shader_filename
        );
        VK_NAME(
            m_render, graphics_pipeline_data.fragment_shader_module,
            "Fragment shader \"%s\"", graphics_pipeline_descriptor.name
        );
    }

    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_infos[2]{};
    pipeline_shader_stage_create_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_shader_stage_create_infos[0].flags = 0;
    pipeline_shader_stage_create_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    pipeline_shader_stage_create_infos[0].module = graphics_pipeline_data.vertex_shader_module;
    pipeline_shader_stage_create_infos[0].pName = "main";
    pipeline_shader_stage_create_infos[0].pSpecializationInfo = nullptr;
    pipeline_shader_stage_create_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_shader_stage_create_infos[1].flags = 0;
    pipeline_shader_stage_create_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipeline_shader_stage_create_infos[1].module = graphics_pipeline_data.fragment_shader_module;
    pipeline_shader_stage_create_infos[1].pName = "main";
    pipeline_shader_stage_create_infos[1].pSpecializationInfo = nullptr;

    //
    // Sort out vertex bindings and attributes for `VkPipelineVertexInputStateCreateInfo`.
    //

    size_t instance_binding_offset = graphics_pipeline_descriptor.vertex_binding_descriptor_count;
    size_t vertex_attribute_count = 0;

    Vector<VkVertexInputBindingDescription> vertex_input_binding_descriptors(create_context.graphics_pipeline_memory_resource);
    vertex_input_binding_descriptors.reserve(graphics_pipeline_descriptor.vertex_binding_descriptor_count + graphics_pipeline_descriptor.instance_binding_descriptor_count);

    for (size_t i = 0; i < graphics_pipeline_descriptor.vertex_binding_descriptor_count; i++) {
        const BindingDescriptor& binding_descriptor = graphics_pipeline_descriptor.vertex_binding_descriptors[i];

        KW_ERROR(
            binding_descriptor.stride < limits.maxVertexInputBindingStride,
            "Binding stride overflow. Max %u, got %zu.", limits.maxVertexInputBindingStride, binding_descriptor.stride
        );

        VkVertexInputBindingDescription vertex_binding_description{};
        vertex_binding_description.binding = static_cast<uint32_t>(i);
        vertex_binding_description.stride = static_cast<uint32_t>(binding_descriptor.stride);
        vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        vertex_input_binding_descriptors.push_back(vertex_binding_description);

        vertex_attribute_count += binding_descriptor.attribute_descriptor_count;
    }

    for (size_t i = 0; i < graphics_pipeline_descriptor.instance_binding_descriptor_count; i++) {
        const BindingDescriptor& binding_descriptor = graphics_pipeline_descriptor.vertex_binding_descriptors[i];

        KW_ERROR(
            binding_descriptor.stride < limits.maxVertexInputBindingStride,
            "Binding stride overflow. Max %u, got %zu.", limits.maxVertexInputBindingStride, binding_descriptor.stride
        );

        VkVertexInputBindingDescription vertex_binding_description{};
        vertex_binding_description.binding = static_cast<uint32_t>(instance_binding_offset + i);
        vertex_binding_description.stride = static_cast<uint32_t>(binding_descriptor.stride);
        vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
        vertex_input_binding_descriptors.push_back(vertex_binding_description);

        vertex_attribute_count += binding_descriptor.attribute_descriptor_count;
    }

    KW_ERROR(
        vertex_shader_reflection.input_variable_count == vertex_attribute_count,
        "Mismatching number of variables in vertex shader \"%s\".", graphics_pipeline_descriptor.vertex_shader_filename
    );

    Vector<VkVertexInputAttributeDescription> vertex_input_attribute_descriptions(create_context.graphics_pipeline_memory_resource);
    vertex_input_attribute_descriptions.reserve(vertex_attribute_count);

    for (size_t i = 0; i < graphics_pipeline_descriptor.vertex_binding_descriptor_count; i++) {
        const BindingDescriptor& binding_descriptor = graphics_pipeline_descriptor.vertex_binding_descriptors[i];

        for (size_t j = 0; j < graphics_pipeline_descriptor.vertex_binding_descriptors[i].attribute_descriptor_count; j++) {
            const AttributeDescriptor& attribute_descriptor = binding_descriptor.attribute_descriptors[j];

            char semantic[32]{};
            sprintf_s(semantic, sizeof(semantic), "%s%zu", SEMANTIC_STRINGS[static_cast<size_t>(attribute_descriptor.semantic)], attribute_descriptor.semantic_index);

            const SpvReflectInterfaceVariable* interface_variable =
                spvReflectGetInputVariableBySemantic(&vertex_shader_reflection, semantic, nullptr);
            
            // "POSITION" and "POSITION0" is the same semantic.
            if (interface_variable == nullptr && attribute_descriptor.semantic_index == 0) {
                interface_variable = spvReflectGetInputVariableBySemantic(&vertex_shader_reflection, SEMANTIC_STRINGS[static_cast<size_t>(attribute_descriptor.semantic)], nullptr);
            }

            KW_ERROR(
                interface_variable != nullptr,
                "Failed to find input variable by semantic \"%s\".", semantic
            );

            KW_ERROR(
                attribute_descriptor.offset < limits.maxVertexInputAttributeOffset,
                "Attribute offset overflow. Max %u, got %zu.", limits.maxVertexInputAttributeOffset, attribute_descriptor.offset
            );

            VkVertexInputAttributeDescription vertex_input_attribute_description{};
            vertex_input_attribute_description.location = interface_variable->location;
            vertex_input_attribute_description.binding = static_cast<uint32_t>(i);
            vertex_input_attribute_description.format = TextureFormatUtils::convert_format_vulkan(attribute_descriptor.format);
            vertex_input_attribute_description.offset = static_cast<uint32_t>(attribute_descriptor.offset);
            vertex_input_attribute_descriptions.push_back(vertex_input_attribute_description);
        }
    }

    for (size_t i = 0; i < graphics_pipeline_descriptor.instance_binding_descriptor_count; i++) {
        const BindingDescriptor& binding_descriptor = graphics_pipeline_descriptor.instance_binding_descriptors[i];

        for (size_t j = 0; j < graphics_pipeline_descriptor.instance_binding_descriptors[i].attribute_descriptor_count; j++) {
            const AttributeDescriptor& attribute_descriptor = binding_descriptor.attribute_descriptors[j];

            char semantic[32]{};
            sprintf_s(semantic, sizeof(semantic), "%s%zu", SEMANTIC_STRINGS[static_cast<size_t>(attribute_descriptor.semantic)], attribute_descriptor.semantic_index);

            const SpvReflectInterfaceVariable* interface_variable =
                spvReflectGetInputVariableBySemantic(&vertex_shader_reflection, semantic, nullptr);

            // "TEXCOORD" and "TEXCOORD0" is the same semantic.
            if (interface_variable == nullptr && attribute_descriptor.semantic_index == 0) {
                interface_variable = spvReflectGetInputVariableBySemantic(&vertex_shader_reflection, SEMANTIC_STRINGS[static_cast<size_t>(attribute_descriptor.semantic)], nullptr);
            }
            
            KW_ERROR(
                interface_variable != nullptr,
                "Failed to find input variable by semantic \"%s\".", semantic
            );

            KW_ERROR(
                attribute_descriptor.offset < limits.maxVertexInputAttributeOffset,
                "Attribute offset overflow. Max %u, got %zu.", limits.maxVertexInputAttributeOffset, attribute_descriptor.offset
            );

            VkVertexInputAttributeDescription vertex_input_attribute_description{};
            vertex_input_attribute_description.location = interface_variable->location;
            vertex_input_attribute_description.binding = static_cast<uint32_t>(instance_binding_offset + i);
            vertex_input_attribute_description.format = TextureFormatUtils::convert_format_vulkan(attribute_descriptor.format);
            vertex_input_attribute_description.offset = static_cast<uint32_t>(attribute_descriptor.offset);
            vertex_input_attribute_descriptions.push_back(vertex_input_attribute_description);
        }
    }

    KW_ERROR(
        vertex_input_binding_descriptors.size() < limits.maxVertexInputBindings,
        "Binding overflow. Max %u, got %zu.", limits.maxVertexInputBindings, vertex_input_binding_descriptors.size()
    );

    KW_ERROR(
        vertex_input_attribute_descriptions.size() < limits.maxVertexInputAttributes,
        "Attribute overflow. Max %u, got %zu.", limits.maxVertexInputAttributes, vertex_input_attribute_descriptions.size()
    );

    KW_ERROR(
        vertex_shader_reflection.output_variable_count <= limits.maxVertexOutputComponents,
        "Too many output variables in vertex shader. Max %u, got %zu.", limits.maxVertexOutputComponents, vertex_shader_reflection.output_variable_count
    );

    if (graphics_pipeline_descriptor.fragment_shader_filename != nullptr) {
        KW_ERROR(
            fragment_shader_reflection.input_variable_count <= limits.maxFragmentInputComponents,
            "Too many input variables in fragment shader. Max %u, got %zu.", limits.maxFragmentInputComponents, fragment_shader_reflection.input_variable_count
        );

        KW_ERROR(
            fragment_shader_reflection.output_variable_count <= limits.maxFragmentOutputAttachments,
            "Too many output attachments in fragment shader. Max %u, got %zu.", limits.maxFragmentOutputAttachments, fragment_shader_reflection.output_variable_count
        );
    }

    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info{};
    pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipeline_vertex_input_state_create_info.flags = 0;
    pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_input_binding_descriptors.size());
    pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = vertex_input_binding_descriptors.data();
    pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attribute_descriptions.size());
    pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_input_attribute_descriptions.data();

    //
    // Other basic descriptors.
    //

    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info{};
    pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipeline_input_assembly_state_create_info.flags = 0;
    pipeline_input_assembly_state_create_info.topology = PRIMITIVE_TOPOLOGY_MAPPING[static_cast<size_t>(graphics_pipeline_descriptor.primitive_topology)];
    pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info{};
    pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipeline_viewport_state_create_info.flags = 0;
    pipeline_viewport_state_create_info.viewportCount = 1;
    pipeline_viewport_state_create_info.pViewports = nullptr;
    pipeline_viewport_state_create_info.scissorCount = 1;
    pipeline_viewport_state_create_info.pScissors = nullptr;

    bool is_depth_bias_enabled =
        graphics_pipeline_descriptor.depth_bias_constant_factor != 0.f ||
        graphics_pipeline_descriptor.depth_bias_clamp != 0.f ||
        graphics_pipeline_descriptor.depth_bias_slope_factor != 0.f;

    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info{};
    pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipeline_rasterization_state_create_info.flags = 0;
    pipeline_rasterization_state_create_info.depthClampEnable = VK_FALSE;
    pipeline_rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    pipeline_rasterization_state_create_info.polygonMode = FILL_MODE_MAPPING[static_cast<size_t>(graphics_pipeline_descriptor.fill_mode)];
    pipeline_rasterization_state_create_info.cullMode = CULL_MODE_MAPPING[static_cast<size_t>(graphics_pipeline_descriptor.cull_mode)];
    pipeline_rasterization_state_create_info.frontFace = FRONT_FACE_MAPPING[static_cast<size_t>(graphics_pipeline_descriptor.front_face)];
    pipeline_rasterization_state_create_info.depthBiasEnable = is_depth_bias_enabled;
    pipeline_rasterization_state_create_info.depthBiasConstantFactor = graphics_pipeline_descriptor.depth_bias_constant_factor;
    pipeline_rasterization_state_create_info.depthBiasClamp = graphics_pipeline_descriptor.depth_bias_clamp;
    pipeline_rasterization_state_create_info.depthBiasSlopeFactor = graphics_pipeline_descriptor.depth_bias_slope_factor;
    pipeline_rasterization_state_create_info.lineWidth = 1.f;

    VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info{};
    pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipeline_multisample_state_create_info.flags = 0;
    pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipeline_multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    pipeline_multisample_state_create_info.minSampleShading = 0.f;
    pipeline_multisample_state_create_info.pSampleMask = nullptr;
    pipeline_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    pipeline_multisample_state_create_info.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info{};
    pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipeline_depth_stencil_state_create_info.flags = 0;
    pipeline_depth_stencil_state_create_info.depthTestEnable = graphics_pipeline_descriptor.is_depth_test_enabled;
    pipeline_depth_stencil_state_create_info.depthWriteEnable = graphics_pipeline_descriptor.is_depth_write_enabled;
    pipeline_depth_stencil_state_create_info.depthCompareOp = COMPARE_OP_MAPPING[static_cast<size_t>(graphics_pipeline_descriptor.depth_compare_op)];
    pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    pipeline_depth_stencil_state_create_info.stencilTestEnable = graphics_pipeline_descriptor.is_stencil_test_enabled;
    pipeline_depth_stencil_state_create_info.front.failOp = STENCIL_OP_MAPPING[static_cast<size_t>(graphics_pipeline_descriptor.front_stencil_op_state.fail_op)];
    pipeline_depth_stencil_state_create_info.front.passOp = STENCIL_OP_MAPPING[static_cast<size_t>(graphics_pipeline_descriptor.front_stencil_op_state.pass_op)];
    pipeline_depth_stencil_state_create_info.front.depthFailOp = STENCIL_OP_MAPPING[static_cast<size_t>(graphics_pipeline_descriptor.front_stencil_op_state.depth_fail_op)];
    pipeline_depth_stencil_state_create_info.front.compareOp = COMPARE_OP_MAPPING[static_cast<size_t>(graphics_pipeline_descriptor.front_stencil_op_state.compare_op)];
    pipeline_depth_stencil_state_create_info.front.compareMask = graphics_pipeline_descriptor.stencil_compare_mask;
    pipeline_depth_stencil_state_create_info.front.writeMask = graphics_pipeline_descriptor.stencil_write_mask;
    pipeline_depth_stencil_state_create_info.front.reference = 0;
    pipeline_depth_stencil_state_create_info.back.failOp = STENCIL_OP_MAPPING[static_cast<size_t>(graphics_pipeline_descriptor.back_stencil_op_state.fail_op)];
    pipeline_depth_stencil_state_create_info.back.passOp = STENCIL_OP_MAPPING[static_cast<size_t>(graphics_pipeline_descriptor.back_stencil_op_state.pass_op)];
    pipeline_depth_stencil_state_create_info.back.depthFailOp = STENCIL_OP_MAPPING[static_cast<size_t>(graphics_pipeline_descriptor.back_stencil_op_state.depth_fail_op)];
    pipeline_depth_stencil_state_create_info.back.compareOp = COMPARE_OP_MAPPING[static_cast<size_t>(graphics_pipeline_descriptor.back_stencil_op_state.compare_op)];
    pipeline_depth_stencil_state_create_info.back.compareMask = graphics_pipeline_descriptor.stencil_compare_mask;
    pipeline_depth_stencil_state_create_info.back.writeMask = graphics_pipeline_descriptor.stencil_write_mask;
    pipeline_depth_stencil_state_create_info.back.reference = 0;
    pipeline_depth_stencil_state_create_info.minDepthBounds = 0.f;
    pipeline_depth_stencil_state_create_info.maxDepthBounds = 0.f;

    VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info{};
    pipeline_dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipeline_dynamic_state_create_info.flags = 0;
    pipeline_dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(std::size(DYNAMIC_STATES));
    pipeline_dynamic_state_create_info.pDynamicStates = DYNAMIC_STATES;

    //
    // `GraphicsPipelineDescriptor` contains only those attachments that need color blending.
    // Other attachments implicitly have `blendEnable` equal to `VK_FALSE`.
    //

    Vector<VkPipelineColorBlendAttachmentState> pipeline_color_blend_attachment_states(create_context.graphics_pipeline_memory_resource);
    pipeline_color_blend_attachment_states.resize(render_pass_descriptor.color_attachment_name_count);

    for (size_t i = 0; i < graphics_pipeline_descriptor.attachment_blend_descriptor_count; i++) {
        const AttachmentBlendDescriptor& attachment_blend_descriptor = graphics_pipeline_descriptor.attachment_blend_descriptors[i];

        for (size_t j = 0; j < render_pass_descriptor.color_attachment_name_count; j++) {
            if (strcmp(render_pass_descriptor.color_attachment_names[j], attachment_blend_descriptor.attachment_name) == 0) {
                VkPipelineColorBlendAttachmentState& pipeline_color_blend_attachment_state = pipeline_color_blend_attachment_states[j];

                pipeline_color_blend_attachment_state.blendEnable = VK_TRUE;
                pipeline_color_blend_attachment_state.srcColorBlendFactor = BLEND_FACTOR_MAPPING[static_cast<size_t>(attachment_blend_descriptor.source_color_blend_factor)];
                pipeline_color_blend_attachment_state.dstColorBlendFactor = BLEND_FACTOR_MAPPING[static_cast<size_t>(attachment_blend_descriptor.destination_color_blend_factor)];
                pipeline_color_blend_attachment_state.colorBlendOp = BLEND_OP_MAPPING[static_cast<size_t>(attachment_blend_descriptor.color_blend_op)];
                pipeline_color_blend_attachment_state.srcAlphaBlendFactor = BLEND_FACTOR_MAPPING[static_cast<size_t>(attachment_blend_descriptor.source_alpha_blend_factor)];
                pipeline_color_blend_attachment_state.dstAlphaBlendFactor = BLEND_FACTOR_MAPPING[static_cast<size_t>(attachment_blend_descriptor.destination_alpha_blend_factor)];
                pipeline_color_blend_attachment_state.alphaBlendOp = BLEND_OP_MAPPING[static_cast<size_t>(attachment_blend_descriptor.alpha_blend_op)];
                pipeline_color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

                break;
            }
        }
    }

    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info{};
    pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipeline_color_blend_state_create_info.flags = 0;
    pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
    pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_CLEAR;
    pipeline_color_blend_state_create_info.attachmentCount = static_cast<uint32_t>(pipeline_color_blend_attachment_states.size());
    pipeline_color_blend_state_create_info.pAttachments = pipeline_color_blend_attachment_states.data();
    pipeline_color_blend_state_create_info.blendConstants[0] = 0.f;
    pipeline_color_blend_state_create_info.blendConstants[1] = 0.f;
    pipeline_color_blend_state_create_info.blendConstants[2] = 0.f;
    pipeline_color_blend_state_create_info.blendConstants[3] = 0.f;

    //
    // Further validation of all uniforms.
    // Create descriptor set layout.
    //

    Vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings(create_context.graphics_pipeline_memory_resource);
    descriptor_set_layout_bindings.reserve(
        graphics_pipeline_descriptor.uniform_attachment_descriptor_count +
        graphics_pipeline_descriptor.uniform_buffer_descriptor_count +
        graphics_pipeline_descriptor.texture_descriptor_count +
        graphics_pipeline_descriptor.sampler_descriptor_count
    );

    KW_ASSERT(graphics_pipeline_data.attachment_indices.empty());
    graphics_pipeline_data.attachment_indices.resize(graphics_pipeline_descriptor.uniform_attachment_descriptor_count, SIZE_MAX);

    for (size_t i = 0; i < graphics_pipeline_descriptor.uniform_attachment_descriptor_count; i++) {
        const UniformAttachmentDescriptor& uniform_attachment_descriptor = graphics_pipeline_descriptor.uniform_attachment_descriptors[i];

        KW_ASSERT(create_context.attachment_mapping.count(uniform_attachment_descriptor.attachment_name) > 0);
        size_t attachment_index = create_context.attachment_mapping[uniform_attachment_descriptor.attachment_name];
        KW_ASSERT(attachment_index < m_attachment_descriptors.size());

        uint32_t attachment_count = static_cast<uint32_t>(m_attachment_descriptors[attachment_index].count);
        KW_ASSERT(attachment_count >= 1);

        ShaderVisibility uniform_attachment_visibility = uniform_attachment_descriptor.visibility;
        uint32_t uniform_attachment_binding = 0;
        bool is_uniform_attachment_found = false;

        {
            if (uniform_attachment_visibility == ShaderVisibility::VERTEX || uniform_attachment_visibility == ShaderVisibility::ALL) {
                const SpvReflectDescriptorBinding* descriptor_binding =
                    spvReflectGetDescriptorBindingByName(&vertex_shader_reflection, uniform_attachment_descriptor.variable_name, nullptr);

                if (descriptor_binding != nullptr) {
                    KW_ERROR(
                        descriptor_binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                        "Descriptor binding \"%s\" is expected to have \"Texture2D\" type in \"%s\".", uniform_attachment_descriptor.variable_name, graphics_pipeline_descriptor.vertex_shader_filename
                    );

                    KW_ERROR(
                        descriptor_binding->image.dim == SpvDim2D,
                        "Descriptor binding \"%s\" is expected to be a \"Texture2D\" in \"%s\".", uniform_attachment_descriptor.variable_name, graphics_pipeline_descriptor.vertex_shader_filename
                    );

                    uint32_t count = 1;

                    for (uint32_t j = 0; j < descriptor_binding->array.dims_count; j++) {
                        count *= descriptor_binding->array.dims[j];
                    }

                    KW_ERROR(
                        count == attachment_count,
                        "Descriptor binding \"%s\" has mismatching array size in \"%s\".", uniform_attachment_descriptor.variable_name, graphics_pipeline_descriptor.vertex_shader_filename
                    );

                    uniform_attachment_binding = descriptor_binding->binding;

                    is_uniform_attachment_found = true;
                } else {
                    Log::print("[WARNING] Uniform attachment \"%s\" is not found in \"%s\".", uniform_attachment_descriptor.variable_name, graphics_pipeline_descriptor.vertex_shader_filename);

                    if (uniform_attachment_visibility == ShaderVisibility::ALL) {
                        uniform_attachment_visibility = ShaderVisibility::FRAGMENT;
                    }
                }
            }
        }

        if (graphics_pipeline_descriptor.fragment_shader_filename != nullptr) {
            if (uniform_attachment_visibility == ShaderVisibility::FRAGMENT || uniform_attachment_visibility == ShaderVisibility::ALL) {
                const SpvReflectDescriptorBinding* descriptor_binding =
                    spvReflectGetDescriptorBindingByName(&fragment_shader_reflection, uniform_attachment_descriptor.variable_name, nullptr);

                if (descriptor_binding != nullptr) {
                    KW_ERROR(
                        descriptor_binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                        "Shader variable \"%s\" is expected to be a \"Texture2D\" in \"%s\".", uniform_attachment_descriptor.variable_name, graphics_pipeline_descriptor.fragment_shader_filename
                    );

                    KW_ERROR(
                        descriptor_binding->image.dim == SpvDim2D,
                        "Shader variable \"%s\" is expected to be a \"Texture2D\" in \"%s\".", uniform_attachment_descriptor.variable_name, graphics_pipeline_descriptor.fragment_shader_filename
                    );

                    uint32_t count = 1;

                    for (uint32_t j = 0; j < descriptor_binding->array.dims_count; j++) {
                        count *= descriptor_binding->array.dims[j];
                    }

                    KW_ERROR(
                        count == attachment_count,
                        "Descriptor binding \"%s\" has mismatching array size in \"%s\".", uniform_attachment_descriptor.variable_name, graphics_pipeline_descriptor.fragment_shader_filename
                    );

                    KW_ASSERT(uniform_attachment_visibility != ShaderVisibility::ALL || descriptor_binding->binding == uniform_attachment_binding);
                    uniform_attachment_binding = descriptor_binding->binding;

                    is_uniform_attachment_found = true;
                } else {
                    Log::print("[WARNING] Uniform attachment \"%s\" is not found in \"%s\".", uniform_attachment_descriptor.variable_name, graphics_pipeline_descriptor.fragment_shader_filename);

                    if (uniform_attachment_visibility == ShaderVisibility::ALL) {
                        uniform_attachment_visibility = ShaderVisibility::VERTEX;
                    }
                }
            }
        }

        if (is_uniform_attachment_found) {
            VkDescriptorSetLayoutBinding descriptor_set_layout_binding{};
            descriptor_set_layout_binding.binding = uniform_attachment_binding;
            descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptor_set_layout_binding.descriptorCount = attachment_count;
            descriptor_set_layout_binding.stageFlags = SHADER_VISILITY_MAPPING[static_cast<size_t>(uniform_attachment_visibility)];
            descriptor_set_layout_binding.pImmutableSamplers = nullptr;
            descriptor_set_layout_bindings.push_back(descriptor_set_layout_binding);

            KW_ASSERT(graphics_pipeline_data.attachment_indices[i] == SIZE_MAX);
            graphics_pipeline_data.attachment_indices[i] = attachment_index;
        }
    }

    for (size_t i = 0; i < graphics_pipeline_descriptor.uniform_buffer_descriptor_count; i++) {
        const UniformDescriptor& uniform_descriptor = graphics_pipeline_descriptor.uniform_buffer_descriptors[i];
        size_t uniform_count = std::max(uniform_descriptor.count, static_cast<size_t>(1));

        ShaderVisibility uniform_visibility = uniform_descriptor.visibility;
        uint32_t uniform_binding = 0;
        bool is_uniform_found = false;

        {
            if (uniform_visibility == ShaderVisibility::VERTEX || uniform_visibility == ShaderVisibility::ALL) {
                const SpvReflectDescriptorBinding* descriptor_binding =
                    spvReflectGetDescriptorBindingByName(&vertex_shader_reflection, uniform_descriptor.variable_name, nullptr);

                if (descriptor_binding != nullptr) {
                    KW_ERROR(
                        descriptor_binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        "Descriptor binding \"%s\" is expected to be an uniform buffer in \"%s\".", uniform_descriptor.variable_name, graphics_pipeline_descriptor.vertex_shader_filename
                    );

                    size_t count = 1;
                    for (uint32_t j = 0; j < descriptor_binding->array.dims_count; j++) {
                        count *= descriptor_binding->array.dims[j];
                    }

                    KW_ERROR(
                        uniform_count == count,
                        "Descriptor binding \"%s\" has mismatching array size in \"%s\".", uniform_descriptor.variable_name, graphics_pipeline_descriptor.vertex_shader_filename
                    );

                    uniform_binding = descriptor_binding->binding;

                    is_uniform_found = true;
                } else {
                    Log::print("[WARNING] Uniform buffer \"%s\" is not found in \"%s\".", uniform_descriptor.variable_name, graphics_pipeline_descriptor.vertex_shader_filename);

                    if (uniform_visibility == ShaderVisibility::ALL) {
                        uniform_visibility = ShaderVisibility::FRAGMENT;
                    }
                }
            }
        }

        if (graphics_pipeline_descriptor.fragment_shader_filename != nullptr) {
            if (uniform_visibility == ShaderVisibility::FRAGMENT || uniform_visibility == ShaderVisibility::ALL) {
                const SpvReflectDescriptorBinding* descriptor_binding =
                    spvReflectGetDescriptorBindingByName(&fragment_shader_reflection, uniform_descriptor.variable_name, nullptr);

                if (descriptor_binding != nullptr) {
                    KW_ERROR(
                        descriptor_binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        "Descriptor binding \"%s\" is expected to be an uniform buffer in \"%s\".", uniform_descriptor.variable_name, graphics_pipeline_descriptor.fragment_shader_filename
                    );

                    size_t count = 1;
                    for (uint32_t j = 0; j < descriptor_binding->array.dims_count; j++) {
                        count *= descriptor_binding->array.dims[j];
                    }

                    KW_ERROR(
                        uniform_count == count,
                        "Descriptor binding \"%s\" has mismatching array size in \"%s\".", uniform_descriptor.variable_name, graphics_pipeline_descriptor.fragment_shader_filename
                    );

                    KW_ASSERT(uniform_visibility != ShaderVisibility::ALL || descriptor_binding->binding == uniform_binding);
                    uniform_binding = descriptor_binding->binding;

                    is_uniform_found = true;
                } else {
                    Log::print("[WARNING] Uniform buffer \"%s\" is not found in \"%s\".", uniform_descriptor.variable_name, graphics_pipeline_descriptor.fragment_shader_filename);

                    if (uniform_visibility == ShaderVisibility::ALL) {
                        uniform_visibility = ShaderVisibility::VERTEX;
                    }
                }
            }
        }

        if (is_uniform_found) {
            VkDescriptorSetLayoutBinding descriptor_set_layout_binding{};
            descriptor_set_layout_binding.binding = uniform_binding;
            descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_set_layout_binding.descriptorCount = static_cast<uint32_t>(uniform_count);
            descriptor_set_layout_binding.stageFlags = SHADER_VISILITY_MAPPING[static_cast<size_t>(uniform_visibility)];
            descriptor_set_layout_binding.pImmutableSamplers = nullptr;
            descriptor_set_layout_bindings.push_back(descriptor_set_layout_binding);
        }
    }

    for (size_t i = 0; i < graphics_pipeline_descriptor.texture_descriptor_count; i++) {
        const UniformDescriptor& uniform_descriptor = graphics_pipeline_descriptor.texture_descriptors[i];
        size_t uniform_count = std::max(uniform_descriptor.count, static_cast<size_t>(1));

        ShaderVisibility uniform_visibility = uniform_descriptor.visibility;
        uint32_t uniform_binding = 0;
        bool is_uniform_found = false;

        {
            if (uniform_visibility == ShaderVisibility::VERTEX || uniform_visibility == ShaderVisibility::ALL) {
                const SpvReflectDescriptorBinding* descriptor_binding =
                    spvReflectGetDescriptorBindingByName(&vertex_shader_reflection, uniform_descriptor.variable_name, nullptr);

                if (descriptor_binding != nullptr) {
                    KW_ERROR(
                        descriptor_binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                        "Descriptor binding \"%s\" is expected to be a texture in \"%s\".", uniform_descriptor.variable_name, graphics_pipeline_descriptor.vertex_shader_filename
                    );

                    size_t count = 1;
                    for (uint32_t j = 0; j < descriptor_binding->array.dims_count; j++) {
                        count *= descriptor_binding->array.dims[j];
                    }

                    KW_ERROR(
                        uniform_count == count,
                        "Descriptor binding \"%s\" has mismatching array size in \"%s\".", uniform_descriptor.variable_name, graphics_pipeline_descriptor.vertex_shader_filename
                    );

                    uniform_binding = descriptor_binding->binding;

                    is_uniform_found = true;
                } else {
                    Log::print("[WARNING] Texture \"%s\" is not found in \"%s\".", uniform_descriptor.variable_name, graphics_pipeline_descriptor.vertex_shader_filename);

                    if (uniform_visibility == ShaderVisibility::ALL) {
                        uniform_visibility = ShaderVisibility::FRAGMENT;
                    }
                }
            }
        }

        if (graphics_pipeline_descriptor.fragment_shader_filename != nullptr) {
            if (uniform_visibility == ShaderVisibility::FRAGMENT || uniform_visibility == ShaderVisibility::ALL) {
                const SpvReflectDescriptorBinding* descriptor_binding =
                    spvReflectGetDescriptorBindingByName(&fragment_shader_reflection, uniform_descriptor.variable_name, nullptr);

                if (descriptor_binding != nullptr) {
                    KW_ERROR(
                        descriptor_binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                        "Descriptor binding \"%s\" is expected to be a texture in \"%s\".", uniform_descriptor.variable_name, graphics_pipeline_descriptor.fragment_shader_filename
                    );

                    size_t count = 1;
                    for (uint32_t j = 0; j < descriptor_binding->array.dims_count; j++) {
                        count *= descriptor_binding->array.dims[j];
                    }

                    KW_ERROR(
                        uniform_count == count,
                        "Descriptor binding \"%s\" has mismatching array size in \"%s\".", uniform_descriptor.variable_name, graphics_pipeline_descriptor.fragment_shader_filename
                    );

                    KW_ASSERT(uniform_visibility != ShaderVisibility::ALL || descriptor_binding->binding == uniform_binding);
                    uniform_binding = descriptor_binding->binding;

                    is_uniform_found = true;
                } else {
                    Log::print("[WARNING] Texture \"%s\" is not found in \"%s\".", uniform_descriptor.variable_name, graphics_pipeline_descriptor.fragment_shader_filename);

                    if (uniform_visibility == ShaderVisibility::ALL) {
                        uniform_visibility = ShaderVisibility::VERTEX;
                    }
                }
            }
        }

        if (is_uniform_found) {
            VkDescriptorSetLayoutBinding descriptor_set_layout_binding{};
            descriptor_set_layout_binding.binding = uniform_binding;
            descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptor_set_layout_binding.descriptorCount = static_cast<uint32_t>(uniform_count);
            descriptor_set_layout_binding.stageFlags = SHADER_VISILITY_MAPPING[static_cast<size_t>(uniform_visibility)];
            descriptor_set_layout_binding.pImmutableSamplers = nullptr;
            descriptor_set_layout_bindings.push_back(descriptor_set_layout_binding);
        }
    }

    KW_ASSERT(graphics_pipeline_data.samplers.empty());
    graphics_pipeline_data.samplers.resize(graphics_pipeline_descriptor.sampler_descriptor_count);

    for (size_t sampler_index = 0; sampler_index < graphics_pipeline_descriptor.sampler_descriptor_count; sampler_index++) {
        const SamplerDescriptor& sampler_descriptor = graphics_pipeline_descriptor.sampler_descriptors[sampler_index];

        VkSamplerCreateInfo sampler_create_info{};
        sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_create_info.flags = 0;
        sampler_create_info.magFilter = FILTER_MAPPING[static_cast<size_t>(sampler_descriptor.mag_filter)];
        sampler_create_info.minFilter = FILTER_MAPPING[static_cast<size_t>(sampler_descriptor.min_filter)];
        sampler_create_info.mipmapMode = MIP_FILTER_MAPPING[static_cast<size_t>(sampler_descriptor.mip_filter)];
        sampler_create_info.addressModeU = ADDRESS_MODE_MAPPING[static_cast<size_t>(sampler_descriptor.address_mode_u)];
        sampler_create_info.addressModeV = ADDRESS_MODE_MAPPING[static_cast<size_t>(sampler_descriptor.address_mode_v)];
        sampler_create_info.addressModeW = ADDRESS_MODE_MAPPING[static_cast<size_t>(sampler_descriptor.address_mode_w)];
        sampler_create_info.mipLodBias = std::min(sampler_descriptor.mip_lod_bias, limits.maxSamplerLodBias);
        sampler_create_info.anisotropyEnable = sampler_descriptor.anisotropy_enable;
        sampler_create_info.maxAnisotropy = std::min(sampler_descriptor.max_anisotropy, limits.maxSamplerAnisotropy);
        sampler_create_info.compareEnable = sampler_descriptor.compare_enable;
        sampler_create_info.compareOp = COMPARE_OP_MAPPING[static_cast<size_t>(sampler_descriptor.compare_op)];
        sampler_create_info.minLod = sampler_descriptor.min_lod;
        sampler_create_info.maxLod = sampler_descriptor.max_lod;
        sampler_create_info.borderColor = BORDER_COLOR_MAPPING[static_cast<size_t>(sampler_descriptor.border_color)];
        sampler_create_info.unnormalizedCoordinates = VK_FALSE;

        KW_ASSERT(graphics_pipeline_data.samplers[sampler_index] == VK_NULL_HANDLE);
        VK_ERROR(
            vkCreateSampler(m_render->device, &sampler_create_info, &m_render->allocation_callbacks, &graphics_pipeline_data.samplers[sampler_index]),
            "Failed to create sampler \"%s\"-\"%s\".", graphics_pipeline_descriptor.name, sampler_descriptor.variable_name
        );
        VK_NAME(
            m_render, graphics_pipeline_data.samplers[sampler_index],
            "Sampler \"%s\"-\"%s\"", graphics_pipeline_descriptor.name, sampler_descriptor.variable_name
        );

        ShaderVisibility sampler_visibility = sampler_descriptor.visibility;
        uint32_t sampler_binding = 0;
        bool is_sampler_found = false;

        {
            if (sampler_visibility == ShaderVisibility::VERTEX || sampler_visibility == ShaderVisibility::ALL) {
                const SpvReflectDescriptorBinding* descriptor_binding =
                    spvReflectGetDescriptorBindingByName(&vertex_shader_reflection, sampler_descriptor.variable_name, nullptr);

                if (descriptor_binding != nullptr) {
                    KW_ERROR(
                        descriptor_binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER,
                        "Descriptor binding \"%s\" is expected to be a sampler in \"%s\".", sampler_descriptor.variable_name, graphics_pipeline_descriptor.vertex_shader_filename
                    );

                    size_t count = 1;
                    for (uint32_t j = 0; j < descriptor_binding->array.dims_count; j++) {
                        count *= descriptor_binding->array.dims[j];
                    }

                    KW_ERROR(
                        count == 1,
                        "Descriptor binding \"%s\" has mismatching array size in \"%s\".", sampler_descriptor.variable_name, graphics_pipeline_descriptor.vertex_shader_filename
                    );

                    sampler_binding = descriptor_binding->binding;

                    is_sampler_found = true;
                } else {
                    Log::print("[WARNING] Sampler \"%s\" is not found in \"%s\".", sampler_descriptor.variable_name, graphics_pipeline_descriptor.vertex_shader_filename);

                    if (sampler_visibility == ShaderVisibility::ALL) {
                        sampler_visibility = ShaderVisibility::FRAGMENT;
                    }
                }
            }
        }

        if (graphics_pipeline_descriptor.fragment_shader_filename != nullptr) {
            if (sampler_visibility == ShaderVisibility::FRAGMENT || sampler_visibility == ShaderVisibility::ALL) {
                const SpvReflectDescriptorBinding* descriptor_binding =
                    spvReflectGetDescriptorBindingByName(&fragment_shader_reflection, sampler_descriptor.variable_name, nullptr);

                if (descriptor_binding != nullptr) {
                    KW_ERROR(
                        descriptor_binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER,
                        "Descriptor binding \"%s\" is expected to be a sampler in \"%s\".", sampler_descriptor.variable_name, graphics_pipeline_descriptor.fragment_shader_filename
                    );

                    size_t count = 1;
                    for (uint32_t j = 0; j < descriptor_binding->array.dims_count; j++) {
                        count *= descriptor_binding->array.dims[j];
                    }

                    KW_ERROR(
                        count == 1,
                        "Descriptor binding \"%s\" has mismatching array size in \"%s\".", sampler_descriptor.variable_name, graphics_pipeline_descriptor.fragment_shader_filename
                    );

                    KW_ASSERT(sampler_visibility != ShaderVisibility::ALL || descriptor_binding->binding == sampler_binding);
                    sampler_binding = descriptor_binding->binding;

                    is_sampler_found = true;
                } else {
                    Log::print("[WARNING] Sampler \"%s\" is not found in \"%s\".", sampler_descriptor.variable_name, graphics_pipeline_descriptor.fragment_shader_filename);

                    if (sampler_visibility == ShaderVisibility::ALL) {
                        sampler_visibility = ShaderVisibility::VERTEX;
                    }
                }
            }
        }

        if (is_sampler_found) {
            VkDescriptorSetLayoutBinding descriptor_set_layout_binding{};
            descriptor_set_layout_binding.binding = sampler_binding;
            descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            descriptor_set_layout_binding.descriptorCount = 1;
            descriptor_set_layout_binding.stageFlags = SHADER_VISILITY_MAPPING[static_cast<size_t>(sampler_visibility)];
            descriptor_set_layout_binding.pImmutableSamplers = &graphics_pipeline_data.samplers[sampler_index];
            descriptor_set_layout_bindings.push_back(descriptor_set_layout_binding);
        }
    }

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{};
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.flags = 0;
    descriptor_set_layout_create_info.bindingCount = static_cast<uint32_t>(descriptor_set_layout_bindings.size());
    descriptor_set_layout_create_info.pBindings = descriptor_set_layout_bindings.data();

    if (!descriptor_set_layout_bindings.empty()) {
        KW_ASSERT(graphics_pipeline_data.descriptor_set_layout == VK_NULL_HANDLE);
        VK_ERROR(
            vkCreateDescriptorSetLayout(m_render->device, &descriptor_set_layout_create_info, &m_render->allocation_callbacks, &graphics_pipeline_data.descriptor_set_layout),
            "Failed to create descriptor set layout \"%s\".", graphics_pipeline_descriptor.name
        );
        VK_NAME(
            m_render, graphics_pipeline_data.descriptor_set_layout,
            "Descriptor set layout \"%s\"", graphics_pipeline_descriptor.name
        );
    }

    ShaderVisibility push_constants_visibility = graphics_pipeline_descriptor.push_constants_visibility;
    bool is_push_constants_found = false;

    if (graphics_pipeline_descriptor.push_constants_name != nullptr) {
        KW_ERROR(
            graphics_pipeline_descriptor.push_constants_size <= limits.maxPushConstantsSize,
            "Push constants overflow. Max %u, got %zu.", limits.maxPushConstantsSize, graphics_pipeline_descriptor.push_constants_size
        );

        {
            if (push_constants_visibility == ShaderVisibility::VERTEX || push_constants_visibility == ShaderVisibility::ALL) {
                if (vertex_shader_reflection.push_constant_block_count == 1) {
                    KW_ERROR(
                        vertex_shader_reflection.push_constant_blocks[0].name != nullptr,
                        "Push constants have invalid name in \"%s\".", graphics_pipeline_descriptor.vertex_shader_filename
                    );

                    KW_ERROR(
                        strcmp(graphics_pipeline_descriptor.push_constants_name, vertex_shader_reflection.push_constant_blocks[0].name) == 0,
                        "Push constants name mismatch in \"%s\". Expected \"%s\", got \"%s\".", graphics_pipeline_descriptor.vertex_shader_filename,
                        graphics_pipeline_descriptor.push_constants_name, vertex_shader_reflection.push_constant_blocks[0].name
                    );

                    KW_ERROR(
                        graphics_pipeline_descriptor.push_constants_size == vertex_shader_reflection.push_constant_blocks[0].size,
                        "Mismatching push constants size in \"%s\". Expected %zu, got %u.", graphics_pipeline_descriptor.vertex_shader_filename,
                        graphics_pipeline_descriptor.push_constants_size, vertex_shader_reflection.push_constant_blocks[0].size
                    );

                    is_push_constants_found = true;
                } else {
                    Log::print("[WARNING] Push constants are not found in \"%s\".", graphics_pipeline_descriptor.vertex_shader_filename);

                    if (push_constants_visibility == ShaderVisibility::ALL) {
                        push_constants_visibility = ShaderVisibility::FRAGMENT;
                    }
                }
            }
        }

        if (graphics_pipeline_descriptor.fragment_shader_filename != nullptr) {
            if (push_constants_visibility == ShaderVisibility::FRAGMENT || push_constants_visibility == ShaderVisibility::ALL) {
                if (fragment_shader_reflection.push_constant_block_count == 1) {
                    KW_ERROR(
                        fragment_shader_reflection.push_constant_blocks[0].name != nullptr,
                        "Push constants have invalid name in \"%s\".", graphics_pipeline_descriptor.fragment_shader_filename
                    );

                    KW_ERROR(
                        strcmp(graphics_pipeline_descriptor.push_constants_name, fragment_shader_reflection.push_constant_blocks[0].name) == 0,
                        "Push constants name mismatch in \"%s\". Expected \"%s\", got \"%s\".", graphics_pipeline_descriptor.fragment_shader_filename,
                        graphics_pipeline_descriptor.push_constants_name, fragment_shader_reflection.push_constant_blocks[0].name
                    );

                    KW_ERROR(
                        graphics_pipeline_descriptor.push_constants_size == fragment_shader_reflection.push_constant_blocks[0].size,
                        "Mismatching push constants size in \"%s\". Expected %zu, got %u.", graphics_pipeline_descriptor.fragment_shader_filename,
                        graphics_pipeline_descriptor.push_constants_size, fragment_shader_reflection.push_constant_blocks[0].size
                    );

                    is_push_constants_found = true;
                } else {
                    Log::print("[WARNING] Push constants are not found in \"%s\".", graphics_pipeline_descriptor.fragment_shader_filename);

                    if (push_constants_visibility == ShaderVisibility::ALL) {
                        push_constants_visibility = ShaderVisibility::VERTEX;
                    }
                }
            }
        }
    } else {
        {
            KW_ERROR(
                vertex_shader_reflection.push_constant_block_count == 0,
                "Unexpected push constants in \"%s\".", graphics_pipeline_descriptor.vertex_shader_filename
            );
        }

        if (graphics_pipeline_descriptor.fragment_shader_filename != nullptr) {
            KW_ERROR(
                fragment_shader_reflection.push_constant_block_count == 0,
                "Unexpected push constants in \"%s\".", graphics_pipeline_descriptor.fragment_shader_filename
            );
        }
    }

    VkPushConstantRange push_constants_range{};
    push_constants_range.stageFlags = SHADER_VISILITY_MAPPING[static_cast<size_t>(push_constants_visibility)];
    push_constants_range.offset = 0;
    push_constants_range.size = graphics_pipeline_descriptor.push_constants_size;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.flags = 0;
    if (!descriptor_set_layout_bindings.empty()) {
        pipeline_layout_create_info.setLayoutCount = 1;
        pipeline_layout_create_info.pSetLayouts = &graphics_pipeline_data.descriptor_set_layout;
    }
    if (is_push_constants_found) {
        pipeline_layout_create_info.pushConstantRangeCount = 1;
        pipeline_layout_create_info.pPushConstantRanges = &push_constants_range;
    }

    KW_ASSERT(graphics_pipeline_data.pipeline_layout == VK_NULL_HANDLE);
    VK_ERROR(
        vkCreatePipelineLayout(m_render->device, &pipeline_layout_create_info, &m_render->allocation_callbacks, &graphics_pipeline_data.pipeline_layout),
        "Failed to create pipeline layout \"%s\".", graphics_pipeline_descriptor.name
    );
    VK_NAME(
        m_render, graphics_pipeline_data.pipeline_layout,
        "Pipeline layout \"%s\"", graphics_pipeline_descriptor.name
    );

    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info{};
    graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_create_info.flags = 0;
    graphics_pipeline_create_info.stageCount = stage_count;
    graphics_pipeline_create_info.pStages = pipeline_shader_stage_create_infos;
    graphics_pipeline_create_info.pVertexInputState = &pipeline_vertex_input_state_create_info;
    graphics_pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
    graphics_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
    graphics_pipeline_create_info.pRasterizationState = &pipeline_rasterization_state_create_info;
    graphics_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
    graphics_pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;
    graphics_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
    graphics_pipeline_create_info.pDynamicState = &pipeline_dynamic_state_create_info;
    graphics_pipeline_create_info.layout = graphics_pipeline_data.pipeline_layout;
    graphics_pipeline_create_info.renderPass = render_pass_data.render_pass;
    graphics_pipeline_create_info.subpass = 0;

    KW_ASSERT(graphics_pipeline_data.pipeline == VK_NULL_HANDLE);
    VK_ERROR(
        vkCreateGraphicsPipelines(m_render->device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, &m_render->allocation_callbacks, &graphics_pipeline_data.pipeline),
        "Failed to create graphics pipeline \"%s\".", graphics_pipeline_descriptor.name
    );
    VK_NAME(
        m_render, graphics_pipeline_data.pipeline,
        "Pipeline \"%s\"", graphics_pipeline_descriptor.name
    );
}

void FrameGraphVulkan::create_command_pools(CreateContext& create_context) {
    size_t thread_count = m_thread_pool->get_count();
    KW_ASSERT(thread_count > 0);

    size_t command_buffer_count = (m_render_pass_data.size() + thread_count - 1) / thread_count;

    for (size_t swapchain_image_index = 0; swapchain_image_index < SWAPCHAIN_IMAGE_COUNT; swapchain_image_index++) {
        KW_ASSERT(m_command_pool_data[swapchain_image_index].empty());
        m_command_pool_data[swapchain_image_index].resize(thread_count, CommandPoolData(m_render->persistent_memory_resource));

        for (size_t thread_index = 0; thread_index < thread_count; thread_index++) {
            CommandPoolData& command_pool_data = m_command_pool_data[swapchain_image_index][thread_index];

            VkCommandPoolCreateInfo command_pool_create_info{};
            command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            command_pool_create_info.queueFamilyIndex = m_render->graphics_queue_family_index;

            KW_ASSERT(command_pool_data.command_pool == VK_NULL_HANDLE);
            VK_ERROR(
                vkCreateCommandPool(m_render->device, &command_pool_create_info, &m_render->allocation_callbacks, &command_pool_data.command_pool),
                "Failed to create a command pool."
            );
            VK_NAME(m_render, command_pool_data.command_pool, "Frame command pool %zu-%zu", swapchain_image_index, thread_index);

            KW_ASSERT(command_pool_data.command_buffers.empty());
            command_pool_data.command_buffers.resize(command_buffer_count);

            VkCommandBufferAllocateInfo command_buffer_allocate_info{};
            command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            command_buffer_allocate_info.commandPool = command_pool_data.command_pool;
            command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            command_buffer_allocate_info.commandBufferCount = static_cast<uint32_t>(command_pool_data.command_buffers.size());

            VK_ERROR(
                vkAllocateCommandBuffers(m_render->device, &command_buffer_allocate_info, command_pool_data.command_buffers.data()),
                "Failed to allocate command buffers."
            );

            for (size_t command_buffer_index = 0; command_buffer_index < command_pool_data.command_buffers.size(); command_buffer_index++) {
                VkCommandBuffer command_buffer = command_pool_data.command_buffers[command_buffer_index];
                VK_NAME(m_render, command_buffer, "Frame command buffer %zu-%zu-%zu", swapchain_image_index, thread_index, command_buffer_index);
            }
        }
    }
}

void FrameGraphVulkan::create_synchronization(CreateContext& create_context) {
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t swapchain_image_index = 0; swapchain_image_index < SWAPCHAIN_IMAGE_COUNT; swapchain_image_index++) {
        KW_ASSERT(m_image_acquired_binary_semaphores[swapchain_image_index] == VK_NULL_HANDLE);
        VK_ERROR(
            vkCreateSemaphore(m_render->device, &semaphore_create_info, &m_render->allocation_callbacks, &m_image_acquired_binary_semaphores[swapchain_image_index]),
            "Failed to create an image acquire binary semaphore."
        );
        VK_NAME(
            m_render, m_image_acquired_binary_semaphores[swapchain_image_index],
            "Frame acquire semaphore %zu", swapchain_image_index
        );

        KW_ASSERT(m_render_finished_binary_semaphores[swapchain_image_index] == VK_NULL_HANDLE);
        VK_ERROR(
            vkCreateSemaphore(m_render->device, &semaphore_create_info, &m_render->allocation_callbacks, &m_render_finished_binary_semaphores[swapchain_image_index]),
            "Failed to create a render finished binary semaphore."
        );
        VK_NAME(
            m_render, m_render_finished_binary_semaphores[swapchain_image_index],
            "Frame finished semaphore %zu", swapchain_image_index
        );

        m_render_finished_timeline_semaphores[swapchain_image_index] = std::make_shared<TimelineSemaphore>(m_render);
        VK_NAME(
            m_render, m_render_finished_timeline_semaphores[swapchain_image_index]->semaphore,
            "Frame finished timeline semaphore %zu", swapchain_image_index
        );

        // Render must wait for this frame to finish before destroying a resource that could be used in this frame.
        m_render->add_resource_dependency(m_render_finished_timeline_semaphores[swapchain_image_index]);
    }
}

void FrameGraphVulkan::create_temporary_resources() {
    RecreateContext recreate_context{ 0, 0 };

    if (create_swapchain(recreate_context)) {
        create_swapchain_images(recreate_context);
        create_swapchain_image_views(recreate_context);

        create_attachment_images(recreate_context);
        allocate_attachment_memory(recreate_context);
        create_attachment_image_views(recreate_context);

        create_framebuffers(recreate_context);

        m_frame_index = 0;
    }
}

void FrameGraphVulkan::destroy_temporary_resources() {
    for (RenderPassData& render_pass_data : m_render_pass_data) {
        for (VkFramebuffer& framebuffer: render_pass_data.framebuffers) {
            vkDestroyFramebuffer(m_render->device, framebuffer, &m_render->allocation_callbacks);
            framebuffer = VK_NULL_HANDLE;
        }
    }

    for (AllocationData& allocation_data : m_allocation_data) {
        m_render->deallocate_device_texture_memory(allocation_data.data_index, allocation_data.data_offset);
    }

    m_allocation_data.clear();

    for (AttachmentData& attachment_data : m_attachment_data) {
        vkDestroyImageView(m_render->device, attachment_data.image_view, &m_render->allocation_callbacks);
        attachment_data.image_view = VK_NULL_HANDLE;

        vkDestroyImage(m_render->device, attachment_data.image, &m_render->allocation_callbacks);
        attachment_data.image = VK_NULL_HANDLE;
    }

    for (VkImageView& image_view : m_swapchain_image_views) {
        vkDestroyImageView(m_render->device, image_view, &m_render->allocation_callbacks);
        image_view = VK_NULL_HANDLE;
    }

    for (VkImage& image : m_swapchain_images) {
        image = VK_NULL_HANDLE;
    }

    // Spec states that `vkDestroySwapchainKHR` must silently ignore `m_swapchain == VK_NULL_HANDLE`, but on my hardware it crashes.
    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_render->device, m_swapchain, &m_render->allocation_callbacks);
        m_swapchain = VK_NULL_HANDLE;
    }
}

bool FrameGraphVulkan::create_swapchain(RecreateContext& recreate_context) {
    VkSurfaceCapabilitiesKHR capabilities;
    VK_ERROR(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_render->physical_device, m_surface, &capabilities),
        "Failed to query surface capabilities."
    );
    KW_ERROR(
        capabilities.minImageCount <= SWAPCHAIN_IMAGE_COUNT && (capabilities.maxImageCount >= SWAPCHAIN_IMAGE_COUNT || capabilities.maxImageCount == 0),
        "Incompatible surface (min %u, max %u).", capabilities.minImageCount, capabilities.maxImageCount
    );

#ifdef KW_FRAME_GRAPH_LOG
    Log::print("[Frame Graph] Swapchain capabilities:");
    Log::print("[Frame Graph] * Min image count: %u", capabilities.minImageCount);
    Log::print("[Frame Graph]   Max image count: %u", capabilities.maxImageCount);
    Log::print("[Frame Graph]   Min image width: %u", capabilities.minImageExtent.width);
    Log::print("[Frame Graph]   Max image width: %u", capabilities.maxImageExtent.width);
    Log::print("[Frame Graph]   Min image height: %u", capabilities.minImageExtent.height);
    Log::print("[Frame Graph]   Max image height: %u", capabilities.maxImageExtent.height);
#endif

    VkExtent2D extent;
    if (capabilities.currentExtent.width != UINT32_MAX) {
        extent = capabilities.currentExtent;
    } else {
        extent = VkExtent2D{
            std::clamp(m_window->get_render_width(), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp(m_window->get_render_height(), capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
        };
    }

#ifdef KW_FRAME_GRAPH_LOG
    Log::print("[Frame Graph] Swapchain width: %u", extent.width);
    Log::print("[Frame Graph] Swapchain height: %u", extent.height);
#endif

    recreate_context.swapchain_width = extent.width;
    recreate_context.swapchain_height = extent.height;

    if (extent.width == 0 || extent.height == 0) {
        // Window is minimized.
        return false;
    }

    VkSwapchainCreateInfoKHR swapchain_create_info{};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = m_surface;
    swapchain_create_info.minImageCount = static_cast<uint32_t>(SWAPCHAIN_IMAGE_COUNT);
    swapchain_create_info.imageFormat = m_surface_format;
    swapchain_create_info.imageColorSpace = m_color_space;
    swapchain_create_info.imageExtent = extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 0;
    swapchain_create_info.pQueueFamilyIndices = nullptr;
    swapchain_create_info.preTransform = capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = m_present_mode;
    swapchain_create_info.clipped = VK_TRUE;

    KW_ASSERT(m_swapchain == VK_NULL_HANDLE);
    VK_ERROR(
        vkCreateSwapchainKHR(m_render->device, &swapchain_create_info, &m_render->allocation_callbacks, &m_swapchain),
        "Failed to create a swapchain."
    );
    VK_NAME(m_render, m_swapchain, "Swapchain");

    return true;
}

void FrameGraphVulkan::create_swapchain_images(RecreateContext& recreate_context) {
    uint32_t image_count;
    VK_ERROR(
        vkGetSwapchainImagesKHR(m_render->device, m_swapchain, &image_count, nullptr),
        "Failed to get swapchain image count."
    );
    KW_ERROR(image_count == SWAPCHAIN_IMAGE_COUNT, "Invalid swapchain image count %u.", image_count);

    for (size_t swapchain_image_index = 0; swapchain_image_index < SWAPCHAIN_IMAGE_COUNT; swapchain_image_index++) {
        KW_ASSERT(m_swapchain_images[swapchain_image_index] == VK_NULL_HANDLE);
    }

    VK_ERROR(
        vkGetSwapchainImagesKHR(m_render->device, m_swapchain, &image_count, m_swapchain_images),
        "Failed to get swapchain images."
    );

    for (size_t swapchain_image_index = 0; swapchain_image_index < SWAPCHAIN_IMAGE_COUNT; swapchain_image_index++) {
        VK_NAME(m_render, m_swapchain_images[swapchain_image_index], "Swapchain image %zu", swapchain_image_index);
    }
}

void FrameGraphVulkan::create_swapchain_image_views(RecreateContext& recreate_context) {
    for (size_t swapchain_image_index = 0; swapchain_image_index < SWAPCHAIN_IMAGE_COUNT; swapchain_image_index++) {
        VkImageViewCreateInfo image_view_create_info{};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.image = m_swapchain_images[swapchain_image_index];
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format = m_surface_format;
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;

        KW_ASSERT(m_swapchain_image_views[swapchain_image_index] == VK_NULL_HANDLE);
        VK_ERROR(
            vkCreateImageView(m_render->device, &image_view_create_info, &m_render->allocation_callbacks, &m_swapchain_image_views[swapchain_image_index]),
            "Failed to create image view %zu.", swapchain_image_index
        );
        VK_NAME(
            m_render, m_swapchain_image_views[swapchain_image_index],
            "Swapchain image view %zu", swapchain_image_index
        );
    }
}

void FrameGraphVulkan::create_attachment_images(RecreateContext& recreate_context) {
    const VkPhysicalDeviceProperties& properties = m_render->physical_device_properties;
    const VkPhysicalDeviceLimits& limits = properties.limits;

    // Ignore the first attachment, because it's a swapchain attachment.
    for (size_t i = 1; i < m_attachment_descriptors.size(); i++) {
        const AttachmentDescriptor& attachment_descriptor = m_attachment_descriptors[i];
        AttachmentData& attachment_data = m_attachment_data[i];

        uint32_t width;
        uint32_t height;
        if (attachment_descriptor.size_class == SizeClass::RELATIVE) {
            width = static_cast<uint32_t>(attachment_descriptor.width * recreate_context.swapchain_width);
            height = static_cast<uint32_t>(attachment_descriptor.height * recreate_context.swapchain_height);
        } else {
            width = static_cast<uint32_t>(attachment_descriptor.width);
            height = static_cast<uint32_t>(attachment_descriptor.height);
        }

        KW_ERROR(width <= limits.maxImageDimension2D, "Attachment image is too big.");
        KW_ERROR(height <= limits.maxImageDimension2D, "Attachment image is too big.");

        VkImageCreateInfo image_create_info{};
        image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.flags = 0;
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.format = TextureFormatUtils::convert_format_vulkan(attachment_descriptor.format);
        image_create_info.extent.width = width;
        image_create_info.extent.height = height;
        image_create_info.extent.depth = 1;
        image_create_info.mipLevels = 1;
        image_create_info.arrayLayers = 1;
        image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_create_info.usage = attachment_data.usage_mask;
        image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_create_info.queueFamilyIndexCount = 0;
        image_create_info.pQueueFamilyIndices = nullptr;
        image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        KW_ASSERT(attachment_data.image == VK_NULL_HANDLE);
        VK_ERROR(
            vkCreateImage(m_render->device, &image_create_info, &m_render->allocation_callbacks, &attachment_data.image),
            "Failed to create attachment image \"%s\".", attachment_descriptor.name
        );
        VK_NAME(m_render, attachment_data.image, "Attachment \"%s\"", attachment_descriptor.name);
    }
}

void FrameGraphVulkan::allocate_attachment_memory(RecreateContext& recreate_context) {
    //
    // Query attachment memory requirements.
    //

    Vector<VkMemoryRequirements> memory_requirements(m_attachment_data.size(), m_render->transient_memory_resource);

    // Ignore the first attachment, because it's a swapchain attachment.
    for (size_t i = 1; i < memory_requirements.size(); i++) {
        vkGetImageMemoryRequirements(m_render->device, m_attachment_data[i].image, &memory_requirements[i]);
    }

    //
    // Compute sorted attachment mapping.
    //

    Vector<size_t> sorted_attachment_indices(memory_requirements.size(), m_render->transient_memory_resource);

    std::iota(sorted_attachment_indices.begin(), sorted_attachment_indices.end(), 0);

    std::sort(sorted_attachment_indices.begin(), sorted_attachment_indices.end(), [&](size_t a, size_t b) {
        return memory_requirements[a].size > memory_requirements[b].size;
    });

    //
    // Allocate memory for attachments or alias other attachments.
    //

    struct AliasData {
        size_t attachment_index;

        VkDeviceMemory memory;

        size_t alias_index;
        size_t alias_offset;
        size_t alias_size_left;
    };

    KW_ASSERT(m_allocation_data.empty());
    m_allocation_data.reserve(m_attachment_data.size());

    Vector<AliasData> alias_data(m_render->transient_memory_resource);
    alias_data.reserve(sorted_attachment_indices.size());

    for (size_t i = 0; i < sorted_attachment_indices.size(); i++) {
        // Ignore the swapchain attachment.
        size_t attachment_index = sorted_attachment_indices[i];
        if (attachment_index != 0) {
            VkDeviceSize size = next_pow2(memory_requirements[attachment_index].size);
            VkDeviceSize alignment = memory_requirements[attachment_index].alignment;

            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkDeviceSize offset = 0;

            for (size_t j = 0; j < alias_data.size(); j++) {
                VkDeviceSize alignemnt_offset = align_up(alias_data[j].alias_offset, alignment) - alias_data[j].alias_offset;
                if (alias_data[j].alias_size_left >= size + alignemnt_offset) {
                    bool overlap = false;

                    size_t alias_index = j;
                    while (!overlap && alias_index != SIZE_MAX) {
                        size_t another_attachment_index = alias_data[alias_index].attachment_index;

                        size_t a = m_attachment_data[attachment_index].min_parallel_block_index;
                        size_t b = m_attachment_data[attachment_index].max_parallel_block_index;
                        size_t c = m_attachment_data[another_attachment_index].min_parallel_block_index;
                        size_t d = m_attachment_data[another_attachment_index].max_parallel_block_index;

                        if (a <= b) {
                            // Attachment range is non-looped.
                            if (c <= d) {
                                // Another attachment range is non-looped.
                                if (a <= d && b >= c) {
                                    overlap = true;
                                }
                            } else {
                                // Another attachment range is looped.
                                if (a <= d || b >= c) {
                                    overlap = true;
                                }
                            }
                        } else {
                            // Attachment range is looped.
                            if (c <= d) {
                                // Another attachment range is non-looped.
                                if (c <= b || d >= a) {
                                    overlap = true;
                                }
                            } else {
                                // Another attachment range is looped. Both looped ranges always overlap.
                                overlap = true;
                            }
                        }

                        alias_index = alias_data[alias_index].alias_index;
                    }

                    if (!overlap) {
                        memory = alias_data[j].memory;
                        offset = alias_data[j].alias_offset + alignemnt_offset;

                        alias_data[j].alias_size_left -= size + alignemnt_offset;
                        alias_data[j].alias_offset += size + alignemnt_offset;

                        alias_data.push_back(AliasData{ attachment_index, memory, alias_data[j].attachment_index, offset, size });

                        break;
                    }
                }
            }

            if (memory == VK_NULL_HANDLE) {
                RenderVulkan::DeviceAllocation device_allocation = m_render->allocate_device_texture_memory(size, alignment);
                KW_ASSERT(device_allocation.memory != VK_NULL_HANDLE);

                m_allocation_data.push_back(AllocationData{ device_allocation.data_index, device_allocation.data_offset });

                memory = device_allocation.memory;
                offset = device_allocation.data_offset;

                alias_data.push_back(AliasData{ attachment_index, memory, SIZE_MAX, offset, size });
            }

            AttachmentData& attachment_data = m_attachment_data[attachment_index];
            KW_ASSERT(attachment_data.image != VK_NULL_HANDLE);

            const AttachmentDescriptor& attachment_descriptor = m_attachment_descriptors[attachment_index];
            KW_ASSERT(attachment_descriptor.name != nullptr);

            VK_ERROR(
                vkBindImageMemory(m_render->device, attachment_data.image, memory, offset),
                "Failed to bind attachment image \"%s\" to memory.", attachment_descriptor.name
            );
        }
    }
}

void FrameGraphVulkan::create_attachment_image_views(RecreateContext& recreate_context) {
    // Ignore the first attachment, because it's a swapchain attachment.
    for (size_t i = 1; i < m_attachment_descriptors.size(); i++) {
        const AttachmentDescriptor& attachment_descriptor = m_attachment_descriptors[i];
        KW_ASSERT(attachment_descriptor.name != nullptr);

        AttachmentData& attachment_data = m_attachment_data[i];
        KW_ASSERT(attachment_data.image != VK_NULL_HANDLE);

        VkImageAspectFlags aspect_mask;
        if (attachment_descriptor.format == TextureFormat::D24_UNORM_S8_UINT || attachment_descriptor.format == TextureFormat::D32_FLOAT_S8X24_UINT) {
            aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        } else if (attachment_descriptor.format == TextureFormat::D16_UNORM || attachment_descriptor.format == TextureFormat::D32_FLOAT) {
            aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
        } else {
            aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        VkImageSubresourceRange image_subresource_range{};
        image_subresource_range.aspectMask = aspect_mask;
        image_subresource_range.baseMipLevel = 0;
        image_subresource_range.levelCount = 1;
        image_subresource_range.baseArrayLayer = 0;
        image_subresource_range.layerCount = 1;

        VkImageViewCreateInfo image_view_create_info{};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.flags = 0;
        image_view_create_info.image = attachment_data.image;
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format = TextureFormatUtils::convert_format_vulkan(attachment_descriptor.format);
        image_view_create_info.subresourceRange = image_subresource_range;

        KW_ASSERT(attachment_data.image_view == VK_NULL_HANDLE);
        VK_ERROR(
            vkCreateImageView(m_render->device, &image_view_create_info, &m_render->allocation_callbacks, &attachment_data.image_view),
            "Failed to create attachment image view \"%s\".", attachment_descriptor.name
        );
        VK_NAME(m_render, attachment_data.image_view, "Attachment view \"%s\"", attachment_descriptor.name);
    }
}

void FrameGraphVulkan::create_framebuffers(RecreateContext& recreate_context) {
    for (size_t render_pass_index = 0; render_pass_index < m_render_pass_data.size(); render_pass_index++) {
        RenderPassData& render_pass_data = m_render_pass_data[render_pass_index];
        KW_ASSERT(render_pass_data.render_pass != VK_NULL_HANDLE);
        KW_ASSERT(!render_pass_data.attachment_indices.empty());

        //
        // Query framebuffer size from any attachment, because they all must have equal size.
        //

        {
            size_t attachment_index = render_pass_data.attachment_indices[0];
            KW_ASSERT(attachment_index < m_attachment_descriptors.size());

            const AttachmentDescriptor& attachment_descriptor = m_attachment_descriptors[attachment_index];
            if (attachment_descriptor.size_class == SizeClass::RELATIVE) {
                render_pass_data.framebuffer_width = static_cast<uint32_t>(attachment_descriptor.width * recreate_context.swapchain_width);
                render_pass_data.framebuffer_height = static_cast<uint32_t>(attachment_descriptor.height * recreate_context.swapchain_height);
            } else {
                render_pass_data.framebuffer_width = static_cast<uint32_t>(attachment_descriptor.width);
                render_pass_data.framebuffer_height = static_cast<uint32_t>(attachment_descriptor.height);
            }
        }

        //
        // Compute framebuffer count.
        //

        size_t framebuffer_count = 1;

        for (size_t i = 0; i < render_pass_data.attachment_indices.size(); i++) {
            size_t attachment_index = render_pass_data.attachment_indices[i];
            KW_ASSERT(attachment_index < m_attachment_descriptors.size());

            if (attachment_index == 0) {
                // Render passes with swapchain attachment need different framebuffers every frame.
                framebuffer_count = SWAPCHAIN_IMAGE_COUNT;
            }
        }

        //
        // Create framebuffers.
        //

        for (size_t framebuffer_index = 0; framebuffer_index < framebuffer_count; framebuffer_index++) {
            Vector<VkImageView> attachments(render_pass_data.attachment_indices.size(), m_render->transient_memory_resource);

            for (size_t i = 0; i < render_pass_data.attachment_indices.size(); i++) {
                size_t attachment_index = render_pass_data.attachment_indices[i];
                KW_ASSERT(attachment_index < m_attachment_descriptors.size());

                if (attachment_index == 0) {
                    attachments[i] = m_swapchain_image_views[framebuffer_index];
                } else {
                    attachments[i] = m_attachment_data[attachment_index].image_view;
                }
            }

            VkFramebufferCreateInfo framebuffer_create_info{};
            framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_create_info.renderPass = render_pass_data.render_pass;
            framebuffer_create_info.pAttachments = attachments.data();
            framebuffer_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebuffer_create_info.width = render_pass_data.framebuffer_width;
            framebuffer_create_info.height = render_pass_data.framebuffer_height;
            framebuffer_create_info.layers = 1;

            KW_ASSERT(render_pass_data.framebuffers[framebuffer_index] == VK_NULL_HANDLE);
            VK_ERROR(
                vkCreateFramebuffer(m_render->device, &framebuffer_create_info, &m_render->allocation_callbacks, &render_pass_data.framebuffers[framebuffer_index]),
                "Failed to create framebuffer %zu.", render_pass_index
            );
            VK_NAME(m_render, render_pass_data.framebuffers[framebuffer_index], "Framebuffer %zu", render_pass_index);
        }
    }
}

} // namespace kw
