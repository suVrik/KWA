#include "core/filesystem_utils.h"
#include "core/error.h"

#include <Windows.h>
#include <Shlwapi.h>

namespace kw::FilesystemUtils {

bool file_exists(const char* relative_path) {
    TCHAR executable_directory[MAX_PATH];

    if (GetModuleFileName(NULL, executable_directory, MAX_PATH) >= MAX_PATH) {
        return false;
    }

    if (PathRemoveFileSpec(executable_directory) == FALSE) {
        return false;
    }

    TCHAR file_relative[MAX_PATH];

    if (MultiByteToWideChar(CP_UTF8, 0, relative_path, -1, file_relative, MAX_PATH) <= 0) {
        return false;
    }

    for (size_t i = 0; i < MAX_PATH; i++) {
        if (file_relative[i] == '/') {
            file_relative[i] = '\\';
        }
    }

    TCHAR file_absolute[MAX_PATH];

    if (PathCombine(file_absolute, executable_directory, file_relative) == NULL) {
        return false;
    }

    if (PathFileExists(file_absolute) == FALSE) {
        return false;
    }

    HANDLE file = CreateFile(file_absolute, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD bytes_total = GetFileSize(file, NULL);

    if (bytes_total == INVALID_FILE_SIZE) {
        return false;
    }

    if (CloseHandle(file) == FALSE) {
        return false;
    }
    
    return true;
}

Vector<uint8_t> read_file(MemoryResource& memory_resource, const char* relative_path) {
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
        MultiByteToWideChar(CP_UTF8, 0, relative_path, -1, file_relative, MAX_PATH) > 0,
        "Failed to convert file \"%s\" relative path to UNICODE.", relative_path
    );

    for (size_t i = 0; i < MAX_PATH; i++) {
        if (file_relative[i] == '/') {
            file_relative[i] = '\\';
        }
    }

    TCHAR file_absolute[MAX_PATH];

    KW_ERROR(
        PathCombine(file_absolute, executable_directory, file_relative) != NULL,
        "Failed to combine executable directory and file \"%s\" relative path.", relative_path
    );

    KW_ERROR(
        PathFileExists(file_absolute) == TRUE,
        "File \"%s\" doesn't exist.", relative_path
    );

    HANDLE file = CreateFile(file_absolute, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    KW_ERROR(
        file != INVALID_HANDLE_VALUE,
        "Failed to open file \"%s\".", relative_path
    );

    DWORD bytes_total = GetFileSize(file, NULL);

    KW_ERROR(
        bytes_total != INVALID_FILE_SIZE,
        "Failed to query file \"%s\" size", relative_path
    );

    Vector<uint8_t> result(bytes_total, memory_resource);

    DWORD bytes_read;
    
    KW_ERROR(
        ReadFile(file, result.data(), bytes_total, &bytes_read, NULL) == TRUE,
        "Failed to read from file \"%s\".", relative_path
    );

    KW_ERROR(
        bytes_read == bytes_total,
        "File \"%s\" size mismatch.", relative_path
    );

    KW_ERROR(
        CloseHandle(file) == TRUE,
        "Failed to close file \"%s\".", relative_path
    );

    return result;
}

} // namespace kw::FilesystemUtils
