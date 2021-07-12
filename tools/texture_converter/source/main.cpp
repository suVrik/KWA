#include <render/render.h>

#include <core/io/binary_reader.h>
#include <core/io/binary_writer.h>
#include <core/utils/endian_utils.h>

#include <iostream>
#include <map>
#include <vector>

using namespace kw;

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

namespace kw::EndianUtils {

static DDS_HEADER swap_le(const DDS_HEADER& dds_header) {
    DDS_HEADER result;
    result.dwSize = swap_le(dds_header.dwSize);
    result.dwFlags = swap_le(dds_header.dwFlags);
    result.dwHeight = swap_le(dds_header.dwHeight);
    result.dwWidth = swap_le(dds_header.dwWidth);
    result.dwPitchOrLinearSize = swap_le(dds_header.dwPitchOrLinearSize);
    result.dwDepth = swap_le(dds_header.dwDepth);
    result.dwMipMapCount = swap_le(dds_header.dwMipMapCount);
    result.ddspf.dwSize = swap_le(dds_header.ddspf.dwSize);
    result.ddspf.dwFlags = swap_le(dds_header.ddspf.dwFlags);
    result.ddspf.dwFourCC = swap_le(dds_header.ddspf.dwFourCC);
    result.ddspf.dwRGBBitCount = swap_le(dds_header.ddspf.dwRGBBitCount);
    result.ddspf.dwRBitMask = swap_le(dds_header.ddspf.dwRBitMask);
    result.ddspf.dwGBitMask = swap_le(dds_header.ddspf.dwGBitMask);
    result.ddspf.dwBBitMask = swap_le(dds_header.ddspf.dwBBitMask);
    result.ddspf.dwABitMask = swap_le(dds_header.ddspf.dwABitMask);
    result.dwCaps = swap_le(dds_header.dwCaps);
    result.dwCaps2 = swap_le(dds_header.dwCaps2);
    result.dwCaps3 = swap_le(dds_header.dwCaps3);
    result.dwCaps4 = swap_le(dds_header.dwCaps4);
    result.dwReserved2 = swap_le(dds_header.dwReserved2);
    return result;
}

} // namespace kw::EndianUtils

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

namespace kw::EndianUtils {

static DDS_HEADER_DXT10 swap_le(const DDS_HEADER_DXT10& dds_header_dxt12) {
    DDS_HEADER_DXT10 result;
    result.dxgiFormat = static_cast<DXGI_FORMAT>(swap_le(static_cast<int>(dds_header_dxt12.dxgiFormat)));
    result.resourceDimension = static_cast<D3D10_RESOURCE_DIMENSION>(swap_le(static_cast<int>(dds_header_dxt12.resourceDimension)));
    result.miscFlag = swap_le(dds_header_dxt12.miscFlag);
    result.arraySize = swap_le(dds_header_dxt12.arraySize);
    result.miscFlags2 = swap_le(dds_header_dxt12.miscFlags2);
    return result;
}

} // namespace kw::EndianUtils

