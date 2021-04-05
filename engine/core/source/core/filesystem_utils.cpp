#include "core/filesystem_utils.h"
#include "core/error.h"

#include <Windows.h>
#include <Shlwapi.h>

namespace kw::FilesystemUtils {

Vector<std::byte> read_file(MemoryResource& memory_resource, const String& relative_path) {
    TCHAR executable_directory[MAX_PATH];

    KW_ERROR(
        GetModuleFileName(NULL, executable_directory, MAX_PATH) < MAX_PATH,
        "Failed to get path to executable."
    );

    KW_ERROR(
        PathRemoveFileSpec(executable_directory) == TRUE,
        "Failed to remove executable filename."
    );

    TCHAR file_relative[MAX_PATH];

    KW_ERROR(
        MultiByteToWideChar(CP_UTF8, 0, relative_path.c_str(), -1, file_relative, MAX_PATH) > 0,
        "Failed to convert file \"%s\" relative path to UNICODE.", relative_path.c_str()
    );

    for (size_t i = 0; i < MAX_PATH; i++) {
        if (file_relative[i] == '/') {
            file_relative[i] = '\\';
        }
    }

    TCHAR file_absolute[MAX_PATH];

    KW_ERROR(
        PathCombine(file_absolute, executable_directory, file_relative) != NULL,
        "Failed to combine executable directory and file \"%s\" relative path.", relative_path.c_str()
    );

    KW_ERROR(
        PathFileExists(file_absolute) == TRUE,
        "File \"%s\" doesn't exist.", relative_path.c_str()
    );

    HANDLE file = CreateFile(file_absolute, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    KW_ERROR(
        file != INVALID_HANDLE_VALUE,
        "Failed to open file \"%s\".", relative_path.c_str()
    );

    DWORD bytes_total = GetFileSize(file, NULL);

    KW_ERROR(
        bytes_total != INVALID_FILE_SIZE,
        "Failed to query file \"%s\" size", relative_path.c_str()
    );

    Vector<std::byte> result(bytes_total, memory_resource);

    DWORD bytes_read;
    
    KW_ERROR(
        ReadFile(file, result.data(), bytes_total, &bytes_read, NULL) == TRUE,
        "Failed to read from file \"%s\".", relative_path.c_str()
    );

    KW_ERROR(
        bytes_read == bytes_total,
        "File \"%s\" size mismatch.", relative_path.c_str()
    );

    return result;
}

} // namespace kw::FilesystemUtils
