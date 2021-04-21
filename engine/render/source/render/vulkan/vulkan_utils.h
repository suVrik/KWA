#include "render/render.h"

#include <core/error.h>

#include <vulkan/vulkan.h>

#include <SDL2/SDL_stdinc.h>

#include <spirv_reflect.h>

#define VK_ERROR(statement, ...)                     \
do {                                                 \
   VkResult vk_result = (statement);                 \
   KW_ERROR(vk_result == VK_SUCCESS, ##__VA_ARGS__); \
} while (false)

#define SDL_ERROR(statement, ...) KW_ERROR((statement) == SDL_TRUE, ##__VA_ARGS__)

#define SPV_ERROR(statement, ...)                                 \
do {                                                              \
   SpvReflectResult result = (statement);                         \
   KW_ERROR(result == SPV_REFLECT_RESULT_SUCCESS, ##__VA_ARGS__); \
} while (false)

#define VK_NAME(render, handle, format, ...) (render).set_debug_name((handle), (format), ##__VA_ARGS__)

namespace kw::TextureFormatUtils {

VkFormat convert_format_vulkan(TextureFormat format);

} // namespace kw::TextureFormatUtils
