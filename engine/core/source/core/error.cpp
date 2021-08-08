#include "core/error.h"
#include "core/debug/debug_utils.h"
#include "core/resource.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include <Windows.h>

namespace kw::error_details {

static INT_PTR CALLBACK dialog_callback(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_INITDIALOG:
        MessageBeep(MB_ICONERROR);
        SetWindowTextA(GetDlgItem(hwnd, IDC_ERROR_MESSAGE), reinterpret_cast<char*>(lparam));
        return TRUE;
    case WM_COMMAND:
        switch (wparam) {
        case IDC_ERROR_BREAK:
            EndDialog(hwnd, 0);
            return TRUE;
        case IDC_ERROR_COPY_MESSAGE:
            if (IsClipboardFormatAvailable(CF_TEXT)) {
                if (OpenClipboard(nullptr)) {
                    if (EmptyClipboard()) {
                        int length = GetWindowTextLengthA(GetDlgItem(hwnd, IDC_ERROR_MESSAGE));
                        if (length > 0) {
                            HGLOBAL memory = GlobalAlloc(GHND, static_cast<size_t>(length) + 1);
                            if (memory != nullptr) {
                                LPVOID text = GlobalLock(memory);
                                if (text != nullptr) {
                                    GetWindowTextA(GetDlgItem(hwnd, IDC_ERROR_MESSAGE), static_cast<LPSTR>(text), length + 1);

                                    GlobalUnlock(memory);

                                    // `SetClipboardData` takes ownership of memory, so free is not needed.
                                    if (!SetClipboardData(CF_TEXT, memory)) {
                                        GlobalFree(memory);
                                    }
                                } else {
                                    GlobalFree(memory);
                                }
                            }
                        }
                    }
                    CloseClipboard();
                }
            }
            return TRUE;
        default:
            return FALSE;
        }
    default:
        return FALSE;
    }
}

static void error_handler_impl(const char* expression, const char* message) {
#ifdef KW_DEBUG
    char* stacktrace = DebugUtils::get_stacktrace(2); // Hide `error_handler_impl` and `error_handler` calls.
    if (stacktrace != nullptr) {
        int length = snprintf(nullptr, 0, "Expression: %s\r\nMessage: %s\r\nStacktrace:\r\n%s", expression, message, stacktrace);
        if (length >= 0) {
            char* buffer = static_cast<char*>(malloc(static_cast<size_t>(length) + 1));
            if (buffer != nullptr) {
                if (snprintf(buffer, static_cast<size_t>(length) + 1, "Expression: %s\r\nMessage: %s\r\nStacktrace:\r\n%s", expression, message, stacktrace) > 0) {
                    DialogBoxParam(nullptr, MAKEINTRESOURCE(IDD_ERROR), nullptr, dialog_callback, reinterpret_cast<LPARAM>(buffer));
                }
                free(buffer);
            }
        }
        free(stacktrace);
    }
#else
    DialogBoxParam(nullptr, MAKEINTRESOURCE(IDD_ERROR), nullptr, dialog_callback, reinterpret_cast<LPARAM>(message));
#endif // KW_DEBUG
}

void error_handler(const char* expression) {
    error_handler_impl(expression, "Runtime Error");
}

void error_handler(const char* expression, const char* format, ...) {
    va_list args;
    va_start(args, format);

    int length = vsnprintf(nullptr, 0, format, args);
    if (length >= 0) {
        char* buffer = static_cast<char*>(malloc(static_cast<size_t>(length) + 1));
        if (buffer != nullptr) {
            if (vsnprintf(buffer, static_cast<size_t>(length) + 1, format, args) > 0) {
                error_handler_impl(expression, buffer);
            }
            free(buffer);
        }
    }

    va_end(args);
}

} // namespace kw::error_details
