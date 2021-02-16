#include "debug/debug_utils.h"
#include "debug/resource.h"

#include <csignal>
#include <cstdio>

// Wingows.h must be included before DbgHelp.h
#include <Windows.h>
#include <DbgHelp.h>

namespace kw::DebugUtils {

#ifdef KW_DEBUG

constexpr size_t MAX_NAME_LENGTH = 255;

static char stacktrace_buffer[4096];
static char symbol_buffer[sizeof(IMAGEHLP_SYMBOL64) + MAX_NAME_LENGTH];

const char* get_stacktrace(uint32_t hide_calls) {
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

    char* current_stacktrace = stacktrace_buffer;

    hide_calls++; // Hide `get_stacktrace` call in the stacktrace too.
    while (StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, &frame, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {
        if (hide_calls-- > 0) {
            continue;
        }

        PCCH symbol_name   = "";
        PCCH file_name     = "";
        DWORD line_number  = 0;
        DWORD displacement = 0;

        auto* symbol = reinterpret_cast<IMAGEHLP_SYMBOL64*>(symbol_buffer);
        symbol->SizeOfStruct  = sizeof(symbol_buffer);
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

        // Avoid some weird looking stacktrace lines such as `:0 (in BaseThreadInitThunk)` or `:0 (in RtlUserThreadStart)`.
        if (line_number > 0) {
            current_stacktrace += snprintf(current_stacktrace, sizeof(stacktrace_buffer) - (current_stacktrace - stacktrace_buffer), "%s:%d (in %s)\r\n", file_name, line_number, symbol_name);
        }
    }

    SymCleanup(process);

    return stacktrace_buffer;
}

constexpr INT_PTR BREAK = 0;
constexpr INT_PTR SKIP = 1;
constexpr INT_PTR SKIP_FOREVER = 2;

static char dialog_buffer[4096];

static INT_PTR CALLBACK dialog_callback(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_INITDIALOG:
        MessageBeep(MB_ICONERROR);
        SetWindowTextA(GetDlgItem(hwnd, IDC_MESSAGE), dialog_buffer);
        return TRUE;
    case WM_COMMAND:
        switch (wparam) {
        case IDC_BREAK:
            EndDialog(hwnd, BREAK);
            return TRUE;
        case IDC_SKIP:
            EndDialog(hwnd, SKIP);
            return TRUE;
        case IDC_SKIP_FOREVER:
            EndDialog(hwnd, SKIP_FOREVER);
            return TRUE;
        case IDC_COPY_MESSAGE:
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

bool show_assert_window(const char* message, bool* skip, uint32_t hide_calls) {
    // Hide `show_assert_window` calls in stacktrace too.
    const char* stacktrace = DebugUtils::get_stacktrace(hide_calls + 1);

    snprintf(dialog_buffer, sizeof(dialog_buffer), "%s\r\nStacktrace:\r\n%s", message, stacktrace);

    switch (DialogBox(nullptr, MAKEINTRESOURCE(IDD_ASSERT), nullptr, dialog_callback)) {
    case BREAK:
        return true;
    case SKIP_FOREVER:
        if (skip != nullptr) {
            *skip = true;
        }
        [[fallthrough]];
    case SKIP:
    default:
        return false;
    }
}

static void signal_handler(int signal) {
    show_assert_window("SIGSEGV", nullptr, 1); // Hide `signal_handler` call.
    __debugbreak();
}

void subscribe_to_segfault() {
    signal(SIGSEGV, signal_handler);
}

#else

const char* get_stacktrace(uint32_t hide_calls) {
    return "";
}

bool show_assert_window(const char* message, bool* skip, uint32_t hide_calls) {
    return false;
}

void subscribe_to_segfault() {
}

#endif

} // namespace kw::DebugUtils
