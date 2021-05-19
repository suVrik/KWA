#include "core/error.h"
#include "core/debug/debug_utils.h"
#include "core/resource.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <Windows.h>

namespace kw::error_details {

#ifdef KW_DEBUG
static char message_buffer[1024];
static char dialog_buffer[4096];
#else
static char message_buffer[1024];
static char* dialog_buffer = message_buffer;
#endif

static INT_PTR CALLBACK dialog_callback(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_INITDIALOG:
        MessageBeep(MB_ICONERROR);
        SetWindowTextA(GetDlgItem(hwnd, IDC_ERROR_MESSAGE), dialog_buffer);
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
                        size_t length = strnlen(dialog_buffer, sizeof(dialog_buffer));
                        // Extra byte for null terminator.
                        HGLOBAL memory = GlobalAlloc(GHND, length + 1);
                        if (memory != nullptr) {
                            LPVOID text = GlobalLock(memory);
                            if (text != nullptr) {
                                memcpy_s(text, length, dialog_buffer, length);
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

static void error_handler_impl(const char* expression) {
#ifdef KW_DEBUG
    const char* stacktrace = DebugUtils::get_stacktrace(2); // Hide `error_handler_impl` and `error_handler` calls.
    snprintf(dialog_buffer, sizeof(dialog_buffer), "Expression: %s\r\nMessage: %s\r\nStacktrace:\r\n%s", expression, message_buffer, stacktrace);
#endif

    DialogBox(nullptr, MAKEINTRESOURCE(IDD_ERROR), nullptr, dialog_callback);
}

void error_handler(const char* expression) {
    strcpy_s(message_buffer, "Runtime Error");

    error_handler_impl(expression);
}

void error_handler(const char* expression, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(message_buffer, sizeof(message_buffer), format, args);
    va_end(args);

    error_handler_impl(expression);
}

} // namespace kw::error_details
