#pragma once

#if defined(_WIN64)
#define KW_WINDOWS
#elif defined(__APPLE__)
#define KW_MACOS
#else
#define KW_LINUX
#endif