static const std::map<DXGI_FORMAT, TextureFormat> DXGI_MAPPING = {
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
    { DXGI_FORMAT_R32G32B32_FLOAT,      TextureFormat::RGB32_FLOAT          },
    { DXGI_FORMAT_R32G32B32_SINT,       TextureFormat::RGB32_SINT           },
    { DXGI_FORMAT_R32G32B32_UINT,       TextureFormat::RGB32_UINT           },
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

typedef std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t> MASK_KEY;

static const std::map<MASK_KEY, TextureFormat> MASK_MAPPING = {
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

static const std::map<uint32_t, TextureFormat> FOURCC_MAPPING = {
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

constexpr uint32_t KWT_SIGNATURE = ' TWK';

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Texture converter requires at two command line arguments: input *.DDS file and output *.KWT file." << std::endl;
        return 1;
    }

    BinaryReader reader(argv[1]);

    std::optional<uint32_t> magic = reader.read_le<uint32_t>();
    if (!magic) {
        std::cout << "Failed to read DDS_SIGNATURE from \"" << argv[1] << "\"." << std::endl;
        return 1;
    } else if (*magic != DDS_SIGNATURE) {
        std::cout << "Invalid DDS_SIGNATURE in \"" << argv[1] << "\"." << std::endl;
        return 1;
    }

    //
    // Validate header.
    //

    std::optional<DDS_HEADER> header = reader.read_le<DDS_HEADER>();
    if (!header) {
        std::cout << "Failed to read DDS_HEADER from \"" << argv[1] << "\"." << std::endl;
        return 1;
    } else if (header->dwSize != sizeof(DDS_HEADER)) {
        std::cout << "Invalid DDS_HEADER size in \"" << argv[1] << "\"." << std::endl;
        return 1;
    } else if ((header->dwFlags & DDSD_REQUIRED_FLAGS) != DDSD_REQUIRED_FLAGS) {
        std::cout << "DDSD_CAPS, DDSD_HEIGHT, DDSD_WIDTH and DDSD_PIXELFORMAT flags are not specified in \"" << argv[1] << "\"." << std::endl;
        return 1;
    } else if ((header->dwCaps & DDSCAPS_TEXTURE) != DDSCAPS_TEXTURE) {
        std::cout << "DDSCAPS_TEXTURE cap is not specified in \"" << argv[1] << "\"." << std::endl;
        return 1;
    } else if (header->dwWidth == 0 || header->dwHeight == 0) {
        std::cout << "Invalid texture size in \"" << argv[1] << "\"." << std::endl;
        return 1;
    } else if (((header->dwFlags & DDSD_MIPMAPCOUNT) != 0) != ((header->dwCaps & DDSCAPS_MIPMAP) != 0)) {
        std::cout << "DDSCAPS_MIPMAP is specified, but DDSD_MIPMAPCOUNT is not in \"" << argv[1] << "\"." << std::endl;
        return 1;
    } else if ((header->dwCaps & DDSCAPS_MIPMAP) != 0 && header->dwMipMapCount == 0) {
        std::cout << "DDSCAPS_MIPMAP is specified, but dwMipMapCount is equal to 0 in \"" << argv[1] << "\"." << std::endl;
        return 1;
    } else if (((header->dwFlags & DDSD_DEPTH) != 0) != ((header->dwCaps2 & DDSCAPS2_VOLUME) != 0)) {
        std::cout << "DDSCAPS2_VOLUME is specified, but DDSD_DEPTH is not specified in \"" << argv[1] << "\"." << std::endl;
        return 1;
    } else if ((header->dwFlags & DDSD_DEPTH) != 0 && header->dwDepth == 0) {
        std::cout << "DDSD_DEPTH is specified, but dwDepth is equal to 0 in \"" << argv[1] << "\"." << std::endl;
        return 1;
    } else if ((header->dwCaps2 & DDSCAPS2_CUBEMAP) != 0 && (header->dwCaps2 & DDSCAPS2_VOLUME) != 0) {
        std::cout << "DDSCAPS2_CUBEMAP is incompatible with DDSCAPS2_VOLUME in \"" << argv[1] << "\"." << std::endl;
        return 1;
    } else if ((header->dwCaps2 & DDSCAPS2_CUBEMAP) != 0 && (header->dwCaps2 & DDSCAPS2_CUBEMAP_ALLFACES) != DDSCAPS2_CUBEMAP_ALLFACES) {
        std::cout << "Incomplete cubemap in \"" << argv[1] << "\"." << std::endl;
        return 1;
    } else if (header->ddspf.dwSize != sizeof(DDS_PIXELFORMAT)) {
        std::cout << "Invalid DDS_PIXELFORMAT size in \"" << argv[1] << "\"." << std::endl;
        return 1;
    } else if ((header->ddspf.dwFlags & (DDPF_ALPHA | DDPF_YUV)) != 0) {
        std::cout << "DDPF_ALPHA and DDPF_YUV pixel format flags are not supported in \"" << argv[1] << "\"." << std::endl;
        return 1;
    } else if (((header->ddspf.dwFlags & DDPF_RGB) != 0) == ((header->ddspf.dwFlags & DDPF_FOURCC) != 0)) {
        std::cout << "Both DDPF_RGB and DDPF_FOURCC are specified in \"" << argv[1] << "\"." << std::endl;
        return 1;
    }

    //
    // Calculate format.
    //

    TextureFormat format;

    DDS_HEADER_DXT10 header10;
    if (header->ddspf.dwFlags == DDPF_FOURCC && header->ddspf.dwFourCC == DDPF_FOURCC_DX10) {
        if (!reader.read_le<DDS_HEADER_DXT10>(&header10)) {
            std::cout << "Failed to read DDS_HEADER_DXT10 from \"" << argv[1] << "\"." << std::endl;
            return 1;
        } else if (header10.resourceDimension < D3D10_RESOURCE_DIMENSION_BUFFER || header10.resourceDimension > D3D10_RESOURCE_DIMENSION_TEXTURE3D) {
            std::cout << "Invalid resourceDimension in \"" << argv[1] << "\"." << std::endl;
            return 1;
        } else if ((header10.resourceDimension == D3D10_RESOURCE_DIMENSION_TEXTURE3D) != ((header->dwCaps2 & DDSCAPS2_VOLUME) != 0)) {
            std::cout << "Inconsistent 3D texture in \"" << argv[1] << "\"." << std::endl;
            return 1;
        } else if (((header10.miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE) != 0) != ((header->dwCaps2 & DDSCAPS2_CUBEMAP) != 0)) {
            std::cout << "Inconsistent cube texture in \"" << argv[1] << "\"." << std::endl;
            return 1;
        } else if (header10.arraySize == 0) {
            std::cout << "Array size must be at least 1 in \"" << argv[1] << "\"." << std::endl;
            return 1;
        } else if (header10.resourceDimension == D3D10_RESOURCE_DIMENSION_TEXTURE3D && header10.arraySize != 1) {
            std::cout << "An array of 3D textures is not supported in \"" << argv[1] << "\"." << std::endl;
            return 1;
        }

        auto it = DXGI_MAPPING.find(header10.dxgiFormat);
        if (it == DXGI_MAPPING.end()) {
            std::cout << "Unsupported DXGI format in \"" << argv[1] << "\"." << std::endl;
            return 1;
        }

        format = it->second;
    } else {
        if (header->ddspf.dwFlags == DDPF_FOURCC) {
            auto it = FOURCC_MAPPING.find(header->ddspf.dwFourCC);
            if (it == FOURCC_MAPPING.end()) {
                std::cout << "Unsupported FOURCC format in \"" << argv[1] << "\"." << std::endl;
                return 1;
            }

            format = it->second;
        } else {
            MASK_KEY key{
                header->ddspf.dwFlags & (DDPF_LUMINANCE | DDPF_BUMPDUDV | DDPF_RGB),
                header->ddspf.dwRGBBitCount,
                header->ddspf.dwRBitMask,
                header->ddspf.dwGBitMask,
                header->ddspf.dwBBitMask,
                header->ddspf.dwABitMask,
            };

            auto it = MASK_MAPPING.find(key);
            if (it == MASK_MAPPING.end()) {
                std::cout << "Unsupported MASK format in \"" << argv[1] << "\"." << std::endl;
                return 1;
            }

            format = it->second;
        }
    }

    //
    // Calculate texture type.
    //

    TextureType type;
    if ((header->dwCaps2 & DDSCAPS2_CUBEMAP) != 0) {
        type = TextureType::TEXTURE_CUBE;
    } else {
        if ((header->dwCaps2 & DDSCAPS2_VOLUME) != 0) {
            type = TextureType::TEXTURE_3D;
        } else {
            type = TextureType::TEXTURE_2D;
        }
    }

    //
    // Texture properties.
    //

    uint32_t mip_level_count = (header->dwFlags & DDSD_MIPMAPCOUNT) != 0 ? header->dwMipMapCount : 1;
    uint32_t array_layer_count = header->ddspf.dwFlags == DDPF_FOURCC && header->ddspf.dwFourCC == DDPF_FOURCC_DX10 ? header10.arraySize : 1;
    uint32_t side_count = (header->dwCaps2 & DDSCAPS2_CUBEMAP) != 0 ? 6 : 1;
    uint32_t width = header->dwWidth;
    uint32_t height = header->dwHeight;
    uint32_t depth = (header->dwFlags & DDSD_DEPTH) != 0 ? header->dwDepth : 1;

    //
    // Store texture data in DDS layout.
    //

    std::vector<std::vector<std::vector<uint8_t>>> data(array_layer_count * side_count, std::vector<std::vector<uint8_t>>(mip_level_count));

    for (uint32_t array_layer = 0; array_layer < array_layer_count * side_count; array_layer++) {
        uint32_t w = width;
        uint32_t h = height;
        uint32_t d = depth;

        for (uint32_t mip_level = 0; mip_level < mip_level_count; mip_level++) {
            uint32_t bytes_count;

            if (TextureFormatUtils::is_compressed(format)) {
                bytes_count = ((w + 3) / 4) * ((h + 3) / 4) * d * TextureFormatUtils::get_texel_size(format);
            } else {
                bytes_count = w * h * d * TextureFormatUtils::get_texel_size(format);
            }

            data[array_layer][mip_level] = std::vector<uint8_t>(bytes_count);

            if (!reader.read(data[array_layer][mip_level].data(), data[array_layer][mip_level].size())) {
                std::cout << "Failed to read a texture \"" << argv[1] << "\"." << std::endl;
                return 1;
            }

            w = std::max(w / 2, 1U);
            h = std::max(h / 2, 1U);
            d = std::max(d / 2, 1U);
        }
    }

    //
    // Write output texture.
    //

    BinaryWriter writer(argv[2]);

    if (!writer) {
        std::cout << "Failed to open output texture file \"" << argv[2] << "\"." << std::endl;
        return 1;
    }

    writer.write_le<uint32_t>(KWT_SIGNATURE);
    writer.write_le<uint32_t>(type);
    writer.write_le<uint32_t>(format);
    writer.write_le<uint32_t>(mip_level_count);
    writer.write_le<uint32_t>(array_layer_count * side_count);
    writer.write_le<uint32_t>(width);
    writer.write_le<uint32_t>(height);
    writer.write_le<uint32_t>(depth);

    for (int32_t mip_level = static_cast<int32_t>(mip_level_count) - 1; mip_level >= 0; mip_level--) {
        for (uint32_t array_layer = 0; array_layer < array_layer_count * side_count; array_layer++) {
            writer.write(data[array_layer][mip_level].data(), data[array_layer][mip_level].size());
        }
    }

    if (!writer) {
        std::cout << "Failed to write to output texture file \"" << argv[2] << "\"." << std::endl;
        return 1;
    }

    return 0;
}
