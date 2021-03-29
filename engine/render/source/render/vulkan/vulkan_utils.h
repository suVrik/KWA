#include <core/error.h>

#include <vulkan/vulkan.h>

#include <SDL2/SDL_stdinc.h>

#include <spirv_reflect.h>

#define VK_ERROR(statement, ...)  KW_ERROR((statement) == VK_SUCCESS, ##__VA_ARGS__)
#define SDL_ERROR(statement, ...) KW_ERROR((statement) == SDL_TRUE, ##__VA_ARGS__)
#define SPV_ERROR(statement, ...) KW_ERROR((statement) == SPV_REFLECT_RESULT_SUCCESS, ##__VA_ARGS__)

#define VK_NAME(render, handle, format, ...) render->set_debug_name(handle, format, ##__VA_ARGS__)
