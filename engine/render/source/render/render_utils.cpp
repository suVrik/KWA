#include "render/render_utils.h"

#include <core/error.h>
#include <core/filesystem_utils.h>

#include <map>

namespace kw::RenderUtils {

namespace render_utils_details {

class Parser {
public:
    Parser(MemoryResource& memory_resource, const String& relative_path)
        : m_data(FilesystemUtils::read_file(memory_resource, relative_path))
        , m_end(m_data.data() + m_data.size())
        , m_position(m_data.data()) {
    }

    const uint8_t* read(size_t size) {
        if (m_position + size <= m_end) {
            const uint8_t* result = m_position;
            m_position += size;
            return result;
        }
        return nullptr;
    }

    template <typename T>
    const T* read() {
        return reinterpret_cast<const T*>(read(sizeof(T)));
    }

private:
    Vector<uint8_t> m_data;
    const uint8_t* m_end;
    const uint8_t* m_position;
};

constexpr uint32_t GEO_SIGNATURE = ' OEG';
constexpr uint32_t DDS_SIGNATURE = ' SDD';

constexpr uint32_t DDPF_ALPHA     = 0x00002;
constexpr uint32_t DDPF_FOURCC    = 0x00004;
constexpr uint32_t DDPF_RGB       = 0x00040;
constexpr uint32_t DDPF_YUV       = 0x00200;
constexpr uint32_t DDPF_LUMINANCE = 0x20000;
constexpr uint32_t DDPF_BUMPDUDV  = 0x80000;

struct DDS_PIXELFORMAT {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwFourCC;
    uint32_t dwRGBBitCount;
    uint32_t dwRBitMask;
    uint32_t dwGBitMask;
    uint32_t dwBBitMask;
    uint32_t dwABitMask;
};

constexpr uint32_t DDSD_CAPS        = 0x000001;
constexpr uint32_t DDSD_HEIGHT      = 0x000002;
constexpr uint32_t DDSD_WIDTH       = 0x000004;
constexpr uint32_t DDSD_PIXELFORMAT = 0x001000;
constexpr uint32_t DDSD_MIPMAPCOUNT = 0x020000;
constexpr uint32_t DDSD_DEPTH       = 0x800000;

constexpr uint32_t DDSD_REQUIRED_FLAGS = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;

constexpr uint32_t DDSCAPS_TEXTURE = 0x001000;
constexpr uint32_t DDSCAPS_MIPMAP  = 0x400000;

constexpr uint32_t DDSCAPS2_CUBEMAP           = 0x000200;
constexpr uint32_t DDSCAPS2_CUBEMAP_POSITIVEX = 0x000400;
constexpr uint32_t DDSCAPS2_CUBEMAP_NEGATIVEX = 0x000800;
constexpr uint32_t DDSCAPS2_CUBEMAP_POSITIVEY = 0x001000;
constexpr uint32_t DDSCAPS2_CUBEMAP_NEGATIVEY = 0x002000;
constexpr uint32_t DDSCAPS2_CUBEMAP_POSITIVEZ = 0x004000;
constexpr uint32_t DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x008000;
constexpr uint32_t DDSCAPS2_VOLUME            = 0x200000;

constexpr uint32_t DDSCAPS2_CUBEMAP_ALLFACES = DDSCAPS2_CUBEMAP_POSITIVEX | DDSCAPS2_CUBEMAP_NEGATIVEX |
                                               DDSCAPS2_CUBEMAP_POSITIVEY | DDSCAPS2_CUBEMAP_NEGATIVEY |
                                               DDSCAPS2_CUBEMAP_POSITIVEZ | DDSCAPS2_CUBEMAP_NEGATIVEZ;

struct DDS_HEADER {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwHeight;
    uint32_t dwWidth;
    uint32_t dwPitchOrLinearSize;
    uint32_t dwDepth;
    uint32_t dwMipMapCount;
    uint32_t dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32_t dwCaps;
    uint32_t dwCaps2;
    uint32_t dwCaps3;
    uint32_t dwCaps4;
    uint32_t dwReserved2;
};

constexpr uint32_t DDPF_FOURCC_DX10 = '01XD';

enum DXGI_FORMAT {
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_R32G32B32A32_TYPELESS,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    DXGI_FORMAT_R32G32B32A32_UINT,
    DXGI_FORMAT_R32G32B32A32_SINT,
    DXGI_FORMAT_R32G32B32_TYPELESS,
    DXGI_FORMAT_R32G32B32_FLOAT,
    DXGI_FORMAT_R32G32B32_UINT,
    DXGI_FORMAT_R32G32B32_SINT,
    DXGI_FORMAT_R16G16B16A16_TYPELESS,
    DXGI_FORMAT_R16G16B16A16_FLOAT,
    DXGI_FORMAT_R16G16B16A16_UNORM,
    DXGI_FORMAT_R16G16B16A16_UINT,
    DXGI_FORMAT_R16G16B16A16_SNORM,
    DXGI_FORMAT_R16G16B16A16_SINT,
    DXGI_FORMAT_R32G32_TYPELESS,
    DXGI_FORMAT_R32G32_FLOAT,
    DXGI_FORMAT_R32G32_UINT,
    DXGI_FORMAT_R32G32_SINT,
    DXGI_FORMAT_R32G8X24_TYPELESS,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
    DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
    DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,
    DXGI_FORMAT_R10G10B10A2_TYPELESS,
    DXGI_FORMAT_R10G10B10A2_UNORM,
    DXGI_FORMAT_R10G10B10A2_UINT,
    DXGI_FORMAT_R11G11B10_FLOAT,
    DXGI_FORMAT_R8G8B8A8_TYPELESS,
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
    DXGI_FORMAT_R8G8B8A8_UINT,
    DXGI_FORMAT_R8G8B8A8_SNORM,
    DXGI_FORMAT_R8G8B8A8_SINT,
    DXGI_FORMAT_R16G16_TYPELESS,
    DXGI_FORMAT_R16G16_FLOAT,
    DXGI_FORMAT_R16G16_UNORM,
    DXGI_FORMAT_R16G16_UINT,
    DXGI_FORMAT_R16G16_SNORM,
    DXGI_FORMAT_R16G16_SINT,
    DXGI_FORMAT_R32_TYPELESS,
    DXGI_FORMAT_D32_FLOAT,
    DXGI_FORMAT_R32_FLOAT,
    DXGI_FORMAT_R32_UINT,
    DXGI_FORMAT_R32_SINT,
    DXGI_FORMAT_R24G8_TYPELESS,
    DXGI_FORMAT_D24_UNORM_S8_UINT,
    DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
    DXGI_FORMAT_X24_TYPELESS_G8_UINT,
    DXGI_FORMAT_R8G8_TYPELESS,
    DXGI_FORMAT_R8G8_UNORM,
    DXGI_FORMAT_R8G8_UINT,
    DXGI_FORMAT_R8G8_SNORM,
    DXGI_FORMAT_R8G8_SINT,
    DXGI_FORMAT_R16_TYPELESS,
    DXGI_FORMAT_R16_FLOAT,
    DXGI_FORMAT_D16_UNORM,
    DXGI_FORMAT_R16_UNORM,
    DXGI_FORMAT_R16_UINT,
    DXGI_FORMAT_R16_SNORM,
    DXGI_FORMAT_R16_SINT,
    DXGI_FORMAT_R8_TYPELESS,
    DXGI_FORMAT_R8_UNORM,
    DXGI_FORMAT_R8_UINT,
    DXGI_FORMAT_R8_SNORM,
    DXGI_FORMAT_R8_SINT,
    DXGI_FORMAT_A8_UNORM,
    DXGI_FORMAT_R1_UNORM,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
    DXGI_FORMAT_R8G8_B8G8_UNORM,
    DXGI_FORMAT_G8R8_G8B8_UNORM,
    DXGI_FORMAT_BC1_TYPELESS,
    DXGI_FORMAT_BC1_UNORM,
    DXGI_FORMAT_BC1_UNORM_SRGB,
    DXGI_FORMAT_BC2_TYPELESS,
    DXGI_FORMAT_BC2_UNORM,
    DXGI_FORMAT_BC2_UNORM_SRGB,
    DXGI_FORMAT_BC3_TYPELESS,
    DXGI_FORMAT_BC3_UNORM,
    DXGI_FORMAT_BC3_UNORM_SRGB,
    DXGI_FORMAT_BC4_TYPELESS,
    DXGI_FORMAT_BC4_UNORM,
    DXGI_FORMAT_BC4_SNORM,
    DXGI_FORMAT_BC5_TYPELESS,
    DXGI_FORMAT_BC5_UNORM,
    DXGI_FORMAT_BC5_SNORM,
    DXGI_FORMAT_B5G6R5_UNORM,
    DXGI_FORMAT_B5G5R5A1_UNORM,
    DXGI_FORMAT_B8G8R8A8_UNORM,
    DXGI_FORMAT_B8G8R8X8_UNORM,
    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
    DXGI_FORMAT_B8G8R8A8_TYPELESS,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
    DXGI_FORMAT_B8G8R8X8_TYPELESS,
    DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
    DXGI_FORMAT_BC6H_TYPELESS,
    DXGI_FORMAT_BC6H_UF16,
    DXGI_FORMAT_BC6H_SF16,
    DXGI_FORMAT_BC7_TYPELESS,
    DXGI_FORMAT_BC7_UNORM,
    DXGI_FORMAT_BC7_UNORM_SRGB,
    DXGI_FORMAT_AYUV,
    DXGI_FORMAT_Y410,
    DXGI_FORMAT_Y416,
    DXGI_FORMAT_NV12,
    DXGI_FORMAT_P010,
    DXGI_FORMAT_P016,
    DXGI_FORMAT_420_OPAQUE,
    DXGI_FORMAT_YUY2,
    DXGI_FORMAT_Y210,
    DXGI_FORMAT_Y216,
    DXGI_FORMAT_NV11,
    DXGI_FORMAT_AI44,
    DXGI_FORMAT_IA44,
    DXGI_FORMAT_P8,
    DXGI_FORMAT_A8P8,
    DXGI_FORMAT_B4G4R4A4_UNORM,
    DXGI_FORMAT_P208,
    DXGI_FORMAT_V208,
    DXGI_FORMAT_V408,
    DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE,
    DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE,
    DXGI_FORMAT_FORCE_UINT,
};

enum D3D10_RESOURCE_DIMENSION {
    D3D10_RESOURCE_DIMENSION_UNKNOWN,
    D3D10_RESOURCE_DIMENSION_BUFFER,
    D3D10_RESOURCE_DIMENSION_TEXTURE1D,
    D3D10_RESOURCE_DIMENSION_TEXTURE2D,
    D3D10_RESOURCE_DIMENSION_TEXTURE3D,
};

constexpr uint32_t DDS_RESOURCE_MISC_TEXTURECUBE = 0x4;

struct DDS_HEADER_DXT10 {
    DXGI_FORMAT dxgiFormat;
    D3D10_RESOURCE_DIMENSION resourceDimension;
    uint32_t miscFlag;
    uint32_t arraySize;
    uint32_t miscFlags2;
};

const std::map<DXGI_FORMAT, TextureFormat> DXGI_MAPPING = {
    { DXGI_FORMAT_R8_SINT,              TextureFormat::R8_SINT              },
    { DXGI_FORMAT_R8_SNORM,             TextureFormat::R8_SNORM             },
    { DXGI_FORMAT_R8_UINT,              TextureFormat::R8_UINT              },
    { DXGI_FORMAT_R8_UNORM,             TextureFormat::R8_UNORM             },
    { DXGI_FORMAT_R8G8_SINT,            TextureFormat::RG8_SINT             },
    { DXGI_FORMAT_R8G8_SNORM,           TextureFormat::RG8_SNORM            },
    { DXGI_FORMAT_R8G8_UINT,            TextureFormat::RG8_UINT             },
    { DXGI_FORMAT_R8G8_UNORM,           TextureFormat::RG8_UNORM            },
    { DXGI_FORMAT_R8G8B8A8_SINT,        TextureFormat::RGBA8_SINT           },
    { DXGI_FORMAT_R8G8B8A8_SNORM,       TextureFormat::RGBA8_SNORM          },
    { DXGI_FORMAT_R8G8B8A8_UINT,        TextureFormat::RGBA8_UINT           },
    { DXGI_FORMAT_R8G8B8A8_UNORM,       TextureFormat::RGBA8_UNORM          },
    { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,  TextureFormat::RGBA8_UNORM_SRGB     },
    { DXGI_FORMAT_R16_FLOAT,            TextureFormat::R16_FLOAT            },
    { DXGI_FORMAT_R16_SINT,             TextureFormat::R16_SINT             },
    { DXGI_FORMAT_R16_SNORM,            TextureFormat::R16_SNORM            },
    { DXGI_FORMAT_R16_UINT,             TextureFormat::R16_UINT             },
    { DXGI_FORMAT_R16_UNORM,            TextureFormat::R16_UNORM            },
    { DXGI_FORMAT_R16G16_FLOAT,         TextureFormat::RG16_FLOAT           },
    { DXGI_FORMAT_R16G16_SINT,          TextureFormat::RG16_SINT            },
    { DXGI_FORMAT_R16G16_SNORM,         TextureFormat::RG16_SNORM           },
    { DXGI_FORMAT_R16G16_UINT,          TextureFormat::RG16_UINT            },
    { DXGI_FORMAT_R16G16_UNORM,         TextureFormat::RG16_UNORM           },
    { DXGI_FORMAT_R16G16B16A16_FLOAT,   TextureFormat::RGBA16_FLOAT         },
    { DXGI_FORMAT_R16G16B16A16_SINT,    TextureFormat::RGBA16_SINT          },
    { DXGI_FORMAT_R16G16B16A16_SNORM,   TextureFormat::RGBA16_SNORM         },
    { DXGI_FORMAT_R16G16B16A16_UINT,    TextureFormat::RGBA16_UINT          },
    { DXGI_FORMAT_R16G16B16A16_UNORM,   TextureFormat::RGBA16_UNORM         },
    { DXGI_FORMAT_R32_FLOAT,            TextureFormat::R32_FLOAT            },
    { DXGI_FORMAT_R32_SINT,             TextureFormat::R32_SINT             },
    { DXGI_FORMAT_R32_UINT,             TextureFormat::R32_UINT             },
    { DXGI_FORMAT_R32G32_FLOAT,         TextureFormat::RG32_FLOAT           },
    { DXGI_FORMAT_R32G32_SINT,          TextureFormat::RG32_SINT            },
    { DXGI_FORMAT_R32G32_UINT,          TextureFormat::RG32_UINT            },
    { DXGI_FORMAT_R32G32B32A32_FLOAT,   TextureFormat::RGBA32_FLOAT         },
    { DXGI_FORMAT_R32G32B32A32_SINT,    TextureFormat::RGBA32_SINT          },
    { DXGI_FORMAT_R32G32B32A32_UINT,    TextureFormat::RGBA32_UINT          },
    { DXGI_FORMAT_B8G8R8A8_UNORM,       TextureFormat::BGRA8_UNORM          },
    { DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,  TextureFormat::BGRA8_UNORM_SRGB     },
    { DXGI_FORMAT_D16_UNORM,            TextureFormat::D16_UNORM            },
    { DXGI_FORMAT_D24_UNORM_S8_UINT,    TextureFormat::D24_UNORM_S8_UINT    },
    { DXGI_FORMAT_D32_FLOAT,            TextureFormat::D32_FLOAT            },
    { DXGI_FORMAT_D32_FLOAT_S8X24_UINT, TextureFormat::D32_FLOAT_S8X24_UINT },
    { DXGI_FORMAT_BC1_UNORM,            TextureFormat::BC1_UNORM            },
    { DXGI_FORMAT_BC1_UNORM_SRGB,       TextureFormat::BC1_UNORM_SRGB       },
    { DXGI_FORMAT_BC2_UNORM,            TextureFormat::BC2_UNORM            },
    { DXGI_FORMAT_BC2_UNORM_SRGB,       TextureFormat::BC2_UNORM_SRGB       },
    { DXGI_FORMAT_BC3_UNORM,            TextureFormat::BC3_UNORM            },
    { DXGI_FORMAT_BC3_UNORM_SRGB,       TextureFormat::BC3_UNORM_SRGB       },
    { DXGI_FORMAT_BC4_SNORM,            TextureFormat::BC4_SNORM            },
    { DXGI_FORMAT_BC4_UNORM,            TextureFormat::BC4_UNORM            },
    { DXGI_FORMAT_BC5_SNORM,            TextureFormat::BC5_SNORM            },
    { DXGI_FORMAT_BC5_UNORM,            TextureFormat::BC5_UNORM            },
    { DXGI_FORMAT_BC6H_SF16,            TextureFormat::BC6H_SF16            },
    { DXGI_FORMAT_BC6H_UF16,            TextureFormat::BC6H_UF16            },
    { DXGI_FORMAT_BC7_UNORM,            TextureFormat::BC7_UNORM            },
    { DXGI_FORMAT_BC7_UNORM_SRGB,       TextureFormat::BC7_UNORM_SRGB       },
};

const std::map<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>, TextureFormat> MASK_MAPPING = {
    { { DDPF_LUMINANCE, 8,  0x000000FF, 0x00000000, 0x00000000, 0x00000000 }, TextureFormat::R8_UNORM    },
    { { DDPF_LUMINANCE, 8,  0x000000FF, 0x00000000, 0x00000000, 0x0000FF00 }, TextureFormat::RG8_UNORM   },
    { { DDPF_LUMINANCE, 16, 0x0000FFFF, 0x00000000, 0x00000000, 0x00000000 }, TextureFormat::R16_UNORM   },
    { { DDPF_LUMINANCE, 16, 0x000000FF, 0x0000FF00, 0x00000000, 0x00000000 }, TextureFormat::RG8_UNORM   },
    { { DDPF_BUMPDUDV,  16, 0x000000FF, 0x0000FF00, 0x00000000, 0x00000000 }, TextureFormat::RG8_SNORM   },
    { { DDPF_RGB,       32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000 }, TextureFormat::RGBA8_UNORM },
    { { DDPF_RGB,       32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000 }, TextureFormat::BGRA8_UNORM },
    { { DDPF_RGB,       32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000 }, TextureFormat::BGRA8_UNORM },
    { { DDPF_RGB,       32, 0x0000FFFF, 0xFFFF0000, 0x00000000, 0x00000000 }, TextureFormat::RG16_UNORM  },
    { { DDPF_RGB,       32, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 }, TextureFormat::R32_FLOAT   },
    { { DDPF_BUMPDUDV,  32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000 }, TextureFormat::RGBA8_SNORM },
    { { DDPF_BUMPDUDV,  32, 0x0000FFFF, 0xFFFF0000, 0x00000000, 0x00000000 }, TextureFormat::RG16_SNORM  },
};

const std::map<uint32_t, TextureFormat> FOURCC_MAPPING = {
    { '1TXD', TextureFormat::BC1_UNORM    },
    { '2TXD', TextureFormat::BC2_UNORM    },
    { '3TXD', TextureFormat::BC2_UNORM    },
    { '4TXD', TextureFormat::BC3_UNORM    },
    { '5TXD', TextureFormat::BC3_UNORM    },
    { '1ITA', TextureFormat::BC4_UNORM    },
    { 'U4CB', TextureFormat::BC4_UNORM    },
    { 'S4CB', TextureFormat::BC4_SNORM    },
    { '2ITA', TextureFormat::BC5_UNORM    },
    { 'U5CB', TextureFormat::BC5_UNORM    },
    { 'S5CB', TextureFormat::BC5_SNORM    },
    { 36,     TextureFormat::RGBA16_UNORM },
    { 110,    TextureFormat::RGBA16_SNORM },
    { 111,    TextureFormat::R16_FLOAT    },
    { 112,    TextureFormat::RG16_FLOAT   },
    { 113,    TextureFormat::RGBA16_FLOAT },
    { 114,    TextureFormat::R32_FLOAT    },
    { 115,    TextureFormat::RG32_FLOAT   },
    { 116,    TextureFormat::RGBA32_FLOAT },
};

struct FormatDescriptor {
    uint8_t bppb;
    bool is_compressed;
};

const FormatDescriptor FORMAT_DESCRIPTORS[] = {
    { 0,  false }, // TextureFormat::UNKNOWN
    { 1,  false }, // TextureFormat::R8_SINT
    { 1,  false }, // TextureFormat::R8_SNORM
    { 1,  false }, // TextureFormat::R8_UINT
    { 1,  false }, // TextureFormat::R8_UNORM
    { 2,  false }, // TextureFormat::RG8_SINT
    { 2,  false }, // TextureFormat::RG8_SNORM
    { 2,  false }, // TextureFormat::RG8_UINT
    { 2,  false }, // TextureFormat::RG8_UNORM
    { 4,  false }, // TextureFormat::RGBA8_SINT
    { 4,  false }, // TextureFormat::RGBA8_SNORM
    { 4,  false }, // TextureFormat::RGBA8_UINT
    { 4,  false }, // TextureFormat::RGBA8_UNORM
    { 4,  false }, // TextureFormat::RGBA8_UNORM_SRGB
    { 2,  false }, // TextureFormat::R16_FLOAT
    { 2,  false }, // TextureFormat::R16_SINT
    { 2,  false }, // TextureFormat::R16_SNORM
    { 2,  false }, // TextureFormat::R16_UINT
    { 2,  false }, // TextureFormat::R16_UNORM
    { 4,  false }, // TextureFormat::RG16_FLOAT
    { 4,  false }, // TextureFormat::RG16_SINT
    { 4,  false }, // TextureFormat::RG16_SNORM
    { 4,  false }, // TextureFormat::RG16_UINT
    { 4,  false }, // TextureFormat::RG16_UNORM
    { 8,  false }, // TextureFormat::RGBA16_FLOAT
    { 8,  false }, // TextureFormat::RGBA16_SINT
    { 8,  false }, // TextureFormat::RGBA16_SNORM
    { 8,  false }, // TextureFormat::RGBA16_UINT
    { 8,  false }, // TextureFormat::RGBA16_UNORM
    { 4,  false }, // TextureFormat::R32_FLOAT
    { 4,  false }, // TextureFormat::R32_SINT
    { 4,  false }, // TextureFormat::R32_UINT
    { 8,  false }, // TextureFormat::RG32_FLOAT
    { 8,  false }, // TextureFormat::RG32_SINT
    { 8,  false }, // TextureFormat::RG32_UINT
    { 16, false }, // TextureFormat::RGBA32_FLOAT
    { 16, false }, // TextureFormat::RGBA32_SINT
    { 16, false }, // TextureFormat::RGBA32_UINT
    { 4,  false }, // TextureFormat::BGRA8_UNORM
    { 4,  false }, // TextureFormat::BGRA8_UNORM_SRGB
    { 2,  false }, // TextureFormat::D16_UNORM
    { 4,  false }, // TextureFormat::D24_UNORM_S8_UINT
    { 4,  false }, // TextureFormat::D32_FLOAT
    { 8,  false }, // TextureFormat::D32_FLOAT_S8X24_UINT
    { 8,  true  }, // TextureFormat::BC1_UNORM
    { 8,  true  }, // TextureFormat::BC1_UNORM_SRGB
    { 16, true  }, // TextureFormat::BC2_UNORM
    { 16, true  }, // TextureFormat::BC2_UNORM_SRGB
    { 16, true  }, // TextureFormat::BC3_UNORM
    { 16, true  }, // TextureFormat::BC3_UNORM_SRGB
    { 8,  true  }, // TextureFormat::BC4_SNORM
    { 8,  true  }, // TextureFormat::BC4_UNORM
    { 16, true  }, // TextureFormat::BC5_SNORM
    { 16, true  }, // TextureFormat::BC5_UNORM
    { 16, true  }, // TextureFormat::BC6H_SF16
    { 16, true  }, // TextureFormat::BC6H_UF16
    { 16, true  }, // TextureFormat::BC7_UNORM
    { 16, true  }, // TextureFormat::BC7_UNORM_SRGB
};

} // namespace render_utils_details

TextureDescriptor load_dds(MemoryResource& memory_resource, const String& relative_path) {
    using namespace render_utils_details;

    Parser parser(memory_resource, relative_path);

    const auto* magic = parser.read<uint32_t>();
    KW_ERROR(magic != nullptr, "Failed to read DDS_SIGNATURE from \"%s\".", relative_path.c_str());
    KW_ERROR(*magic == DDS_SIGNATURE, "Invalid DDS_SIGNATURE in \"%s\".", relative_path.c_str());

    //
    // Validate header.
    //

    const auto* header = parser.read<DDS_HEADER>();
    KW_ERROR(header != nullptr, "Failed to read DDS_HEADER from \"%s\".", relative_path.c_str());
    KW_ERROR(header->dwSize == sizeof(DDS_HEADER), "Invalid DDS_HEADER size in \"%s\".", relative_path.c_str());
    KW_ERROR((header->dwFlags & DDSD_REQUIRED_FLAGS) == DDSD_REQUIRED_FLAGS, "DDSD_CAPS, DDSD_HEIGHT, DDSD_WIDTH and DDSD_PIXELFORMAT flags are not specified in \"%s\".", relative_path.c_str());
    KW_ERROR((header->dwCaps & DDSCAPS_TEXTURE) == DDSCAPS_TEXTURE, "DDSCAPS_TEXTURE cap is not specified in \"%s\".", relative_path.c_str());
    KW_ERROR(header->dwWidth != 0 && header->dwHeight != 0, "Invalid texture size in \"%s\".", relative_path.c_str());
    KW_ERROR(((header->dwFlags & DDSD_MIPMAPCOUNT) != 0) == ((header->dwCaps & DDSCAPS_MIPMAP) != 0), "DDSCAPS_MIPMAP is specified, but DDSD_MIPMAPCOUNT is not in \"%s\".", relative_path.c_str());
    KW_ERROR((header->dwCaps & DDSCAPS_MIPMAP) == 0 || header->dwMipMapCount != 0, "DDSCAPS_MIPMAP is specified, but dwMipMapCount is equal to 0 in \"%s\".", relative_path.c_str());
    KW_ERROR((header->dwCaps & DDSCAPS_MIPMAP) == 0 || header->dwMipMapCount <= 16, "dwMipMapCount is too large in \"%s\".", relative_path.c_str());
    KW_ERROR(((header->dwFlags & DDSD_DEPTH) != 0) == ((header->dwCaps2 & DDSCAPS2_VOLUME) != 0), "DDSCAPS2_VOLUME is specified, but DDSD_DEPTH is not specified in \"%s\".", relative_path.c_str());
    KW_ERROR((header->dwFlags & DDSD_DEPTH) == 0 || header->dwDepth != 0, "DDSD_DEPTH is specified, but dwDepth is equal to 0 in \"%s\".", relative_path.c_str());
    KW_ERROR((header->dwCaps2 & DDSCAPS2_CUBEMAP) == 0 || (header->dwCaps2 & DDSCAPS2_VOLUME) == 0, "DDSCAPS2_CUBEMAP is incompatible with DDSCAPS2_VOLUME in \"%s\".", relative_path.c_str());
    KW_ERROR((header->dwCaps2 & DDSCAPS2_CUBEMAP) == 0 || (header->dwCaps2 & DDSCAPS2_CUBEMAP_ALLFACES) == DDSCAPS2_CUBEMAP_ALLFACES, "Incomplete cubemap in \"%s\".", relative_path.c_str());
    KW_ERROR(header->ddspf.dwSize == sizeof(DDS_PIXELFORMAT), "Invalid DDS_PIXELFORMAT size in \"%s\".", relative_path.c_str());
    KW_ERROR((header->ddspf.dwFlags & (DDPF_ALPHA | DDPF_YUV)) == 0, "DDPF_ALPHA and DDPF_YUV pixel format flags are not supported in \"%s\".", relative_path.c_str());
    KW_ERROR(((header->ddspf.dwFlags & DDPF_RGB) != 0) != ((header->ddspf.dwFlags & DDPF_FOURCC) != 0), "Both DDPF_RGB and DDPF_FOURCC are specified in \"%s\".", relative_path.c_str());

    //
    // Calculate format.
    //

    TextureFormat format;

    const DDS_HEADER_DXT10* header10 = nullptr;
    if (header->ddspf.dwFlags == DDPF_FOURCC && header->ddspf.dwFourCC == DDPF_FOURCC_DX10) {
        header10 = parser.read<DDS_HEADER_DXT10>();
        KW_ERROR(header10 != nullptr, "Failed to read DDS_HEADER_DXT10 from \"%s\".", relative_path.c_str());
        KW_ERROR(header10->resourceDimension >= D3D10_RESOURCE_DIMENSION_BUFFER && header10->resourceDimension <= D3D10_RESOURCE_DIMENSION_TEXTURE3D, "Invalid resourceDimension in \"%s\".", relative_path.c_str());
        KW_ERROR((header10->resourceDimension == D3D10_RESOURCE_DIMENSION_TEXTURE3D) == ((header->dwCaps2 & DDSCAPS2_VOLUME) != 0), "Inconsistent 3D texture in \"%s\".", relative_path.c_str());
        KW_ERROR(((header10->miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE) != 0) == ((header->dwCaps2 & DDSCAPS2_CUBEMAP) != 0), "Inconsistent cube texture in \"%s\".", relative_path.c_str());
        KW_ERROR(header10->arraySize != 0, "Array size must be at least 1 in \"%s\".", relative_path.c_str());
        KW_ERROR(header10->resourceDimension != D3D10_RESOURCE_DIMENSION_TEXTURE3D || header10->arraySize == 1, "An array of 3D textures is not supported in \"%s\".", relative_path.c_str());

        auto it = DXGI_MAPPING.find(header10->dxgiFormat);
        KW_ERROR(it != DXGI_MAPPING.end(), "Unsupported DXGI format in \"%s\".", relative_path.c_str());

        format = it->second;
    } else {
        if (header->ddspf.dwFlags == DDPF_FOURCC) {
            auto it = FOURCC_MAPPING.find(header->ddspf.dwFourCC);
            KW_ERROR(it != FOURCC_MAPPING.end(), "Unsupported FOURCC format in \"%s\".", relative_path.c_str());

            format = it->second;
        } else {
            std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t> key{
                header->ddspf.dwFlags & (DDPF_LUMINANCE | DDPF_BUMPDUDV | DDPF_RGB),
                header->ddspf.dwRGBBitCount,
                header->ddspf.dwRBitMask,
                header->ddspf.dwGBitMask,
                header->ddspf.dwBBitMask,
                header->ddspf.dwABitMask,
            };

            auto it = MASK_MAPPING.find(key);
            KW_ERROR(it != MASK_MAPPING.end(), "Unsupported MASK format in \"%s\".", relative_path.c_str());

            format = it->second;
        }
    }

    //
    // Calculate texture type.
    //

    TextureType texture_type;
    if ((header->dwCaps2 & DDSCAPS2_CUBEMAP) != 0) {
        texture_type = TextureType::TEXTURE_CUBE;
    } else {
        if ((header->dwCaps2 & DDSCAPS2_VOLUME) != 0) {
            texture_type = TextureType::TEXTURE_3D;
        } else {
            texture_type = TextureType::TEXTURE_2D;
        }
    }

    //
    // Compose the texture descriptor.
    //

    uint32_t array_size = header10 != nullptr ? header10->arraySize : 1;
    uint32_t side_count = (header->dwCaps2 & DDSCAPS2_CUBEMAP) != 0 ? 6 : 1;
    uint32_t mip_levels = (header->dwFlags & DDSD_MIPMAPCOUNT) != 0 ? header->dwMipMapCount : 1;
    size_t* offsets = memory_resource.allocate<size_t>(array_size * side_count * mip_levels);

    TextureDescriptor texture_descriptor{};
    texture_descriptor.name = relative_path.c_str();
    texture_descriptor.data = nullptr;
    texture_descriptor.size = 0;
    texture_descriptor.type = texture_type;
    texture_descriptor.format = format;
    texture_descriptor.array_size = array_size * side_count;
    texture_descriptor.mip_levels = mip_levels;
    texture_descriptor.width = header->dwWidth;
    texture_descriptor.height = header->dwHeight;
    texture_descriptor.depth = (header->dwFlags & DDSD_DEPTH) != 0 ? header->dwDepth : 1;
    texture_descriptor.offsets = offsets;

    //
    // Calculate data size, array and mip offsets.
    //

    const FormatDescriptor& format_descriptor = FORMAT_DESCRIPTORS[static_cast<size_t>(format)];

    for (uint32_t array_index = 0; array_index < texture_descriptor.array_size; array_index++) {
        uint32_t width = texture_descriptor.width;
        uint32_t height = texture_descriptor.height;
        uint32_t depth = texture_descriptor.depth;

        for (uint32_t mip_index = 0; mip_index < texture_descriptor.mip_levels; mip_index++) {
            uint32_t bytes_count;

            if (format_descriptor.is_compressed) {
                uint32_t blocks_wide_count = (width + 3) / 4;
                uint32_t blocks_high_count = (height + 3) / 4;

                bytes_count = blocks_wide_count * blocks_high_count * depth * format_descriptor.bppb;
            } else {
                bytes_count = width * height * depth * format_descriptor.bppb;
            }

            offsets[array_index * texture_descriptor.mip_levels + mip_index] = texture_descriptor.size;

            const uint8_t* data = parser.read(bytes_count);
            KW_ERROR(data != nullptr, "Failed to read a texture \"%s\".", relative_path.c_str());

            if (texture_descriptor.data == nullptr) {
                texture_descriptor.data = data;
            }
            texture_descriptor.size += bytes_count;

            width = std::max(width / 2, 1U);
            height = std::max(height / 2, 1U);
            depth = std::max(depth / 2, 1U);
        }
    }

    return texture_descriptor;
}

} // namespace kw::RenderUtils
