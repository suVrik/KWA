#include "core/debug/debug_utils.h"
#include "core/resource.h"

#include <csignal>
#include <cstdio>
#include <mutex>

// Wingows.h must be included before DbgHelp.h
#include <Windows.h>
#include <DbgHelp.h>

namespace kw::DebugUtils {

#ifdef KW_DEBUG

constexpr size_t STACKTRACE_LENGTH = 4096;
constexpr size_t MAX_NAME_LENGTH   = 255;

char* get_stacktrace(uint32_t hide_calls) {
    // `SymInitialize` crashes when called from multiple threads.
    static std::mutex stacktrace_mutex;
    std::lock_guard lock(stacktrace_mutex);

    HANDLE process = GetCurrentProcess();
    HANDLE thread  = GetCurrentThread();

    CONTEXT context{};
    context.ContextFlags = CONTEXT_FULL;
    RtlCaptureContext(&context);

    SymInitialize(process, nullptr, TRUE);
    SymSetOptions(SYMOPT_LOAD_LINES);

    STACKFRAME64 frame{};
    frame.AddrPC.Offset    = context.Rip;
    frame.AddrPC.Mode      = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrFrame.Mode   = AddrModeFlat;
    frame.AddrStack.Offset = context.Rsp;
    frame.AddrStack.Mode   = AddrModeFlat;

    void* symbol_buffer = malloc(sizeof(IMAGEHLP_SYMBOL64) + MAX_NAME_LENGTH);

    char* stacktrace_begin = static_cast<char*>(malloc(STACKTRACE_LENGTH));
    char* stacktrace_end = stacktrace_begin + STACKTRACE_LENGTH;
    char* stacktrace = stacktrace_begin;

    hide_calls++; // Hide `get_stacktrace` call in the stacktrace.

    while (StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, &frame, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {
        if (hide_calls > 0) {
            hide_calls--;
            continue;
        }

        PCCH symbol_name   = "";
        PCCH file_name     = "";
        DWORD line_number  = 0;
        DWORD displacement = 0;

        IMAGEHLP_SYMBOL64* symbol = reinterpret_cast<IMAGEHLP_SYMBOL64*>(symbol_buffer);
        symbol->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL64) + MAX_NAME_LENGTH;
        symbol->MaxNameLength = MAX_NAME_LENGTH;

        if (SymGetSymFromAddr64(process, frame.AddrPC.Offset, nullptr, symbol)) {
            symbol_name = symbol->Name;
        }

        IMAGEHLP_LINE64 line{};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &displacement, &line)) {
            file_name   = line.FileName;
            line_number = line.LineNumber;
        }

        // Avoid some weird looking stacktrace lines like `:0 (in BaseThreadInitThunk)` or `:0 (in RtlUserThreadStart)`.
        if (line_number > 0) {
            int length = snprintf(stacktrace, stacktrace_end - stacktrace, "%s:%d (in %s)\r\n", file_name, line_number, symbol_name);
            if (length >= 0) {
                if (length >= stacktrace_end - stacktrace) {
                    size_t offset = stacktrace - stacktrace_begin;
                    size_t new_size = (stacktrace_end - stacktrace_begin) * 2;

                    stacktrace_begin = static_cast<char*>(realloc(stacktrace_begin, new_size));
                    stacktrace_end = stacktrace_begin + new_size;
                    stacktrace = stacktrace_begin + offset;
                }

                stacktrace += length;
            }
        }
    }

    free(symbol_buffer);

    SymCleanup(process);

    // `stacktrace_begin` will be freed outside.
    return stacktrace_begin;
}

constexpr INT_PTR BREAK        = 0;
constexpr INT_PTR SKIP         = 1;
constexpr INT_PTR SKIP_FOREVER = 2;

static INT_PTR CALLBACK dialog_callback(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_INITDIALOG:
        MessageBeep(MB_ICONERROR);
        SetWindowTextA(GetDlgItem(hwnd, IDC_ASSERT_MESSAGE), reinterpret_cast<char*>(lparam));
        return TRUE;
    case WM_COMMAND:
        switch (wparam) {
        case IDC_ASSERT_BREAK:
            EndDialog(hwnd, BREAK);
            return TRUE;
        case IDC_ASSERT_SKIP:
            EndDialog(hwnd, SKIP);
            return TRUE;
        case IDC_ASSERT_SKIP_FOREVER:
            EndDialog(hwnd, SKIP_FOREVER);
            return TRUE;
        case IDC_ASSERT_COPY_MESSAGE:
            if (IsClipboardFormatAvailable(CF_TEXT)) {
                if (OpenClipboard(nullptr)) {
                    if (EmptyClipboard()) {
                        int length = GetWindowTextLengthA(GetDlgItem(hwnd, IDC_ASSERT_MESSAGE));
                        if (length > 0) {
                            HGLOBAL memory = GlobalAlloc(GHND, static_cast<size_t>(length) + 1);
                            if (memory != nullptr) {
                                LPVOID text = GlobalLock(memory);
                                if (text != nullptr) {
                                    GetWindowTextA(GetDlgItem(hwnd, IDC_ASSERT_MESSAGE), static_cast<LPSTR>(text), length + 1);

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

bool show_assert_window(const char* message, bool* skip, uint32_t hide_calls) {
    bool result = true;

    char* stacktrace = DebugUtils::get_stacktrace(hide_calls + 1); // Hide `show_assert_window` call.
    if (stacktrace != nullptr) {
        int length = snprintf(nullptr, 0, "%s\r\nStacktrace:\r\n%s", message, stacktrace);
        if (length >= 0) {
            char* buffer = static_cast<char*>(malloc(static_cast<size_t>(length) + 1));
            if (buffer != nullptr) {
                if (snprintf(buffer, static_cast<size_t>(length) + 1, "%s\r\nStacktrace:\r\n%s", message, stacktrace) >= 0) {
                    switch (DialogBoxParam(nullptr, MAKEINTRESOURCE(IDD_ASSERT), nullptr, dialog_callback, reinterpret_cast<LPARAM>(buffer))) {
                    case BREAK:
                        result = true;
                        break;
                    case SKIP_FOREVER:
                        if (skip != nullptr) {
                            *skip = true;
                        }
                        [[fallthrough]];
                    case SKIP:
                    default:
                        result = false;
                    }
                }
                free(buffer);
            }
        }
        free(stacktrace);
    }

    return result;
}

static void signal_handler(int signal) {
    show_assert_window("SIGSEGV", nullptr, 1); // Hide `signal_handler` call.
    __debugbreak();
}

void subscribe_to_segfault() {
    signal(SIGSEGV, signal_handler);
}

#else

char* get_stacktrace(uint32_t hide_calls) {
    return "";
}

bool show_assert_window(const char* message, bool* skip, uint32_t hide_calls) {
    return false;
}

void subscribe_to_segfault() {
}

#endif // KW_DEBUG

} // namespace kw::DebugUtils
