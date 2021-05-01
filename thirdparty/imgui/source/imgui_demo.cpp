// dear imgui, v1.82
// (demo code)

// Help:
// - Read FAQ at http://dearimgui.org/faq
// - Newcomers, read 'Programmer guide' in imgui.cpp for notes on how to setup Dear ImGui in your codebase.
// - Call and read ImGui::ShowDemoWindow() in imgui_demo.cpp. All applications in examples/ are doing that.
// Read imgui.cpp for more details, documentation and comments.
// Get latest version at https://github.com/ocornut/imgui

// Message to the person tempted to delete this file when integrating Dear ImGui into their code base:
// Do NOT remove this file from your project! Think again! It is the most useful reference code that you and other
// coders will want to refer to and call. Have the ImGui::ShowDemoWindow() function wired in an always-available
// debug menu of your game/app! Removing this file from your project is hindering access to documentation for everyone
// in your team, likely leading you to poorer usage of the library.
// Everything in this file will be stripped out by the linker if you don't call ImGui::ShowDemoWindow().
// If you want to link core Dear ImGui in your shipped builds but want a thorough guarantee that the demo will not be
// linked, you can setup your imconfig.h with #define IMGUI_DISABLE_DEMO_WINDOWS and those functions will be empty.
// In other situation, whenever you have Dear ImGui available you probably want this to be available for reference.
// Thank you,
// -Your beloved friend, imgui_demo.cpp (which you won't delete)

// Message to beginner C/C++ programmers about the meaning of the 'static' keyword:
// In this demo code, we frequently we use 'static' variables inside functions. A static variable persist across calls,
// so it is essentially like a global variable but declared inside the scope of the function. We do this as a way to
// gather code and data in the same place, to make the demo source code faster to read, faster to write, and smaller
// in size. It also happens to be a convenient way of storing simple UI related information as long as your function
// doesn't need to be reentrant or used in multiple threads. This might be a pattern you will want to use in your code,
// but most of the real data you would be editing is likely going to be stored outside your functions.

// The Demo code in this file is designed to be easy to copy-and-paste in into your application!
// Because of this:
// - We never omit the ImGui:: prefix when calling functions, even though most code here is in the same namespace.
// - We try to declare static variables in the local scope, as close as possible to the code using them.
// - We never use any of the helpers/facilities used internally by Dear ImGui, unless available in the public API.
// - We never use maths operators on ImVec2/ImVec4. For our other sources files we use them, and they are provided
//   by imgui_internal.h using the IMGUI_DEFINE_MATH_OPERATORS define. For your own sources file they are optional
//   and require you either enable those, either provide your own via IM_VEC2_CLASS_EXTRA in imconfig.h.
//   Because we can't assume anything about your support of maths operators, we cannot use them in imgui_demo.cpp.

// Navigating this file:
// - In Visual Studio IDE: CTRL+comma ("Edit.NavigateTo") can follow symbols in comments, whereas CTRL+F12 ("Edit.GoToImplementation") cannot.
// - With Visual Assist installed: ALT+G ("VAssistX.GoToImplementation") can also follow symbols in comments.

/*

Index of this file:

// [SECTION] Forward Declarations, Helpers
// [SECTION] Demo Window / ShowDemoWindow()
// - sub section: ShowDemoWindowWidgets()
// - sub section: ShowDemoWindowLayout()
// - sub section: ShowDemoWindowPopups()
// - sub section: ShowDemoWindowTables()
// - sub section: ShowDemoWindowMisc()
// [SECTION] About Window / ShowAboutWindow()
// [SECTION] Style Editor / ShowStyleEditor()
// [SECTION] Example App: Main Menu Bar / ShowExampleAppMainMenuBar()
// [SECTION] Example App: Debug Console / ShowExampleAppConsole()
// [SECTION] Example App: Debug Log / ShowExampleAppLog()
// [SECTION] Example App: Simple Layout / ShowExampleAppLayout()
// [SECTION] Example App: Property Editor / ShowExampleAppPropertyEditor()
// [SECTION] Example App: Long Text / ShowExampleAppLongText()
// [SECTION] Example App: Auto Resize / ShowExampleAppAutoResize()
// [SECTION] Example App: Constrained Resize / ShowExampleAppConstrainedResize()
// [SECTION] Example App: Simple overlay / ShowExampleAppSimpleOverlay()
// [SECTION] Example App: Fullscreen window / ShowExampleAppFullscreen()
// [SECTION] Example App: Manipulating window titles / ShowExampleAppWindowTitles()
// [SECTION] Example App: Custom Rendering using ImDrawList API / ShowExampleAppCustomRendering()
// [SECTION] Example App: Documents Handling / ShowExampleAppDocuments()

*/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#ifndef IMGUI_DISABLE

// System includes
#include <ctype.h>          // toupper
#include <limits.h>         // INT_MIN, INT_MAX
#include <math.h>           // sqrtf, powf, cosf, sinf, floorf, ceilf
#include <stdio.h>          // vsnprintf, sscanf, printf
#include <stdlib.h>         // NULL, malloc, free, atoi
#if defined(_MSC_VER) && _MSC_VER <= 1500 // MSVC 2008 or earlier
#include <stddef.h>         // intptr_t
#else
#include <stdint.h>         // intptr_t
#endif

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4996) // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#if __has_warning("-Wunknown-warning-option")
#pragma clang diagnostic ignored "-Wunknown-warning-option"         // warning: unknown warning group 'xxx'                     // not all warnings are known by all Clang versions and they tend to be rename-happy.. so ignoring warnings triggers new warnings on some configuration. Great!
#endif
#pragma clang diagnostic ignored "-Wunknown-pragmas"                // warning: unknown warning group 'xxx'
#pragma clang diagnostic ignored "-Wold-style-cast"                 // warning: use of old-style cast                           // yes, they are more terse.
#pragma clang diagnostic ignored "-Wdeprecated-declarations"        // warning: 'xx' is deprecated: The POSIX name for this..   // for strdup used in demo code (so user can copy & paste the code)
#pragma clang diagnostic ignored "-Wint-to-void-pointer-cast"       // warning: cast to 'void *' from smaller integer type
#pragma clang diagnostic ignored "-Wformat-security"                // warning: format string is not a string literal
#pragma clang diagnostic ignored "-Wexit-time-destructors"          // warning: declaration requires an exit-time destructor    // exit-time destruction order is undefined. if MemFree() leads to users code that has been disabled before exit it might cause problems. ImGui coding style welcomes static/globals.
#pragma clang diagnostic ignored "-Wunused-macros"                  // warning: macro is not used                               // we define snprintf/vsnprintf on Windows so they are available, but not always used.
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning: zero as null pointer constant                   // some standard header variations use #define NULL 0
#pragma clang diagnostic ignored "-Wdouble-promotion"               // warning: implicit conversion from 'float' to 'double' when passing argument to function  // using printf() is a misery with this as C++ va_arg ellipsis changes float to double.
#pragma clang diagnostic ignored "-Wreserved-id-macro"              // warning: macro name is a reserved identifier
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"  // warning: implicit conversion from 'xxx' to 'float' may lose precision
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpragmas"                  // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"      // warning: cast to pointer from integer of different size
#pragma GCC diagnostic ignored "-Wformat-security"          // warning: format string is not a string literal (potentially insecure)
#pragma GCC diagnostic ignored "-Wdouble-promotion"         // warning: implicit conversion from 'float' to 'double' when passing argument to function
#pragma GCC diagnostic ignored "-Wconversion"               // warning: conversion to 'xxxx' from 'xxxx' may alter its value
#pragma GCC diagnostic ignored "-Wmisleading-indentation"   // [__GNUC__ >= 6] warning: this 'if' clause does not guard this statement      // GCC 6.0+ only. See #883 on GitHub.
#endif

// Play it nice with Windows users (Update: May 2018, Notepad now supports Unix-style carriage returns!)
#ifdef _WIN32
#define IM_NEWLINE  "\r\n"
#else
#define IM_NEWLINE  "\n"
#endif

// Helpers
#if defined(_MSC_VER) && !defined(snprintf)
#define snprintf    _snprintf
#endif
#if defined(_MSC_VER) && !defined(vsnprintf)
#define vsnprintf   _vsnprintf
#endif

// Format specifiers, printing 64-bit hasn't been decently standardized...
// In a real application you should be using PRId64 and PRIu64 from <inttypes.h> (non-windows) and on Windows define them yourself.
#ifdef _MSC_VER
#define IM_PRId64   "I64d"
#define IM_PRIu64   "I64u"
#else
#define IM_PRId64   "lld"
#define IM_PRIu64   "llu"
#endif

// Helpers macros
// We normally try to not use many helpers in imgui_demo.cpp in order to make code easier to copy and paste,
// but making an exception here as those are largely simplifying code...
// In other imgui sources we can use nicer internal functions from imgui_internal.h (ImMin/ImMax) but not in the demo.
#define IM_MIN(A, B)            (((A) < (B)) ? (A) : (B))
#define IM_MAX(A, B)            (((A) >= (B)) ? (A) : (B))
#define IM_CLAMP(V, MN, MX)     ((V) < (MN) ? (MN) : (V) > (MX) ? (MX) : (V))

// Enforce cdecl calling convention for functions called by the standard library, in case compilation settings changed the default to e.g. __vectorcall
#ifndef IMGUI_CDECL
#ifdef _MSC_VER
#define IMGUI_CDECL __cdecl
#else
#define IMGUI_CDECL
#endif
#endif

//-----------------------------------------------------------------------------
// [SECTION] Forward Declarations, Helpers
//-----------------------------------------------------------------------------

#if !defined(IMGUI_DISABLE_DEMO_WINDOWS)

// Forward Declarations
static void ShowExampleAppDocuments(ImGui& imgui_, bool* p_open);
static void ShowExampleAppMainMenuBar(ImGui& imgui_);
static void ShowExampleAppConsole(ImGui& imgui_, bool* p_open);
static void ShowExampleAppLog(ImGui& imgui_, bool* p_open);
static void ShowExampleAppLayout(ImGui& imgui_, bool* p_open);
static void ShowExampleAppPropertyEditor(ImGui& imgui_, bool* p_open);
static void ShowExampleAppLongText(ImGui& imgui_, bool* p_open);
static void ShowExampleAppAutoResize(ImGui& imgui_, bool* p_open);
static void ShowExampleAppConstrainedResize(ImGui& imgui_, bool* p_open);
static void ShowExampleAppSimpleOverlay(ImGui& imgui_, bool* p_open);
static void ShowExampleAppFullscreen(ImGui& imgui_, bool* p_open);
static void ShowExampleAppWindowTitles(ImGui& imgui_, bool* p_open);
static void ShowExampleAppCustomRendering(ImGui& imgui_, bool* p_open);
static void ShowExampleMenuFile(ImGui& imgui_);

// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a merged icon fonts (see docs/FONTS.md)
static void HelpMarker(ImGui& imgui_, const char* desc)
{
    imgui_.TextDisabled("(?)");
    if (imgui_.IsItemHovered())
    {
        imgui_.BeginTooltip();
        imgui_.PushTextWrapPos(imgui_.GetFontSize() * 35.0f);
        imgui_.TextUnformatted(desc);
        imgui_.PopTextWrapPos();
        imgui_.EndTooltip();
    }
}

// Helper to display basic user controls.
void ImGui::ShowUserGuide()
{
    ImGuiIO& io = GetIO();
    BulletText("Double-click on title bar to collapse window.");
    BulletText(
        "Click and drag on lower corner to resize window\n"
        "(double-click to auto fit window to its contents).");
    BulletText("CTRL+Click on a slider or drag box to input value as text.");
    BulletText("TAB/SHIFT+TAB to cycle through keyboard editable fields.");
    if (io.FontAllowUserScaling)
        BulletText("CTRL+Mouse Wheel to zoom window contents.");
    BulletText("While inputing text:\n");
    Indent();
    BulletText("CTRL+Left/Right to word jump.");
    BulletText("CTRL+A or double-click to select all.");
    BulletText("CTRL+X/C/V to use clipboard cut/copy/paste.");
    BulletText("CTRL+Z,CTRL+Y to undo/redo.");
    BulletText("ESCAPE to revert.");
    BulletText("You can apply arithmetic operators +,*,/ on numerical values.\nUse +- to subtract.");
    Unindent();
    BulletText("With keyboard navigation enabled:");
    Indent();
    BulletText("Arrow keys to navigate.");
    BulletText("Space to activate a widget.");
    BulletText("Return to input text into a widget.");
    BulletText("Escape to deactivate a widget, close popup, exit child window.");
    BulletText("Alt to jump to the menu layer of a window.");
    BulletText("CTRL+Tab to select a window.");
    Unindent();
}

//-----------------------------------------------------------------------------
// [SECTION] Demo Window / ShowDemoWindow()
//-----------------------------------------------------------------------------
// - ShowDemoWindowWidgets()
// - ShowDemoWindowLayout()
// - ShowDemoWindowPopups()
// - ShowDemoWindowTables()
// - ShowDemoWindowColumns()
// - ShowDemoWindowMisc()
//-----------------------------------------------------------------------------

// We split the contents of the big ShowDemoWindow() function into smaller functions
// (because the link time of very large functions grow non-linearly)
static void ShowDemoWindowWidgets(ImGui& imgui_);
static void ShowDemoWindowLayout(ImGui& imgui_);
static void ShowDemoWindowPopups(ImGui& imgui_);
static void ShowDemoWindowTables(ImGui& imgui_);
static void ShowDemoWindowColumns(ImGui& imgui_);
static void ShowDemoWindowMisc(ImGui& imgui_);

// Demonstrate most Dear ImGui features (this is big function!)
// You may execute this function to experiment with the UI and understand what it does.
// You may then search for keywords in the code when you are interested by a specific feature.
void ImGui::ShowDemoWindow(bool* p_open)
{
    // Examples Apps (accessible from the "Examples" menu)
    static bool show_app_main_menu_bar = false;
    static bool show_app_documents = false;

    static bool show_app_console = false;
    static bool show_app_log = false;
    static bool show_app_layout = false;
    static bool show_app_property_editor = false;
    static bool show_app_long_text = false;
    static bool show_app_auto_resize = false;
    static bool show_app_constrained_resize = false;
    static bool show_app_simple_overlay = false;
    static bool show_app_fullscreen = false;
    static bool show_app_window_titles = false;
    static bool show_app_custom_rendering = false;

    if (show_app_main_menu_bar)       ShowExampleAppMainMenuBar(*this);
    if (show_app_documents)           ShowExampleAppDocuments(*this, &show_app_documents);

    if (show_app_console)             ShowExampleAppConsole(*this, &show_app_console);
    if (show_app_log)                 ShowExampleAppLog(*this, &show_app_log);
    if (show_app_layout)              ShowExampleAppLayout(*this, &show_app_layout);
    if (show_app_property_editor)     ShowExampleAppPropertyEditor(*this, &show_app_property_editor);
    if (show_app_long_text)           ShowExampleAppLongText(*this, &show_app_long_text);
    if (show_app_auto_resize)         ShowExampleAppAutoResize(*this, &show_app_auto_resize);
    if (show_app_constrained_resize)  ShowExampleAppConstrainedResize(*this, &show_app_constrained_resize);
    if (show_app_simple_overlay)      ShowExampleAppSimpleOverlay(*this, &show_app_simple_overlay);
    if (show_app_fullscreen)          ShowExampleAppFullscreen(*this, &show_app_fullscreen);
    if (show_app_window_titles)       ShowExampleAppWindowTitles(*this, &show_app_window_titles);
    if (show_app_custom_rendering)    ShowExampleAppCustomRendering(*this, &show_app_custom_rendering);

    // Dear ImGui Apps (accessible from the "Tools" menu)
    static bool show_app_metrics = false;
    static bool show_app_style_editor = false;
    static bool show_app_about = false;

    if (show_app_metrics)       { ShowMetricsWindow(&show_app_metrics); }
    if (show_app_about)         { ShowAboutWindow(&show_app_about); }
    if (show_app_style_editor)
    {
        Begin("Dear ImGui Style Editor", &show_app_style_editor);
        ShowStyleEditor();
        End();
    }

    // Demonstrate the various window flags. Typically you would just use the default!
    static bool no_titlebar = false;
    static bool no_scrollbar = false;
    static bool no_menu = false;
    static bool no_move = false;
    static bool no_resize = false;
    static bool no_collapse = false;
    static bool no_close = false;
    static bool no_nav = false;
    static bool no_background = false;
    static bool no_bring_to_front = false;

    ImGuiWindowFlags window_flags = 0;
    if (no_titlebar)        window_flags |= ImGuiWindowFlags_NoTitleBar;
    if (no_scrollbar)       window_flags |= ImGuiWindowFlags_NoScrollbar;
    if (!no_menu)           window_flags |= ImGuiWindowFlags_MenuBar;
    if (no_move)            window_flags |= ImGuiWindowFlags_NoMove;
    if (no_resize)          window_flags |= ImGuiWindowFlags_NoResize;
    if (no_collapse)        window_flags |= ImGuiWindowFlags_NoCollapse;
    if (no_nav)             window_flags |= ImGuiWindowFlags_NoNav;
    if (no_background)      window_flags |= ImGuiWindowFlags_NoBackground;
    if (no_bring_to_front)  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (no_close)           p_open = NULL; // Don't pass our bool* to Begin

    // We specify a default position/size in case there's no data in the .ini file.
    // We only do it to make the demo applications a little more welcoming, but typically this isn't required.
    const ImGuiViewport* main_viewport = GetMainViewport();
    SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 650, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);
    SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);

    // Main body of the Demo window starts here.
    if (!Begin("Dear ImGui Demo", p_open, window_flags))
    {
        // Early out if the window is collapsed, as an optimization.
        End();
        return;
    }

    // Most "big" widgets share a common width settings by default. See 'Demo->Layout->Widgets Width' for details.

    // e.g. Use 2/3 of the space for widgets and 1/3 for labels (right align)
    //PushItemWidth(-GetWindowWidth() * 0.35f);

    // e.g. Leave a fixed amount of width for labels (by passing a negative value), the rest goes to widgets.
    PushItemWidth(GetFontSize() * -12);

    // Menu Bar
    if (BeginMenuBar())
    {
        if (BeginMenu("Menu"))
        {
            ShowExampleMenuFile(*this);
            EndMenu();
        }
        if (BeginMenu("Examples"))
        {
            MenuItem("Main menu bar", NULL, &show_app_main_menu_bar);
            MenuItem("Console", NULL, &show_app_console);
            MenuItem("Log", NULL, &show_app_log);
            MenuItem("Simple layout", NULL, &show_app_layout);
            MenuItem("Property editor", NULL, &show_app_property_editor);
            MenuItem("Long text display", NULL, &show_app_long_text);
            MenuItem("Auto-resizing window", NULL, &show_app_auto_resize);
            MenuItem("Constrained-resizing window", NULL, &show_app_constrained_resize);
            MenuItem("Simple overlay", NULL, &show_app_simple_overlay);
            MenuItem("Fullscreen window", NULL, &show_app_fullscreen);
            MenuItem("Manipulating window titles", NULL, &show_app_window_titles);
            MenuItem("Custom rendering", NULL, &show_app_custom_rendering);
            MenuItem("Documents", NULL, &show_app_documents);
            EndMenu();
        }
        if (BeginMenu("Tools"))
        {
            MenuItem("Metrics/Debugger", NULL, &show_app_metrics);
            MenuItem("Style Editor", NULL, &show_app_style_editor);
            MenuItem("About Dear ImGui", NULL, &show_app_about);
            EndMenu();
        }
        EndMenuBar();
    }

    Text("dear imgui says hello. (%s)", IMGUI_VERSION);
    Spacing();

    if (CollapsingHeader("Help"))
    {
        Text("ABOUT THIS DEMO:");
        BulletText("Sections below are demonstrating many aspects of the library.");
        BulletText("The \"Examples\" menu above leads to more demo contents.");
        BulletText("The \"Tools\" menu above gives access to: About Box, Style Editor,\n"
                          "and Metrics/Debugger (general purpose Dear ImGui debugging tool).");
        Separator();

        Text("PROGRAMMER GUIDE:");
        BulletText("See the ShowDemoWindow() code in imgui_demo.cpp. <- you are here!");
        BulletText("See comments in imgui.cpp.");
        BulletText("See example applications in the examples/ folder.");
        BulletText("Read the FAQ at http://www.dearimgui.org/faq/");
        BulletText("Set 'io.ConfigFlags |= NavEnableKeyboard' for keyboard controls.");
        BulletText("Set 'io.ConfigFlags |= NavEnableGamepad' for gamepad controls.");
        Separator();

        Text("USER GUIDE:");
        ShowUserGuide();
    }

    if (CollapsingHeader("Configuration"))
    {
        ImGuiIO& io = GetIO();

        if (TreeNode("Configuration##2"))
        {
            CheckboxFlags("io.ConfigFlags: NavEnableKeyboard",    &io.ConfigFlags, ImGuiConfigFlags_NavEnableKeyboard);
            SameLine(); HelpMarker(*this, "Enable keyboard controls.");
            CheckboxFlags("io.ConfigFlags: NavEnableGamepad",     &io.ConfigFlags, ImGuiConfigFlags_NavEnableGamepad);
            SameLine(); HelpMarker(*this, "Enable gamepad controls. Require backend to set io.BackendFlags |= ImGuiBackendFlags_HasGamepad.\n\nRead instructions in imgui.cpp for details.");
            CheckboxFlags("io.ConfigFlags: NavEnableSetMousePos", &io.ConfigFlags, ImGuiConfigFlags_NavEnableSetMousePos);
            SameLine(); HelpMarker(*this, "Instruct navigation to move the mouse cursor. See comment for ImGuiConfigFlags_NavEnableSetMousePos.");
            CheckboxFlags("io.ConfigFlags: NoMouse",              &io.ConfigFlags, ImGuiConfigFlags_NoMouse);
            if (io.ConfigFlags & ImGuiConfigFlags_NoMouse)
            {
                // The "NoMouse" option can get us stuck with a disabled mouse! Let's provide an alternative way to fix it:
                if (fmodf((float)GetTime(), 0.40f) < 0.20f)
                {
                    SameLine();
                    Text("<<PRESS SPACE TO DISABLE>>");
                }
                if (IsKeyPressed(GetKeyIndex(ImGuiKey_Space)))
                    io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
            }
            CheckboxFlags("io.ConfigFlags: NoMouseCursorChange", &io.ConfigFlags, ImGuiConfigFlags_NoMouseCursorChange);
            SameLine(); HelpMarker(*this, "Instruct backend to not alter mouse cursor shape and visibility.");
            Checkbox("io.ConfigInputTextCursorBlink", &io.ConfigInputTextCursorBlink);
            SameLine(); HelpMarker(*this, "Enable blinking cursor (optional as some users consider it to be distracting)");
            Checkbox("io.ConfigDragClickToInputText", &io.ConfigDragClickToInputText);
            SameLine(); HelpMarker(*this, "Enable turning DragXXX widgets into text input with a simple mouse click-release (without moving).");
            Checkbox("io.ConfigWindowsResizeFromEdges", &io.ConfigWindowsResizeFromEdges);
            SameLine(); HelpMarker(*this, "Enable resizing of windows from their edges and from the lower-left corner.\nThis requires (io.BackendFlags & ImGuiBackendFlags_HasMouseCursors) because it needs mouse cursor feedback.");
            Checkbox("io.ConfigWindowsMoveFromTitleBarOnly", &io.ConfigWindowsMoveFromTitleBarOnly);
            Checkbox("io.MouseDrawCursor", &io.MouseDrawCursor);
            SameLine(); HelpMarker(*this, "Instruct Dear ImGui to render a mouse cursor itself. Note that a mouse cursor rendered via your application GPU rendering path will feel more laggy than hardware cursor, but will be more in sync with your other visuals.\n\nSome desktop applications may use both kinds of cursors (e.g. enable software cursor only when resizing/dragging something).");
            Text("Also see Style->Rendering for rendering options.");
            TreePop();
            Separator();
        }

        if (TreeNode("Backend Flags"))
        {
            HelpMarker(*this, 
                "Those flags are set by the backends (imgui_impl_xxx files) to specify their capabilities.\n"
                "Here we expose then as read-only fields to avoid breaking interactions with your backend.");

            // Make a local copy to avoid modifying actual backend flags.
            ImGuiBackendFlags backend_flags = io.BackendFlags;
            CheckboxFlags("io.BackendFlags: HasGamepad",           &backend_flags, ImGuiBackendFlags_HasGamepad);
            CheckboxFlags("io.BackendFlags: HasMouseCursors",      &backend_flags, ImGuiBackendFlags_HasMouseCursors);
            CheckboxFlags("io.BackendFlags: HasSetMousePos",       &backend_flags, ImGuiBackendFlags_HasSetMousePos);
            CheckboxFlags("io.BackendFlags: RendererHasVtxOffset", &backend_flags, ImGuiBackendFlags_RendererHasVtxOffset);
            TreePop();
            Separator();
        }

        if (TreeNode("Style"))
        {
            HelpMarker(*this, "The same contents can be accessed in 'Tools->Style Editor' or by calling the ShowStyleEditor() function.");
            ShowStyleEditor();
            TreePop();
            Separator();
        }

        if (TreeNode("Capture/Logging"))
        {
            HelpMarker(*this, 
                "The logging API redirects all text output so you can easily capture the content of "
                "a window or a block. Tree nodes can be automatically expanded.\n"
                "Try opening any of the contents below in this window and then click one of the \"Log To\" button.");
            LogButtons();

            HelpMarker(*this, "You can also call LogText() to output directly to the log without a visual output.");
            if (Button("Copy \"Hello, world!\" to clipboard"))
            {
                LogToClipboard();
                LogText("Hello, world!");
                LogFinish();
            }
            TreePop();
        }
    }

    if (CollapsingHeader("Window options"))
    {
        if (BeginTable("split", 3))
        {
            TableNextColumn(); Checkbox("No titlebar", &no_titlebar);
            TableNextColumn(); Checkbox("No scrollbar", &no_scrollbar);
            TableNextColumn(); Checkbox("No menu", &no_menu);
            TableNextColumn(); Checkbox("No move", &no_move);
            TableNextColumn(); Checkbox("No resize", &no_resize);
            TableNextColumn(); Checkbox("No collapse", &no_collapse);
            TableNextColumn(); Checkbox("No close", &no_close);
            TableNextColumn(); Checkbox("No nav", &no_nav);
            TableNextColumn(); Checkbox("No background", &no_background);
            TableNextColumn(); Checkbox("No bring to front", &no_bring_to_front);
            EndTable();
        }
    }

    // All demo contents
    ShowDemoWindowWidgets(*this);
    ShowDemoWindowLayout(*this);
    ShowDemoWindowPopups(*this);
    ShowDemoWindowTables(*this);
    ShowDemoWindowMisc(*this);

    // End of ShowDemoWindow()
    PopItemWidth();
    End();
}

static void ShowDemoWindowWidgets(ImGui& imgui_)
{
    if (!imgui_.CollapsingHeader("Widgets"))
        return;

    if (imgui_.TreeNode("Basic"))
    {
        static int clicked = 0;
        if (imgui_.Button("Button"))
            clicked++;
        if (clicked & 1)
        {
            imgui_.SameLine();
            imgui_.Text("Thanks for clicking me!");
        }

        static bool check = true;
        imgui_.Checkbox("checkbox", &check);

        static int e = 0;
        imgui_.RadioButton("radio a", &e, 0); imgui_.SameLine();
        imgui_.RadioButton("radio b", &e, 1); imgui_.SameLine();
        imgui_.RadioButton("radio c", &e, 2);

        // Color buttons, demonstrate using PushID() to add unique identifier in the ID stack, and changing style.
        for (int i = 0; i < 7; i++)
        {
            if (i > 0)
                imgui_.SameLine();
            imgui_.PushID(i);
            imgui_.PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(i / 7.0f, 0.6f, 0.6f));
            imgui_.PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(i / 7.0f, 0.7f, 0.7f));
            imgui_.PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(i / 7.0f, 0.8f, 0.8f));
            imgui_.Button("Click");
            imgui_.PopStyleColor(3);
            imgui_.PopID();
        }

        // Use AlignTextToFramePadding() to align text baseline to the baseline of framed widgets elements
        // (otherwise a Text+SameLine+Button sequence will have the text a little too high by default!)
        // See 'Demo->Layout->Text Baseline Alignment' for details.
        imgui_.AlignTextToFramePadding();
        imgui_.Text("Hold to repeat:");
        imgui_.SameLine();

        // Arrow buttons with Repeater
        static int counter = 0;
        float spacing = imgui_.GetStyle().ItemInnerSpacing.x;
        imgui_.PushButtonRepeat(true);
        if (imgui_.ArrowButton("##left", ImGuiDir_Left)) { counter--; }
        imgui_.SameLine(0.0f, spacing);
        if (imgui_.ArrowButton("##right", ImGuiDir_Right)) { counter++; }
        imgui_.PopButtonRepeat();
        imgui_.SameLine();
        imgui_.Text("%d", counter);

        imgui_.Text("Hover over me");
        if (imgui_.IsItemHovered())
            imgui_.SetTooltip("I am a tooltip");

        imgui_.SameLine();
        imgui_.Text("- or me");
        if (imgui_.IsItemHovered())
        {
            imgui_.BeginTooltip();
            imgui_.Text("I am a fancy tooltip");
            static float arr[] = { 0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f };
            imgui_.PlotLines("Curve", arr, IM_ARRAYSIZE(arr));
            imgui_.EndTooltip();
        }

        imgui_.Separator();

        imgui_.LabelText("label", "Value");

        {
            // Using the _simplified_ one-liner Combo() api here
            // See "Combo" section for examples of how to use the more flexible BeginCombo()/EndCombo() api.
            const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIIIIII", "JJJJ", "KKKKKKK" };
            static int item_current = 0;
            imgui_.Combo("combo", &item_current, items, IM_ARRAYSIZE(items));
            imgui_.SameLine(); HelpMarker(imgui_, 
                "Using the simplified one-liner Combo API here.\nRefer to the \"Combo\" section below for an explanation of how to use the more flexible and general BeginCombo/EndCombo API.");
        }

        {
            // To wire InputText() with std::string or any other custom string type,
            // see the "Text Input > Resize Callback" section of this demo, and the misc/cpp/imgui_stdlib.h file.
            static char str0[128] = "Hello, world!";
            imgui_.InputText("input text", str0, IM_ARRAYSIZE(str0));
            imgui_.SameLine(); HelpMarker(imgui_, 
                "USER:\n"
                "Hold SHIFT or use mouse to select text.\n"
                "CTRL+Left/Right to word jump.\n"
                "CTRL+A or double-click to select all.\n"
                "CTRL+X,CTRL+C,CTRL+V clipboard.\n"
                "CTRL+Z,CTRL+Y undo/redo.\n"
                "ESCAPE to revert.\n\n"
                "PROGRAMMER:\n"
                "You can use the ImGuiInputTextFlags_CallbackResize facility if you need to wire InputText() "
                "to a dynamic string type. See misc/cpp/imgui_stdlib.h for an example (this is not demonstrated "
                "in imgui_demo.cpp).");

            static char str1[128] = "";
            imgui_.InputTextWithHint("input text (w/ hint)", "enter text here", str1, IM_ARRAYSIZE(str1));

            static int i0 = 123;
            imgui_.InputInt("input int", &i0);
            imgui_.SameLine(); HelpMarker(imgui_, 
                "You can apply arithmetic operators +,*,/ on numerical values.\n"
                "  e.g. [ 100 ], input \'*2\', result becomes [ 200 ]\n"
                "Use +- to subtract.");

            static float f0 = 0.001f;
            imgui_.InputFloat("input float", &f0, 0.01f, 1.0f, "%.3f");

            static double d0 = 999999.00000001;
            imgui_.InputDouble("input double", &d0, 0.01f, 1.0f, "%.8f");

            static float f1 = 1.e10f;
            imgui_.InputFloat("input scientific", &f1, 0.0f, 0.0f, "%e");
            imgui_.SameLine(); HelpMarker(imgui_, 
                "You can input value using the scientific notation,\n"
                "  e.g. \"1e+8\" becomes \"100000000\".");

            static float vec4a[4] = { 0.10f, 0.20f, 0.30f, 0.44f };
            imgui_.InputFloat3("input float3", vec4a);
        }

        {
            static int i1 = 50, i2 = 42;
            imgui_.DragInt("drag int", &i1, 1);
            imgui_.SameLine(); HelpMarker(imgui_, 
                "Click and drag to edit value.\n"
                "Hold SHIFT/ALT for faster/slower edit.\n"
                "Double-click or CTRL+click to input value.");

            imgui_.DragInt("drag int 0..100", &i2, 1, 0, 100, "%d%%", ImGuiSliderFlags_AlwaysClamp);

            static float f1 = 1.00f, f2 = 0.0067f;
            imgui_.DragFloat("drag float", &f1, 0.005f);
            imgui_.DragFloat("drag small float", &f2, 0.0001f, 0.0f, 0.0f, "%.06f ns");
        }

        {
            static int i1 = 0;
            imgui_.SliderInt("slider int", &i1, -1, 3);
            imgui_.SameLine(); HelpMarker(imgui_, "CTRL+click to input value.");

            static float f1 = 0.123f, f2 = 0.0f;
            imgui_.SliderFloat("slider float", &f1, 0.0f, 1.0f, "ratio = %.3f");
            imgui_.SliderFloat("slider float (log)", &f2, -10.0f, 10.0f, "%.4f", ImGuiSliderFlags_Logarithmic);

            static float angle = 0.0f;
            imgui_.SliderAngle("slider angle", &angle);

            // Using the format string to display a name instead of an integer.
            // Here we completely omit '%d' from the format string, so it'll only display a name.
            // This technique can also be used with DragInt().
            enum Element { Element_Fire, Element_Earth, Element_Air, Element_Water, Element_COUNT };
            static int elem = Element_Fire;
            const char* elems_names[Element_COUNT] = { "Fire", "Earth", "Air", "Water" };
            const char* elem_name = (elem >= 0 && elem < Element_COUNT) ? elems_names[elem] : "Unknown";
            imgui_.SliderInt("slider enum", &elem, 0, Element_COUNT - 1, elem_name);
            imgui_.SameLine(); HelpMarker(imgui_, "Using the format string parameter to display a name instead of the underlying integer.");
        }

        {
            static float col1[3] = { 1.0f, 0.0f, 0.2f };
            static float col2[4] = { 0.4f, 0.7f, 0.0f, 0.5f };
            imgui_.ColorEdit3("color 1", col1);
            imgui_.SameLine(); HelpMarker(imgui_, 
                "Click on the color square to open a color picker.\n"
                "Click and hold to use drag and drop.\n"
                "Right-click on the color square to show options.\n"
                "CTRL+click on individual component to input value.\n");

            imgui_.ColorEdit4("color 2", col2);
        }

        {
            // Using the _simplified_ one-liner ListBox() api here
            // See "List boxes" section for examples of how to use the more flexible BeginListBox()/EndListBox() api.
            const char* items[] = { "Apple", "Banana", "Cherry", "Kiwi", "Mango", "Orange", "Pineapple", "Strawberry", "Watermelon" };
            static int item_current = 1;
            imgui_.ListBox("listbox", &item_current, items, IM_ARRAYSIZE(items), 4);
            imgui_.SameLine(); HelpMarker(imgui_, 
                "Using the simplified one-liner ListBox API here.\nRefer to the \"List boxes\" section below for an explanation of how to use the more flexible and general BeginListBox/EndListBox API.");
        }

        imgui_.TreePop();
    }

    // Testing ImGuiOnceUponAFrame helper.
    //static ImGuiOnceUponAFrame once;
    //for (int i = 0; i < 5; i++)
    //    if (once)
    //        imgui_.Text("This will be displayed only once.");

    if (imgui_.TreeNode("Trees"))
    {
        if (imgui_.TreeNode("Basic trees"))
        {
            for (int i = 0; i < 5; i++)
            {
                // Use SetNextItemOpen() so set the default state of a node to be open. We could
                // also use TreeNodeEx() with the ImGuiTreeNodeFlags_DefaultOpen flag to achieve the same thing!
                if (i == 0)
                    imgui_.SetNextItemOpen(true, ImGuiCond_Once);

                if (imgui_.TreeNode((void*)(intptr_t)i, "Child %d", i))
                {
                    imgui_.Text("blah blah");
                    imgui_.SameLine();
                    if (imgui_.SmallButton("button")) {}
                    imgui_.TreePop();
                }
            }
            imgui_.TreePop();
        }

        if (imgui_.TreeNode("Advanced, with Selectable nodes"))
        {
            HelpMarker(imgui_, 
                "This is a more typical looking tree with selectable nodes.\n"
                "Click to select, CTRL+Click to toggle, click on arrows or double-click to open.");
            static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
            static bool align_label_with_current_x_position = false;
            static bool test_drag_and_drop = false;
            imgui_.CheckboxFlags("ImGuiTreeNodeFlags_OpenOnArrow",       &base_flags, ImGuiTreeNodeFlags_OpenOnArrow);
            imgui_.CheckboxFlags("ImGuiTreeNodeFlags_OpenOnDoubleClick", &base_flags, ImGuiTreeNodeFlags_OpenOnDoubleClick);
            imgui_.CheckboxFlags("ImGuiTreeNodeFlags_SpanAvailWidth",    &base_flags, ImGuiTreeNodeFlags_SpanAvailWidth); imgui_.SameLine(); HelpMarker(imgui_, "Extend hit area to all available width instead of allowing more items to be laid out after the node.");
            imgui_.CheckboxFlags("ImGuiTreeNodeFlags_SpanFullWidth",     &base_flags, ImGuiTreeNodeFlags_SpanFullWidth);
            imgui_.Checkbox("Align label with current X position", &align_label_with_current_x_position);
            imgui_.Checkbox("Test tree node as drag source", &test_drag_and_drop);
            imgui_.Text("Hello!");
            if (align_label_with_current_x_position)
                imgui_.Unindent(imgui_.GetTreeNodeToLabelSpacing());

            // 'selection_mask' is dumb representation of what may be user-side selection state.
            //  You may retain selection state inside or outside your objects in whatever format you see fit.
            // 'node_clicked' is temporary storage of what node we have clicked to process selection at the end
            /// of the loop. May be a pointer to your own node type, etc.
            static int selection_mask = (1 << 2);
            int node_clicked = -1;
            for (int i = 0; i < 6; i++)
            {
                // Disable the default "open on single-click behavior" + set Selected flag according to our selection.
                ImGuiTreeNodeFlags node_flags = base_flags;
                const bool is_selected = (selection_mask & (1 << i)) != 0;
                if (is_selected)
                    node_flags |= ImGuiTreeNodeFlags_Selected;
                if (i < 3)
                {
                    // Items 0..2 are Tree Node
                    bool node_open = imgui_.TreeNodeEx((void*)(intptr_t)i, node_flags, "Selectable Node %d", i);
                    if (imgui_.IsItemClicked())
                        node_clicked = i;
                    if (test_drag_and_drop && imgui_.BeginDragDropSource())
                    {
                        imgui_.SetDragDropPayload("_TREENODE", NULL, 0);
                        imgui_.Text("This is a drag and drop source");
                        imgui_.EndDragDropSource();
                    }
                    if (node_open)
                    {
                        imgui_.BulletText("Blah blah\nBlah Blah");
                        imgui_.TreePop();
                    }
                }
                else
                {
                    // Items 3..5 are Tree Leaves
                    // The only reason we use TreeNode at all is to allow selection of the leaf. Otherwise we can
                    // use BulletText() or advance the cursor by GetTreeNodeToLabelSpacing() and call Text().
                    node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen; // ImGuiTreeNodeFlags_Bullet
                    imgui_.TreeNodeEx((void*)(intptr_t)i, node_flags, "Selectable Leaf %d", i);
                    if (imgui_.IsItemClicked())
                        node_clicked = i;
                    if (test_drag_and_drop && imgui_.BeginDragDropSource())
                    {
                        imgui_.SetDragDropPayload("_TREENODE", NULL, 0);
                        imgui_.Text("This is a drag and drop source");
                        imgui_.EndDragDropSource();
                    }
                }
            }
            if (node_clicked != -1)
            {
                // Update selection state
                // (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
                if (imgui_.GetIO().KeyCtrl)
                    selection_mask ^= (1 << node_clicked);          // CTRL+click to toggle
                else //if (!(selection_mask & (1 << node_clicked))) // Depending on selection behavior you want, may want to preserve selection when clicking on item that is part of the selection
                    selection_mask = (1 << node_clicked);           // Click to single-select
            }
            if (align_label_with_current_x_position)
                imgui_.Indent(imgui_.GetTreeNodeToLabelSpacing());
            imgui_.TreePop();
        }
        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Collapsing Headers"))
    {
        static bool closable_group = true;
        imgui_.Checkbox("Show 2nd header", &closable_group);
        if (imgui_.CollapsingHeader("Header", ImGuiTreeNodeFlags_None))
        {
            imgui_.Text("IsItemHovered: %d", imgui_.IsItemHovered());
            for (int i = 0; i < 5; i++)
                imgui_.Text("Some content %d", i);
        }
        if (imgui_.CollapsingHeader("Header with a close button", &closable_group))
        {
            imgui_.Text("IsItemHovered: %d", imgui_.IsItemHovered());
            for (int i = 0; i < 5; i++)
                imgui_.Text("More content %d", i);
        }
        /*
        if (imgui_.CollapsingHeader("Header with a bullet", ImGuiTreeNodeFlags_Bullet))
            imgui_.Text("IsItemHovered: %d", imgui_.IsItemHovered());
        */
        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Bullets"))
    {
        imgui_.BulletText("Bullet point 1");
        imgui_.BulletText("Bullet point 2\nOn multiple lines");
        if (imgui_.TreeNode("Tree node"))
        {
            imgui_.BulletText("Another bullet point");
            imgui_.TreePop();
        }
        imgui_.Bullet(); imgui_.Text("Bullet point 3 (two calls)");
        imgui_.Bullet(); imgui_.SmallButton("Button");
        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Text"))
    {
        if (imgui_.TreeNode("Colorful Text"))
        {
            // Using shortcut. You can use PushStyleColor()/PopStyleColor() for more flexibility.
            imgui_.TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Pink");
            imgui_.TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Yellow");
            imgui_.TextDisabled("Disabled");
            imgui_.SameLine(); HelpMarker(imgui_, "The TextDisabled color is stored in ImGuiStyle.");
            imgui_.TreePop();
        }

        if (imgui_.TreeNode("Word Wrapping"))
        {
            // Using shortcut. You can use PushTextWrapPos()/PopTextWrapPos() for more flexibility.
            imgui_.TextWrapped(
                "This text should automatically wrap on the edge of the window. The current implementation "
                "for text wrapping follows simple rules suitable for English and possibly other languages.");
            imgui_.Spacing();

            static float wrap_width = 200.0f;
            imgui_.SliderFloat("Wrap width", &wrap_width, -20, 600, "%.0f");

            ImDrawList* draw_list = imgui_.GetWindowDrawList();
            for (int n = 0; n < 2; n++)
            {
                imgui_.Text("Test paragraph %d:", n);
                ImVec2 pos = imgui_.GetCursorScreenPos();
                ImVec2 marker_min = ImVec2(pos.x + wrap_width, pos.y);
                ImVec2 marker_max = ImVec2(pos.x + wrap_width + 10, pos.y + imgui_.GetTextLineHeight());
                imgui_.PushTextWrapPos(imgui_.GetCursorPos().x + wrap_width);
                if (n == 0)
                    imgui_.Text("The lazy dog is a good dog. This paragraph should fit within %.0f pixels. Testing a 1 character word. The quick brown fox jumps over the lazy dog.", wrap_width);
                else
                    imgui_.Text("aaaaaaaa bbbbbbbb, c cccccccc,dddddddd. d eeeeeeee   ffffffff. gggggggg!hhhhhhhh");

                // Draw actual text bounding box, following by marker of our expected limit (should not overlap!)
                draw_list->AddRect(imgui_.GetItemRectMin(), imgui_.GetItemRectMax(), IM_COL32(255, 255, 0, 255));
                draw_list->AddRectFilled(marker_min, marker_max, IM_COL32(255, 0, 255, 255));
                imgui_.PopTextWrapPos();
            }

            imgui_.TreePop();
        }

        if (imgui_.TreeNode("UTF-8 Text"))
        {
            // UTF-8 test with Japanese characters
            // (Needs a suitable font? Try "Google Noto" or "Arial Unicode". See docs/FONTS.md for details.)
            // - From C++11 you can use the u8"my text" syntax to encode literal strings as UTF-8
            // - For earlier compiler, you may be able to encode your sources as UTF-8 (e.g. in Visual Studio, you
            //   can save your source files as 'UTF-8 without signature').
            // - FOR THIS DEMO FILE ONLY, BECAUSE WE WANT TO SUPPORT OLD COMPILERS, WE ARE *NOT* INCLUDING RAW UTF-8
            //   CHARACTERS IN THIS SOURCE FILE. Instead we are encoding a few strings with hexadecimal constants.
            //   Don't do this in your application! Please use u8"text in any language" in your application!
            // Note that characters values are preserved even by InputText() if the font cannot be displayed,
            // so you can safely copy & paste garbled characters into another application.
            imgui_.TextWrapped(
                "CJK text will only appears if the font was loaded with the appropriate CJK character ranges. "
                "Call io.Fonts->AddFontFromFileTTF() manually to load extra character ranges. "
                "Read docs/FONTS.md for details.");
            imgui_.Text("Hiragana: \xe3\x81\x8b\xe3\x81\x8d\xe3\x81\x8f\xe3\x81\x91\xe3\x81\x93 (kakikukeko)"); // Normally we would use u8"blah blah" with the proper characters directly in the string.
            imgui_.Text("Kanjis: \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e (nihongo)");
            static char buf[32] = "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e";
            //static char buf[32] = u8"NIHONGO"; // <- this is how you would write it with C++11, using real kanjis
            imgui_.InputText("UTF-8 input", buf, IM_ARRAYSIZE(buf));
            imgui_.TreePop();
        }
        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Images"))
    {
        ImGuiIO& io = imgui_.GetIO();
        imgui_.TextWrapped(
            "Below we are displaying the font texture (which is the only texture we have access to in this demo). "
            "Use the 'ImTextureID' type as storage to pass pointers or identifier to your own texture data. "
            "Hover the texture for a zoomed view!");

        // Below we are displaying the font texture because it is the only texture we have access to inside the demo!
        // Remember that ImTextureID is just storage for whatever you want it to be. It is essentially a value that
        // will be passed to the rendering backend via the ImDrawCmd structure.
        // If you use one of the default imgui_impl_XXXX.cpp rendering backend, they all have comments at the top
        // of their respective source file to specify what they expect to be stored in ImTextureID, for example:
        // - The imgui_impl_dx11.cpp renderer expect a 'ID3D11ShaderResourceView*' pointer
        // - The imgui_impl_opengl3.cpp renderer expect a GLuint OpenGL texture identifier, etc.
        // More:
        // - If you decided that ImTextureID = MyEngineTexture*, then you can pass your MyEngineTexture* pointers
        //   to imgui_.Image(), and gather width/height through your own functions, etc.
        // - You can use ShowMetricsWindow() to inspect the draw data that are being passed to your renderer,
        //   it will help you debug issues if you are confused about it.
        // - Consider using the lower-level ImDrawList::AddImage() API, via imgui_.GetWindowDrawList()->AddImage().
        // - Read https://github.com/ocornut/imgui/blob/master/docs/FAQ.md
        // - Read https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
        ImTextureID my_tex_id = io.Fonts->TexID;
        float my_tex_w = (float)io.Fonts->TexWidth;
        float my_tex_h = (float)io.Fonts->TexHeight;
        {
            imgui_.Text("%.0fx%.0f", my_tex_w, my_tex_h);
            ImVec2 pos = imgui_.GetCursorScreenPos();
            ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
            ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
            ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
            imgui_.Image(my_tex_id, ImVec2(my_tex_w, my_tex_h), uv_min, uv_max, tint_col, border_col);
            if (imgui_.IsItemHovered())
            {
                imgui_.BeginTooltip();
                float region_sz = 32.0f;
                float region_x = io.MousePos.x - pos.x - region_sz * 0.5f;
                float region_y = io.MousePos.y - pos.y - region_sz * 0.5f;
                float zoom = 4.0f;
                if (region_x < 0.0f) { region_x = 0.0f; }
                else if (region_x > my_tex_w - region_sz) { region_x = my_tex_w - region_sz; }
                if (region_y < 0.0f) { region_y = 0.0f; }
                else if (region_y > my_tex_h - region_sz) { region_y = my_tex_h - region_sz; }
                imgui_.Text("Min: (%.2f, %.2f)", region_x, region_y);
                imgui_.Text("Max: (%.2f, %.2f)", region_x + region_sz, region_y + region_sz);
                ImVec2 uv0 = ImVec2((region_x) / my_tex_w, (region_y) / my_tex_h);
                ImVec2 uv1 = ImVec2((region_x + region_sz) / my_tex_w, (region_y + region_sz) / my_tex_h);
                imgui_.Image(my_tex_id, ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1, tint_col, border_col);
                imgui_.EndTooltip();
            }
        }
        imgui_.TextWrapped("And now some textured buttons..");
        static int pressed_count = 0;
        for (int i = 0; i < 8; i++)
        {
            imgui_.PushID(i);
            int frame_padding = -1 + i;                             // -1 == uses default padding (style.FramePadding)
            ImVec2 size = ImVec2(32.0f, 32.0f);                     // Size of the image we want to make visible
            ImVec2 uv0 = ImVec2(0.0f, 0.0f);                        // UV coordinates for lower-left
            ImVec2 uv1 = ImVec2(32.0f / my_tex_w, 32.0f / my_tex_h);// UV coordinates for (32,32) in our texture
            ImVec4 bg_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);         // Black background
            ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);       // No tint
            if (imgui_.ImageButton(my_tex_id, size, uv0, uv1, frame_padding, bg_col, tint_col))
                pressed_count += 1;
            imgui_.PopID();
            imgui_.SameLine();
        }
        imgui_.NewLine();
        imgui_.Text("Pressed %d times.", pressed_count);
        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Combo"))
    {
        // Expose flags as checkbox for the demo
        static ImGuiComboFlags flags = 0;
        imgui_.CheckboxFlags("ImGuiComboFlags_PopupAlignLeft", &flags, ImGuiComboFlags_PopupAlignLeft);
        imgui_.SameLine(); HelpMarker(imgui_, "Only makes a difference if the popup is larger than the combo");
        if (imgui_.CheckboxFlags("ImGuiComboFlags_NoArrowButton", &flags, ImGuiComboFlags_NoArrowButton))
            flags &= ~ImGuiComboFlags_NoPreview;     // Clear the other flag, as we cannot combine both
        if (imgui_.CheckboxFlags("ImGuiComboFlags_NoPreview", &flags, ImGuiComboFlags_NoPreview))
            flags &= ~ImGuiComboFlags_NoArrowButton; // Clear the other flag, as we cannot combine both

        // Using the generic BeginCombo() API, you have full control over how to display the combo contents.
        // (your selection data could be an index, a pointer to the object, an id for the object, a flag intrusively
        // stored in the object itself, etc.)
        const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIII", "JJJJ", "KKKK", "LLLLLLL", "MMMM", "OOOOOOO" };
        static int item_current_idx = 0; // Here we store our selection data as an index.
        const char* combo_label = items[item_current_idx];  // Label to preview before opening the combo (technically it could be anything)
        if (imgui_.BeginCombo("combo 1", combo_label, flags))
        {
            for (int n = 0; n < IM_ARRAYSIZE(items); n++)
            {
                const bool is_selected = (item_current_idx == n);
                if (imgui_.Selectable(items[n], is_selected))
                    item_current_idx = n;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    imgui_.SetItemDefaultFocus();
            }
            imgui_.EndCombo();
        }

        // Simplified one-liner Combo() API, using values packed in a single constant string
        static int item_current_2 = 0;
        imgui_.Combo("combo 2 (one-liner)", &item_current_2, "aaaa\0bbbb\0cccc\0dddd\0eeee\0\0");

        // Simplified one-liner Combo() using an array of const char*
        static int item_current_3 = -1; // If the selection isn't within 0..count, Combo won't display a preview
        imgui_.Combo("combo 3 (array)", &item_current_3, items, IM_ARRAYSIZE(items));

        // Simplified one-liner Combo() using an accessor function
        struct Funcs { static bool ItemGetter(void* data, int n, const char** out_str) { *out_str = ((const char**)data)[n]; return true; } };
        static int item_current_4 = 0;
        imgui_.Combo("combo 4 (function)", &item_current_4, &Funcs::ItemGetter, items, IM_ARRAYSIZE(items));

        imgui_.TreePop();
    }

    if (imgui_.TreeNode("List boxes"))
    {
        // Using the generic BeginListBox() API, you have full control over how to display the combo contents.
        // (your selection data could be an index, a pointer to the object, an id for the object, a flag intrusively
        // stored in the object itself, etc.)
        const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIII", "JJJJ", "KKKK", "LLLLLLL", "MMMM", "OOOOOOO" };
        static int item_current_idx = 0; // Here we store our selection data as an index.
        if (imgui_.BeginListBox("listbox 1"))
        {
            for (int n = 0; n < IM_ARRAYSIZE(items); n++)
            {
                const bool is_selected = (item_current_idx == n);
                if (imgui_.Selectable(items[n], is_selected))
                    item_current_idx = n;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    imgui_.SetItemDefaultFocus();
            }
            imgui_.EndListBox();
        }

        // Custom size: use all width, 5 items tall
        imgui_.Text("Full-width:");
        if (imgui_.BeginListBox("##listbox 2", ImVec2(-FLT_MIN, 5 * imgui_.GetTextLineHeightWithSpacing())))
        {
            for (int n = 0; n < IM_ARRAYSIZE(items); n++)
            {
                const bool is_selected = (item_current_idx == n);
                if (imgui_.Selectable(items[n], is_selected))
                    item_current_idx = n;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    imgui_.SetItemDefaultFocus();
            }
            imgui_.EndListBox();
        }

        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Selectables"))
    {
        // Selectable() has 2 overloads:
        // - The one taking "bool selected" as a read-only selection information.
        //   When Selectable() has been clicked it returns true and you can alter selection state accordingly.
        // - The one taking "bool* p_selected" as a read-write selection information (convenient in some cases)
        // The earlier is more flexible, as in real application your selection may be stored in many different ways
        // and not necessarily inside a bool value (e.g. in flags within objects, as an external list, etc).
        if (imgui_.TreeNode("Basic"))
        {
            static bool selection[5] = { false, true, false, false, false };
            imgui_.Selectable("1. I am selectable", &selection[0]);
            imgui_.Selectable("2. I am selectable", &selection[1]);
            imgui_.Text("3. I am not selectable");
            imgui_.Selectable("4. I am selectable", &selection[3]);
            if (imgui_.Selectable("5. I am double clickable", selection[4], ImGuiSelectableFlags_AllowDoubleClick))
                if (imgui_.IsMouseDoubleClicked(0))
                    selection[4] = !selection[4];
            imgui_.TreePop();
        }
        if (imgui_.TreeNode("Selection State: Single Selection"))
        {
            static int selected = -1;
            for (int n = 0; n < 5; n++)
            {
                char buf[32];
                sprintf(buf, "Object %d", n);
                if (imgui_.Selectable(buf, selected == n))
                    selected = n;
            }
            imgui_.TreePop();
        }
        if (imgui_.TreeNode("Selection State: Multiple Selection"))
        {
            HelpMarker(imgui_, "Hold CTRL and click to select multiple items.");
            static bool selection[5] = { false, false, false, false, false };
            for (int n = 0; n < 5; n++)
            {
                char buf[32];
                sprintf(buf, "Object %d", n);
                if (imgui_.Selectable(buf, selection[n]))
                {
                    if (!imgui_.GetIO().KeyCtrl)    // Clear selection when CTRL is not held
                        memset(selection, 0, sizeof(selection));
                    selection[n] ^= 1;
                }
            }
            imgui_.TreePop();
        }
        if (imgui_.TreeNode("Rendering more text into the same line"))
        {
            // Using the Selectable() override that takes "bool* p_selected" parameter,
            // this function toggle your bool value automatically.
            static bool selected[3] = { false, false, false };
            imgui_.Selectable("main.c",    &selected[0]); imgui_.SameLine(300); imgui_.Text(" 2,345 bytes");
            imgui_.Selectable("Hello.cpp", &selected[1]); imgui_.SameLine(300); imgui_.Text("12,345 bytes");
            imgui_.Selectable("Hello.h",   &selected[2]); imgui_.SameLine(300); imgui_.Text(" 2,345 bytes");
            imgui_.TreePop();
        }
        if (imgui_.TreeNode("In columns"))
        {
            static bool selected[10] = {};

            if (imgui_.BeginTable("split1", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings))
            {
                for (int i = 0; i < 10; i++)
                {
                    char label[32];
                    sprintf(label, "Item %d", i);
                    imgui_.TableNextColumn();
                    imgui_.Selectable(label, &selected[i]); // FIXME-TABLE: Selection overlap
                }
                imgui_.EndTable();
            }
            imgui_.Separator();
            if (imgui_.BeginTable("split2", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings))
            {
                for (int i = 0; i < 10; i++)
                {
                    char label[32];
                    sprintf(label, "Item %d", i);
                    imgui_.TableNextRow();
                    imgui_.TableNextColumn();
                    imgui_.Selectable(label, &selected[i], ImGuiSelectableFlags_SpanAllColumns);
                    imgui_.TableNextColumn();
                    imgui_.Text("Some other contents");
                    imgui_.TableNextColumn();
                    imgui_.Text("123456");
                }
                imgui_.EndTable();
            }
            imgui_.TreePop();
        }
        if (imgui_.TreeNode("Grid"))
        {
            static char selected[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };

            // Add in a bit of silly fun...
            const float time = (float)imgui_.GetTime();
            const bool winning_state = memchr(selected, 0, sizeof(selected)) == NULL; // If all cells are selected...
            if (winning_state)
                imgui_.PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f + 0.5f * cosf(time * 2.0f), 0.5f + 0.5f * sinf(time * 3.0f)));

            for (int y = 0; y < 4; y++)
                for (int x = 0; x < 4; x++)
                {
                    if (x > 0)
                        imgui_.SameLine();
                    imgui_.PushID(y * 4 + x);
                    if (imgui_.Selectable("Sailor", selected[y][x] != 0, 0, ImVec2(50, 50)))
                    {
                        // Toggle clicked cell + toggle neighbors
                        selected[y][x] ^= 1;
                        if (x > 0) { selected[y][x - 1] ^= 1; }
                        if (x < 3) { selected[y][x + 1] ^= 1; }
                        if (y > 0) { selected[y - 1][x] ^= 1; }
                        if (y < 3) { selected[y + 1][x] ^= 1; }
                    }
                    imgui_.PopID();
                }

            if (winning_state)
                imgui_.PopStyleVar();
            imgui_.TreePop();
        }
        if (imgui_.TreeNode("Alignment"))
        {
            HelpMarker(imgui_, 
                "By default, Selectables uses style.SelectableTextAlign but it can be overridden on a per-item "
                "basis using PushStyleVar(). You'll probably want to always keep your default situation to "
                "left-align otherwise it becomes difficult to layout multiple items on a same line");
            static bool selected[3 * 3] = { true, false, true, false, true, false, true, false, true };
            for (int y = 0; y < 3; y++)
            {
                for (int x = 0; x < 3; x++)
                {
                    ImVec2 alignment = ImVec2((float)x / 2.0f, (float)y / 2.0f);
                    char name[32];
                    sprintf(name, "(%.1f,%.1f)", alignment.x, alignment.y);
                    if (x > 0) imgui_.SameLine();
                    imgui_.PushStyleVar(ImGuiStyleVar_SelectableTextAlign, alignment);
                    imgui_.Selectable(name, &selected[3 * y + x], ImGuiSelectableFlags_None, ImVec2(80, 80));
                    imgui_.PopStyleVar();
                }
            }
            imgui_.TreePop();
        }
        imgui_.TreePop();
    }

    // To wire InputText() with std::string or any other custom string type,
    // see the "Text Input > Resize Callback" section of this demo, and the misc/cpp/imgui_stdlib.h file.
    if (imgui_.TreeNode("Text Input"))
    {
        if (imgui_.TreeNode("Multi-line Text Input"))
        {
            // Note: we are using a fixed-sized buffer for simplicity here. See ImGuiInputTextFlags_CallbackResize
            // and the code in misc/cpp/imgui_stdlib.h for how to setup InputText() for dynamically resizing strings.
            static char text[1024 * 16] =
                "/*\n"
                " The Pentium F00F bug, shorthand for F0 0F C7 C8,\n"
                " the hexadecimal encoding of one offending instruction,\n"
                " more formally, the invalid operand with locked CMPXCHG8B\n"
                " instruction bug, is a design flaw in the majority of\n"
                " Intel Pentium, Pentium MMX, and Pentium OverDrive\n"
                " processors (all in the P5 microarchitecture).\n"
                "*/\n\n"
                "label:\n"
                "\tlock cmpxchg8b eax\n";

            static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
            HelpMarker(imgui_, "You can use the ImGuiInputTextFlags_CallbackResize facility if you need to wire InputTextMultiline() to a dynamic string type. See misc/cpp/imgui_stdlib.h for an example. (This is not demonstrated in imgui_demo.cpp because we don't want to include <string> in here)");
            imgui_.CheckboxFlags("ImGuiInputTextFlags_ReadOnly", &flags, ImGuiInputTextFlags_ReadOnly);
            imgui_.CheckboxFlags("ImGuiInputTextFlags_AllowTabInput", &flags, ImGuiInputTextFlags_AllowTabInput);
            imgui_.CheckboxFlags("ImGuiInputTextFlags_CtrlEnterForNewLine", &flags, ImGuiInputTextFlags_CtrlEnterForNewLine);
            imgui_.InputTextMultiline("##source", text, IM_ARRAYSIZE(text), ImVec2(-FLT_MIN, imgui_.GetTextLineHeight() * 16), flags);
            imgui_.TreePop();
        }

        if (imgui_.TreeNode("Filtered Text Input"))
        {
            struct TextFilters
            {
                // Return 0 (pass) if the character is 'i' or 'm' or 'g' or 'u' or 'i'
                static int FilterImGuiLetters(ImGuiInputTextCallbackData* data)
                {
                    if (data->EventChar < 256 && strchr("imgui", (char)data->EventChar))
                        return 0;
                    return 1;
                }
            };

            static char buf1[64] = ""; imgui_.InputText("default",     buf1, 64);
            static char buf2[64] = ""; imgui_.InputText("decimal",     buf2, 64, ImGuiInputTextFlags_CharsDecimal);
            static char buf3[64] = ""; imgui_.InputText("hexadecimal", buf3, 64, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
            static char buf4[64] = ""; imgui_.InputText("uppercase",   buf4, 64, ImGuiInputTextFlags_CharsUppercase);
            static char buf5[64] = ""; imgui_.InputText("no blank",    buf5, 64, ImGuiInputTextFlags_CharsNoBlank);
            static char buf6[64] = ""; imgui_.InputText("\"imgui\" letters", buf6, 64, ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterImGuiLetters);
            imgui_.TreePop();
        }

        if (imgui_.TreeNode("Password Input"))
        {
            static char password[64] = "password123";
            imgui_.InputText("password", password, IM_ARRAYSIZE(password), ImGuiInputTextFlags_Password);
            imgui_.SameLine(); HelpMarker(imgui_, "Display all characters as '*'.\nDisable clipboard cut and copy.\nDisable logging.\n");
            imgui_.InputTextWithHint("password (w/ hint)", "<password>", password, IM_ARRAYSIZE(password), ImGuiInputTextFlags_Password);
            imgui_.InputText("password (clear)", password, IM_ARRAYSIZE(password));
            imgui_.TreePop();
        }

        if (imgui_.TreeNode("Completion, History, Edit Callbacks"))
        {
            struct Funcs
            {
                static int MyCallback(ImGuiInputTextCallbackData* data)
                {
                    if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
                    {
                        data->InsertChars(data->CursorPos, "..");
                    }
                    else if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
                    {
                        if (data->EventKey == ImGuiKey_UpArrow)
                        {
                            data->DeleteChars(0, data->BufTextLen);
                            data->InsertChars(0, "Pressed Up!");
                            data->SelectAll();
                        }
                        else if (data->EventKey == ImGuiKey_DownArrow)
                        {
                            data->DeleteChars(0, data->BufTextLen);
                            data->InsertChars(0, "Pressed Down!");
                            data->SelectAll();
                        }
                    }
                    else if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit)
                    {
                        // Toggle casing of first character
                        char c = data->Buf[0];
                        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) data->Buf[0] ^= 32;
                        data->BufDirty = true;

                        // Increment a counter
                        int* p_int = (int*)data->UserData;
                        *p_int = *p_int + 1;
                    }
                    return 0;
                }
            };
            static char buf1[64];
            imgui_.InputText("Completion", buf1, 64, ImGuiInputTextFlags_CallbackCompletion, Funcs::MyCallback);
            imgui_.SameLine(); HelpMarker(imgui_, "Here we append \"..\" each time Tab is pressed. See 'Examples>Console' for a more meaningful demonstration of using this callback.");

            static char buf2[64];
            imgui_.InputText("History", buf2, 64, ImGuiInputTextFlags_CallbackHistory, Funcs::MyCallback);
            imgui_.SameLine(); HelpMarker(imgui_, "Here we replace and select text each time Up/Down are pressed. See 'Examples>Console' for a more meaningful demonstration of using this callback.");

            static char buf3[64];
            static int edit_count = 0;
            imgui_.InputText("Edit", buf3, 64, ImGuiInputTextFlags_CallbackEdit, Funcs::MyCallback, (void*)&edit_count);
            imgui_.SameLine(); HelpMarker(imgui_, "Here we toggle the casing of the first character on every edits + count edits.");
            imgui_.SameLine(); imgui_.Text("(%d)", edit_count);

            imgui_.TreePop();
        }

        if (imgui_.TreeNode("Resize Callback"))
        {
            // To wire InputText() with std::string or any other custom string type,
            // you can use the ImGuiInputTextFlags_CallbackResize flag + create a custom imgui_.InputText() wrapper
            // using your preferred type. See misc/cpp/imgui_stdlib.h for an implementation of this using std::string.
            HelpMarker(imgui_, 
                "Using ImGuiInputTextFlags_CallbackResize to wire your custom string type to InputText().\n\n"
                "See misc/cpp/imgui_stdlib.h for an implementation of this for std::string.");
            
            struct Funcs
            {
                ImGui& imgui;

                static int MyResizeCallback(ImGuiInputTextCallbackData* data)
                {
                    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
                    {
                        ImVector<char>* my_str = (ImVector<char>*)data->UserData;
                        IM_ASSERT(my_str->begin() == data->Buf);
                        my_str->resize(data->BufSize); // NB: On resizing calls, generally data->BufSize == data->BufTextLen + 1
                        data->Buf = my_str->begin();
                    }
                    return 0;
                }

                // Note: Because imgui_. is a namespace you would typically add your own function into the namespace.
                // For example, you code may declare a function 'imgui_.InputText(const char* label, MyString* my_str)'
                bool MyInputTextMultiline(const char* label, ImVector<char>* my_str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0)
                {
                    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
                    return imgui.InputTextMultiline(label, my_str->begin(), (size_t)my_str->size(), size, flags | ImGuiInputTextFlags_CallbackResize, Funcs::MyResizeCallback, (void*)my_str);
                }
            };

            // For this demo we are using ImVector as a string container.
            // Note that because we need to store a terminating zero character, our size/capacity are 1 more
            // than usually reported by a typical string class.
            static ImVector<char> my_str(imgui_);
            if (my_str.empty())
                my_str.push_back(0);
            Funcs{imgui_}.MyInputTextMultiline("##MyStr", &my_str, ImVec2(-FLT_MIN, imgui_.GetTextLineHeight() * 16));
            imgui_.Text("Data: %p\nSize: %d\nCapacity: %d", (void*)my_str.begin(), my_str.size(), my_str.capacity());
            imgui_.TreePop();
        }

        imgui_.TreePop();
    }

    // Tabs
    if (imgui_.TreeNode("Tabs"))
    {
        if (imgui_.TreeNode("Basic"))
        {
            ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
            if (imgui_.BeginTabBar("MyTabBar", tab_bar_flags))
            {
                if (imgui_.BeginTabItem("Avocado"))
                {
                    imgui_.Text("This is the Avocado tab!\nblah blah blah blah blah");
                    imgui_.EndTabItem();
                }
                if (imgui_.BeginTabItem("Broccoli"))
                {
                    imgui_.Text("This is the Broccoli tab!\nblah blah blah blah blah");
                    imgui_.EndTabItem();
                }
                if (imgui_.BeginTabItem("Cucumber"))
                {
                    imgui_.Text("This is the Cucumber tab!\nblah blah blah blah blah");
                    imgui_.EndTabItem();
                }
                imgui_.EndTabBar();
            }
            imgui_.Separator();
            imgui_.TreePop();
        }

        if (imgui_.TreeNode("Advanced & Close Button"))
        {
            // Expose a couple of the available flags. In most cases you may just call BeginTabBar() with no flags (0).
            static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_Reorderable;
            imgui_.CheckboxFlags("ImGuiTabBarFlags_Reorderable", &tab_bar_flags, ImGuiTabBarFlags_Reorderable);
            imgui_.CheckboxFlags("ImGuiTabBarFlags_AutoSelectNewTabs", &tab_bar_flags, ImGuiTabBarFlags_AutoSelectNewTabs);
            imgui_.CheckboxFlags("ImGuiTabBarFlags_TabListPopupButton", &tab_bar_flags, ImGuiTabBarFlags_TabListPopupButton);
            imgui_.CheckboxFlags("ImGuiTabBarFlags_NoCloseWithMiddleMouseButton", &tab_bar_flags, ImGuiTabBarFlags_NoCloseWithMiddleMouseButton);
            if ((tab_bar_flags & ImGuiTabBarFlags_FittingPolicyMask_) == 0)
                tab_bar_flags |= ImGuiTabBarFlags_FittingPolicyDefault_;
            if (imgui_.CheckboxFlags("ImGuiTabBarFlags_FittingPolicyResizeDown", &tab_bar_flags, ImGuiTabBarFlags_FittingPolicyResizeDown))
                tab_bar_flags &= ~(ImGuiTabBarFlags_FittingPolicyMask_ ^ ImGuiTabBarFlags_FittingPolicyResizeDown);
            if (imgui_.CheckboxFlags("ImGuiTabBarFlags_FittingPolicyScroll", &tab_bar_flags, ImGuiTabBarFlags_FittingPolicyScroll))
                tab_bar_flags &= ~(ImGuiTabBarFlags_FittingPolicyMask_ ^ ImGuiTabBarFlags_FittingPolicyScroll);

            // Tab Bar
            const char* names[4] = { "Artichoke", "Beetroot", "Celery", "Daikon" };
            static bool opened[4] = { true, true, true, true }; // Persistent user state
            for (int n = 0; n < IM_ARRAYSIZE(opened); n++)
            {
                if (n > 0) { imgui_.SameLine(); }
                imgui_.Checkbox(names[n], &opened[n]);
            }

            // Passing a bool* to BeginTabItem() is similar to passing one to Begin():
            // the underlying bool will be set to false when the tab is closed.
            if (imgui_.BeginTabBar("MyTabBar", tab_bar_flags))
            {
                for (int n = 0; n < IM_ARRAYSIZE(opened); n++)
                    if (opened[n] && imgui_.BeginTabItem(names[n], &opened[n], ImGuiTabItemFlags_None))
                    {
                        imgui_.Text("This is the %s tab!", names[n]);
                        if (n & 1)
                            imgui_.Text("I am an odd tab.");
                        imgui_.EndTabItem();
                    }
                imgui_.EndTabBar();
            }
            imgui_.Separator();
            imgui_.TreePop();
        }

        if (imgui_.TreeNode("TabItemButton & Leading/Trailing flags"))
        {
            static ImVector<int> active_tabs(imgui_);
            static int next_tab_id = 0;
            if (next_tab_id == 0) // Initialize with some default tabs
                for (int i = 0; i < 3; i++)
                    active_tabs.push_back(next_tab_id++);

            // TabItemButton() and Leading/Trailing flags are distinct features which we will demo together.
            // (It is possible to submit regular tabs with Leading/Trailing flags, or TabItemButton tabs without Leading/Trailing flags...
            // but they tend to make more sense together)
            static bool show_leading_button = true;
            static bool show_trailing_button = true;
            imgui_.Checkbox("Show Leading TabItemButton()", &show_leading_button);
            imgui_.Checkbox("Show Trailing TabItemButton()", &show_trailing_button);

            // Expose some other flags which are useful to showcase how they interact with Leading/Trailing tabs
            static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyResizeDown;
            imgui_.CheckboxFlags("ImGuiTabBarFlags_TabListPopupButton", &tab_bar_flags, ImGuiTabBarFlags_TabListPopupButton);
            if (imgui_.CheckboxFlags("ImGuiTabBarFlags_FittingPolicyResizeDown", &tab_bar_flags, ImGuiTabBarFlags_FittingPolicyResizeDown))
                tab_bar_flags &= ~(ImGuiTabBarFlags_FittingPolicyMask_ ^ ImGuiTabBarFlags_FittingPolicyResizeDown);
            if (imgui_.CheckboxFlags("ImGuiTabBarFlags_FittingPolicyScroll", &tab_bar_flags, ImGuiTabBarFlags_FittingPolicyScroll))
                tab_bar_flags &= ~(ImGuiTabBarFlags_FittingPolicyMask_ ^ ImGuiTabBarFlags_FittingPolicyScroll);

            if (imgui_.BeginTabBar("MyTabBar", tab_bar_flags))
            {
                // Demo a Leading TabItemButton(): click the "?" button to open a menu
                if (show_leading_button)
                    if (imgui_.TabItemButton("?", ImGuiTabItemFlags_Leading | ImGuiTabItemFlags_NoTooltip))
                        imgui_.OpenPopup("MyHelpMenu");
                if (imgui_.BeginPopup("MyHelpMenu"))
                {
                    imgui_.Selectable("Hello!");
                    imgui_.EndPopup();
                }

                // Demo Trailing Tabs: click the "+" button to add a new tab (in your app you may want to use a font icon instead of the "+")
                // Note that we submit it before the regular tabs, but because of the ImGuiTabItemFlags_Trailing flag it will always appear at the end.
                if (show_trailing_button)
                    if (imgui_.TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
                        active_tabs.push_back(next_tab_id++); // Add new tab

                // Submit our regular tabs
                for (int n = 0; n < active_tabs.Size; )
                {
                    bool open = true;
                    char name[16];
                    snprintf(name, IM_ARRAYSIZE(name), "%04d", active_tabs[n]);
                    if (imgui_.BeginTabItem(name, &open, ImGuiTabItemFlags_None))
                    {
                        imgui_.Text("This is the %s tab!", name);
                        imgui_.EndTabItem();
                    }

                    if (!open)
                        active_tabs.erase(active_tabs.Data + n);
                    else
                        n++;
                }

                imgui_.EndTabBar();
            }
            imgui_.Separator();
            imgui_.TreePop();
        }
        imgui_.TreePop();
    }

    // Plot/Graph widgets are not very good.
    // Consider writing your own, or using a third-party one, see:
    // - ImPlot https://github.com/epezent/implot
    // - others https://github.com/ocornut/imgui/wiki/Useful-Widgets
    if (imgui_.TreeNode("Plots Widgets"))
    {
        static bool animate = true;
        imgui_.Checkbox("Animate", &animate);

        static float arr[] = { 0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f };
        imgui_.PlotLines("Frame Times", arr, IM_ARRAYSIZE(arr));

        // Fill an array of contiguous float values to plot
        // Tip: If your float aren't contiguous but part of a structure, you can pass a pointer to your first float
        // and the sizeof() of your structure in the "stride" parameter.
        static float values[90] = {};
        static int values_offset = 0;
        static double refresh_time = 0.0;
        if (!animate || refresh_time == 0.0)
            refresh_time = imgui_.GetTime();
        while (refresh_time < imgui_.GetTime()) // Create data at fixed 60 Hz rate for the demo
        {
            static float phase = 0.0f;
            values[values_offset] = cosf(phase);
            values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
            phase += 0.10f * values_offset;
            refresh_time += 1.0f / 60.0f;
        }

        // Plots can display overlay texts
        // (in this example, we will display an average value)
        {
            float average = 0.0f;
            for (int n = 0; n < IM_ARRAYSIZE(values); n++)
                average += values[n];
            average /= (float)IM_ARRAYSIZE(values);
            char overlay[32];
            sprintf(overlay, "avg %f", average);
            imgui_.PlotLines("Lines", values, IM_ARRAYSIZE(values), values_offset, overlay, -1.0f, 1.0f, ImVec2(0, 80.0f));
        }
        imgui_.PlotHistogram("Histogram", arr, IM_ARRAYSIZE(arr), 0, NULL, 0.0f, 1.0f, ImVec2(0, 80.0f));

        // Use functions to generate output
        // FIXME: This is rather awkward because current plot API only pass in indices.
        // We probably want an API passing floats and user provide sample rate/count.
        struct Funcs
        {
            static float Sin(void*, int i) { return sinf(i * 0.1f); }
            static float Saw(void*, int i) { return (i & 1) ? 1.0f : -1.0f; }
        };
        static int func_type = 0, display_count = 70;
        imgui_.Separator();
        imgui_.SetNextItemWidth(100);
        imgui_.Combo("func", &func_type, "Sin\0Saw\0");
        imgui_.SameLine();
        imgui_.SliderInt("Sample count", &display_count, 1, 400);
        float (*func)(void*, int) = (func_type == 0) ? Funcs::Sin : Funcs::Saw;
        imgui_.PlotLines("Lines", func, NULL, display_count, 0, NULL, -1.0f, 1.0f, ImVec2(0, 80));
        imgui_.PlotHistogram("Histogram", func, NULL, display_count, 0, NULL, -1.0f, 1.0f, ImVec2(0, 80));
        imgui_.Separator();

        // Animate a simple progress bar
        static float progress = 0.0f, progress_dir = 1.0f;
        if (animate)
        {
            progress += progress_dir * 0.4f * imgui_.GetIO().DeltaTime;
            if (progress >= +1.1f) { progress = +1.1f; progress_dir *= -1.0f; }
            if (progress <= -0.1f) { progress = -0.1f; progress_dir *= -1.0f; }
        }

        // Typically we would use ImVec2(-1.0f,0.0f) or ImVec2(-FLT_MIN,0.0f) to use all available width,
        // or ImVec2(width,0.0f) for a specified width. ImVec2(0.0f,0.0f) uses ItemWidth.
        imgui_.ProgressBar(progress, ImVec2(0.0f, 0.0f));
        imgui_.SameLine(0.0f, imgui_.GetStyle().ItemInnerSpacing.x);
        imgui_.Text("Progress Bar");

        float progress_saturated = IM_CLAMP(progress, 0.0f, 1.0f);
        char buf[32];
        sprintf(buf, "%d/%d", (int)(progress_saturated * 1753), 1753);
        imgui_.ProgressBar(progress, ImVec2(0.f, 0.f), buf);
        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Color/Picker Widgets"))
    {
        static ImVec4 color = ImVec4(114.0f / 255.0f, 144.0f / 255.0f, 154.0f / 255.0f, 200.0f / 255.0f);

        static bool alpha_preview = true;
        static bool alpha_half_preview = false;
        static bool drag_and_drop = true;
        static bool options_menu = true;
        static bool hdr = false;
        imgui_.Checkbox("With Alpha Preview", &alpha_preview);
        imgui_.Checkbox("With Half Alpha Preview", &alpha_half_preview);
        imgui_.Checkbox("With Drag and Drop", &drag_and_drop);
        imgui_.Checkbox("With Options Menu", &options_menu); imgui_.SameLine(); HelpMarker(imgui_, "Right-click on the individual color widget to show options.");
        imgui_.Checkbox("With HDR", &hdr); imgui_.SameLine(); HelpMarker(imgui_, "Currently all this does is to lift the 0..1 limits on dragging widgets.");
        ImGuiColorEditFlags misc_flags = (hdr ? ImGuiColorEditFlags_HDR : 0) | (drag_and_drop ? 0 : ImGuiColorEditFlags_NoDragDrop) | (alpha_half_preview ? ImGuiColorEditFlags_AlphaPreviewHalf : (alpha_preview ? ImGuiColorEditFlags_AlphaPreview : 0)) | (options_menu ? 0 : ImGuiColorEditFlags_NoOptions);

        imgui_.Text("Color widget:");
        imgui_.SameLine(); HelpMarker(imgui_, 
            "Click on the color square to open a color picker.\n"
            "CTRL+click on individual component to input value.\n");
        imgui_.ColorEdit3("MyColor##1", (float*)&color, misc_flags);

        imgui_.Text("Color widget HSV with Alpha:");
        imgui_.ColorEdit4("MyColor##2", (float*)&color, ImGuiColorEditFlags_DisplayHSV | misc_flags);

        imgui_.Text("Color widget with Float Display:");
        imgui_.ColorEdit4("MyColor##2f", (float*)&color, ImGuiColorEditFlags_Float | misc_flags);

        imgui_.Text("Color button with Picker:");
        imgui_.SameLine(); HelpMarker(imgui_, 
            "With the ImGuiColorEditFlags_NoInputs flag you can hide all the slider/text inputs.\n"
            "With the ImGuiColorEditFlags_NoLabel flag you can pass a non-empty label which will only "
            "be used for the tooltip and picker popup.");
        imgui_.ColorEdit4("MyColor##3", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | misc_flags);

        imgui_.Text("Color button with Custom Picker Popup:");

        // Generate a default palette. The palette will persist and can be edited.
        static bool saved_palette_init = true;
        static ImVec4 saved_palette[32] = {};
        if (saved_palette_init)
        {
            for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
            {
                imgui_.ColorConvertHSVtoRGB(n / 31.0f, 0.8f, 0.8f,
                    saved_palette[n].x, saved_palette[n].y, saved_palette[n].z);
                saved_palette[n].w = 1.0f; // Alpha
            }
            saved_palette_init = false;
        }

        static ImVec4 backup_color;
        bool open_popup = imgui_.ColorButton("MyColor##3b", color, misc_flags);
        imgui_.SameLine(0, imgui_.GetStyle().ItemInnerSpacing.x);
        open_popup |= imgui_.Button("Palette");
        if (open_popup)
        {
            imgui_.OpenPopup("mypicker");
            backup_color = color;
        }
        if (imgui_.BeginPopup("mypicker"))
        {
            imgui_.Text("MY CUSTOM COLOR PICKER WITH AN AMAZING PALETTE!");
            imgui_.Separator();
            imgui_.ColorPicker4("##picker", (float*)&color, misc_flags | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
            imgui_.SameLine();

            imgui_.BeginGroup(); // Lock X position
            imgui_.Text("Current");
            imgui_.ColorButton("##current", color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40));
            imgui_.Text("Previous");
            if (imgui_.ColorButton("##previous", backup_color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40)))
                color = backup_color;
            imgui_.Separator();
            imgui_.Text("Palette");
            for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
            {
                imgui_.PushID(n);
                if ((n % 8) != 0)
                    imgui_.SameLine(0.0f, imgui_.GetStyle().ItemSpacing.y);

                ImGuiColorEditFlags palette_button_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip;
                if (imgui_.ColorButton("##palette", saved_palette[n], palette_button_flags, ImVec2(20, 20)))
                    color = ImVec4(saved_palette[n].x, saved_palette[n].y, saved_palette[n].z, color.w); // Preserve alpha!

                // Allow user to drop colors into each palette entry. Note that ColorButton() is already a
                // drag source by default, unless specifying the ImGuiColorEditFlags_NoDragDrop flag.
                if (imgui_.BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = imgui_.AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
                        memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 3);
                    if (const ImGuiPayload* payload = imgui_.AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
                        memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 4);
                    imgui_.EndDragDropTarget();
                }

                imgui_.PopID();
            }
            imgui_.EndGroup();
            imgui_.EndPopup();
        }

        imgui_.Text("Color button only:");
        static bool no_border = false;
        imgui_.Checkbox("ImGuiColorEditFlags_NoBorder", &no_border);
        imgui_.ColorButton("MyColor##3c", *(ImVec4*)&color, misc_flags | (no_border ? ImGuiColorEditFlags_NoBorder : 0), ImVec2(80, 80));

        imgui_.Text("Color picker:");
        static bool alpha = true;
        static bool alpha_bar = true;
        static bool side_preview = true;
        static bool ref_color = false;
        static ImVec4 ref_color_v(1.0f, 0.0f, 1.0f, 0.5f);
        static int display_mode = 0;
        static int picker_mode = 0;
        imgui_.Checkbox("With Alpha", &alpha);
        imgui_.Checkbox("With Alpha Bar", &alpha_bar);
        imgui_.Checkbox("With Side Preview", &side_preview);
        if (side_preview)
        {
            imgui_.SameLine();
            imgui_.Checkbox("With Ref Color", &ref_color);
            if (ref_color)
            {
                imgui_.SameLine();
                imgui_.ColorEdit4("##RefColor", &ref_color_v.x, ImGuiColorEditFlags_NoInputs | misc_flags);
            }
        }
        imgui_.Combo("Display Mode", &display_mode, "Auto/Current\0None\0RGB Only\0HSV Only\0Hex Only\0");
        imgui_.SameLine(); HelpMarker(imgui_, 
            "ColorEdit defaults to displaying RGB inputs if you don't specify a display mode, "
            "but the user can change it with a right-click.\n\nColorPicker defaults to displaying RGB+HSV+Hex "
            "if you don't specify a display mode.\n\nYou can change the defaults using SetColorEditOptions().");
        imgui_.Combo("Picker Mode", &picker_mode, "Auto/Current\0Hue bar + SV rect\0Hue wheel + SV triangle\0");
        imgui_.SameLine(); HelpMarker(imgui_, "User can right-click the picker to change mode.");
        ImGuiColorEditFlags flags = misc_flags;
        if (!alpha)            flags |= ImGuiColorEditFlags_NoAlpha;        // This is by default if you call ColorPicker3() instead of ColorPicker4()
        if (alpha_bar)         flags |= ImGuiColorEditFlags_AlphaBar;
        if (!side_preview)     flags |= ImGuiColorEditFlags_NoSidePreview;
        if (picker_mode == 1)  flags |= ImGuiColorEditFlags_PickerHueBar;
        if (picker_mode == 2)  flags |= ImGuiColorEditFlags_PickerHueWheel;
        if (display_mode == 1) flags |= ImGuiColorEditFlags_NoInputs;       // Disable all RGB/HSV/Hex displays
        if (display_mode == 2) flags |= ImGuiColorEditFlags_DisplayRGB;     // Override display mode
        if (display_mode == 3) flags |= ImGuiColorEditFlags_DisplayHSV;
        if (display_mode == 4) flags |= ImGuiColorEditFlags_DisplayHex;
        imgui_.ColorPicker4("MyColor##4", (float*)&color, flags, ref_color ? &ref_color_v.x : NULL);

        imgui_.Text("Set defaults in code:");
        imgui_.SameLine(); HelpMarker(imgui_, 
            "SetColorEditOptions() is designed to allow you to set boot-time default.\n"
            "We don't have Push/Pop functions because you can force options on a per-widget basis if needed,"
            "and the user can change non-forced ones with the options menu.\nWe don't have a getter to avoid"
            "encouraging you to persistently save values that aren't forward-compatible.");
        if (imgui_.Button("Default: Uint8 + HSV + Hue Bar"))
            imgui_.SetColorEditOptions(ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_PickerHueBar);
        if (imgui_.Button("Default: Float + HDR + Hue Wheel"))
            imgui_.SetColorEditOptions(ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);

        // HSV encoded support (to avoid RGB<>HSV round trips and singularities when S==0 or V==0)
        static ImVec4 color_hsv(0.23f, 1.0f, 1.0f, 1.0f); // Stored as HSV!
        imgui_.Spacing();
        imgui_.Text("HSV encoded colors");
        imgui_.SameLine(); HelpMarker(imgui_, 
            "By default, colors are given to ColorEdit and ColorPicker in RGB, but ImGuiColorEditFlags_InputHSV"
            "allows you to store colors as HSV and pass them to ColorEdit and ColorPicker as HSV. This comes with the"
            "added benefit that you can manipulate hue values with the picker even when saturation or value are zero.");
        imgui_.Text("Color widget with InputHSV:");
        imgui_.ColorEdit4("HSV shown as RGB##1", (float*)&color_hsv, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputHSV | ImGuiColorEditFlags_Float);
        imgui_.ColorEdit4("HSV shown as HSV##1", (float*)&color_hsv, ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_InputHSV | ImGuiColorEditFlags_Float);
        imgui_.DragFloat4("Raw HSV values", (float*)&color_hsv, 0.01f, 0.0f, 1.0f);

        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Drag/Slider Flags"))
    {
        // Demonstrate using advanced flags for DragXXX and SliderXXX functions. Note that the flags are the same!
        static ImGuiSliderFlags flags = ImGuiSliderFlags_None;
        imgui_.CheckboxFlags("ImGuiSliderFlags_AlwaysClamp", &flags, ImGuiSliderFlags_AlwaysClamp);
        imgui_.SameLine(); HelpMarker(imgui_, "Always clamp value to min/max bounds (if any) when input manually with CTRL+Click.");
        imgui_.CheckboxFlags("ImGuiSliderFlags_Logarithmic", &flags, ImGuiSliderFlags_Logarithmic);
        imgui_.SameLine(); HelpMarker(imgui_, "Enable logarithmic editing (more precision for small values).");
        imgui_.CheckboxFlags("ImGuiSliderFlags_NoRoundToFormat", &flags, ImGuiSliderFlags_NoRoundToFormat);
        imgui_.SameLine(); HelpMarker(imgui_, "Disable rounding underlying value to match precision of the format string (e.g. %.3f values are rounded to those 3 digits).");
        imgui_.CheckboxFlags("ImGuiSliderFlags_NoInput", &flags, ImGuiSliderFlags_NoInput);
        imgui_.SameLine(); HelpMarker(imgui_, "Disable CTRL+Click or Enter key allowing to input text directly into the widget.");

        // Drags
        static float drag_f = 0.5f;
        static int drag_i = 50;
        imgui_.Text("Underlying float value: %f", drag_f);
        imgui_.DragFloat("DragFloat (0 -> 1)", &drag_f, 0.005f, 0.0f, 1.0f, "%.3f", flags);
        imgui_.DragFloat("DragFloat (0 -> +inf)", &drag_f, 0.005f, 0.0f, FLT_MAX, "%.3f", flags);
        imgui_.DragFloat("DragFloat (-inf -> 1)", &drag_f, 0.005f, -FLT_MAX, 1.0f, "%.3f", flags);
        imgui_.DragFloat("DragFloat (-inf -> +inf)", &drag_f, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f", flags);
        imgui_.DragInt("DragInt (0 -> 100)", &drag_i, 0.5f, 0, 100, "%d", flags);

        // Sliders
        static float slider_f = 0.5f;
        static int slider_i = 50;
        imgui_.Text("Underlying float value: %f", slider_f);
        imgui_.SliderFloat("SliderFloat (0 -> 1)", &slider_f, 0.0f, 1.0f, "%.3f", flags);
        imgui_.SliderInt("SliderInt (0 -> 100)", &slider_i, 0, 100, "%d", flags);

        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Range Widgets"))
    {
        static float begin = 10, end = 90;
        static int begin_i = 100, end_i = 1000;
        imgui_.DragFloatRange2("range float", &begin, &end, 0.25f, 0.0f, 100.0f, "Min: %.1f %%", "Max: %.1f %%", ImGuiSliderFlags_AlwaysClamp);
        imgui_.DragIntRange2("range int", &begin_i, &end_i, 5, 0, 1000, "Min: %d units", "Max: %d units");
        imgui_.DragIntRange2("range int (no bounds)", &begin_i, &end_i, 5, 0, 0, "Min: %d units", "Max: %d units");
        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Data Types"))
    {
        // DragScalar/InputScalar/SliderScalar functions allow various data types
        // - signed/unsigned
        // - 8/16/32/64-bits
        // - integer/float/double
        // To avoid polluting the public API with all possible combinations, we use the ImGuiDataType enum
        // to pass the type, and passing all arguments by pointer.
        // This is the reason the test code below creates local variables to hold "zero" "one" etc. for each types.
        // In practice, if you frequently use a given type that is not covered by the normal API entry points,
        // you can wrap it yourself inside a 1 line function which can take typed argument as value instead of void*,
        // and then pass their address to the generic function. For example:
        //   bool MySliderU64(const char *label, u64* value, u64 min = 0, u64 max = 0, const char* format = "%lld")
        //   {
        //      return SliderScalar(label, ImGuiDataType_U64, value, &min, &max, format);
        //   }

        // Setup limits (as helper variables so we can take their address, as explained above)
        // Note: SliderScalar() functions have a maximum usable range of half the natural type maximum, hence the /2.
        #ifndef LLONG_MIN
        ImS64 LLONG_MIN = -9223372036854775807LL - 1;
        ImS64 LLONG_MAX = 9223372036854775807LL;
        ImU64 ULLONG_MAX = (2ULL * 9223372036854775807LL + 1);
        #endif
        const char    s8_zero  = 0,   s8_one  = 1,   s8_fifty  = 50, s8_min  = -128,        s8_max = 127;
        const ImU8    u8_zero  = 0,   u8_one  = 1,   u8_fifty  = 50, u8_min  = 0,           u8_max = 255;
        const short   s16_zero = 0,   s16_one = 1,   s16_fifty = 50, s16_min = -32768,      s16_max = 32767;
        const ImU16   u16_zero = 0,   u16_one = 1,   u16_fifty = 50, u16_min = 0,           u16_max = 65535;
        const ImS32   s32_zero = 0,   s32_one = 1,   s32_fifty = 50, s32_min = INT_MIN/2,   s32_max = INT_MAX/2,    s32_hi_a = INT_MAX/2 - 100,    s32_hi_b = INT_MAX/2;
        const ImU32   u32_zero = 0,   u32_one = 1,   u32_fifty = 50, u32_min = 0,           u32_max = UINT_MAX/2,   u32_hi_a = UINT_MAX/2 - 100,   u32_hi_b = UINT_MAX/2;
        const ImS64   s64_zero = 0,   s64_one = 1,   s64_fifty = 50, s64_min = LLONG_MIN/2, s64_max = LLONG_MAX/2,  s64_hi_a = LLONG_MAX/2 - 100,  s64_hi_b = LLONG_MAX/2;
        const ImU64   u64_zero = 0,   u64_one = 1,   u64_fifty = 50, u64_min = 0,           u64_max = ULLONG_MAX/2, u64_hi_a = ULLONG_MAX/2 - 100, u64_hi_b = ULLONG_MAX/2;
        const float   f32_zero = 0.f, f32_one = 1.f, f32_lo_a = -10000000000.0f, f32_hi_a = +10000000000.0f;
        const double  f64_zero = 0.,  f64_one = 1.,  f64_lo_a = -1000000000000000.0, f64_hi_a = +1000000000000000.0;

        // State
        static char   s8_v  = 127;
        static ImU8   u8_v  = 255;
        static short  s16_v = 32767;
        static ImU16  u16_v = 65535;
        static ImS32  s32_v = -1;
        static ImU32  u32_v = (ImU32)-1;
        static ImS64  s64_v = -1;
        static ImU64  u64_v = (ImU64)-1;
        static float  f32_v = 0.123f;
        static double f64_v = 90000.01234567890123456789;

        const float drag_speed = 0.2f;
        static bool drag_clamp = false;
        imgui_.Text("Drags:");
        imgui_.Checkbox("Clamp integers to 0..50", &drag_clamp);
        imgui_.SameLine(); HelpMarker(imgui_, 
            "As with every widgets in dear imgui, we never modify values unless there is a user interaction.\n"
            "You can override the clamping limits by using CTRL+Click to input a value.");
        imgui_.DragScalar("drag s8",        ImGuiDataType_S8,     &s8_v,  drag_speed, drag_clamp ? &s8_zero  : NULL, drag_clamp ? &s8_fifty  : NULL);
        imgui_.DragScalar("drag u8",        ImGuiDataType_U8,     &u8_v,  drag_speed, drag_clamp ? &u8_zero  : NULL, drag_clamp ? &u8_fifty  : NULL, "%u ms");
        imgui_.DragScalar("drag s16",       ImGuiDataType_S16,    &s16_v, drag_speed, drag_clamp ? &s16_zero : NULL, drag_clamp ? &s16_fifty : NULL);
        imgui_.DragScalar("drag u16",       ImGuiDataType_U16,    &u16_v, drag_speed, drag_clamp ? &u16_zero : NULL, drag_clamp ? &u16_fifty : NULL, "%u ms");
        imgui_.DragScalar("drag s32",       ImGuiDataType_S32,    &s32_v, drag_speed, drag_clamp ? &s32_zero : NULL, drag_clamp ? &s32_fifty : NULL);
        imgui_.DragScalar("drag u32",       ImGuiDataType_U32,    &u32_v, drag_speed, drag_clamp ? &u32_zero : NULL, drag_clamp ? &u32_fifty : NULL, "%u ms");
        imgui_.DragScalar("drag s64",       ImGuiDataType_S64,    &s64_v, drag_speed, drag_clamp ? &s64_zero : NULL, drag_clamp ? &s64_fifty : NULL);
        imgui_.DragScalar("drag u64",       ImGuiDataType_U64,    &u64_v, drag_speed, drag_clamp ? &u64_zero : NULL, drag_clamp ? &u64_fifty : NULL);
        imgui_.DragScalar("drag float",     ImGuiDataType_Float,  &f32_v, 0.005f,  &f32_zero, &f32_one, "%f");
        imgui_.DragScalar("drag float log", ImGuiDataType_Float,  &f32_v, 0.005f,  &f32_zero, &f32_one, "%f", ImGuiSliderFlags_Logarithmic);
        imgui_.DragScalar("drag double",    ImGuiDataType_Double, &f64_v, 0.0005f, &f64_zero, NULL,     "%.10f grams");
        imgui_.DragScalar("drag double log",ImGuiDataType_Double, &f64_v, 0.0005f, &f64_zero, &f64_one, "0 < %.10f < 1", ImGuiSliderFlags_Logarithmic);

        imgui_.Text("Sliders");
        imgui_.SliderScalar("slider s8 full",       ImGuiDataType_S8,     &s8_v,  &s8_min,   &s8_max,   "%d");
        imgui_.SliderScalar("slider u8 full",       ImGuiDataType_U8,     &u8_v,  &u8_min,   &u8_max,   "%u");
        imgui_.SliderScalar("slider s16 full",      ImGuiDataType_S16,    &s16_v, &s16_min,  &s16_max,  "%d");
        imgui_.SliderScalar("slider u16 full",      ImGuiDataType_U16,    &u16_v, &u16_min,  &u16_max,  "%u");
        imgui_.SliderScalar("slider s32 low",       ImGuiDataType_S32,    &s32_v, &s32_zero, &s32_fifty,"%d");
        imgui_.SliderScalar("slider s32 high",      ImGuiDataType_S32,    &s32_v, &s32_hi_a, &s32_hi_b, "%d");
        imgui_.SliderScalar("slider s32 full",      ImGuiDataType_S32,    &s32_v, &s32_min,  &s32_max,  "%d");
        imgui_.SliderScalar("slider u32 low",       ImGuiDataType_U32,    &u32_v, &u32_zero, &u32_fifty,"%u");
        imgui_.SliderScalar("slider u32 high",      ImGuiDataType_U32,    &u32_v, &u32_hi_a, &u32_hi_b, "%u");
        imgui_.SliderScalar("slider u32 full",      ImGuiDataType_U32,    &u32_v, &u32_min,  &u32_max,  "%u");
        imgui_.SliderScalar("slider s64 low",       ImGuiDataType_S64,    &s64_v, &s64_zero, &s64_fifty,"%" IM_PRId64);
        imgui_.SliderScalar("slider s64 high",      ImGuiDataType_S64,    &s64_v, &s64_hi_a, &s64_hi_b, "%" IM_PRId64);
        imgui_.SliderScalar("slider s64 full",      ImGuiDataType_S64,    &s64_v, &s64_min,  &s64_max,  "%" IM_PRId64);
        imgui_.SliderScalar("slider u64 low",       ImGuiDataType_U64,    &u64_v, &u64_zero, &u64_fifty,"%" IM_PRIu64 " ms");
        imgui_.SliderScalar("slider u64 high",      ImGuiDataType_U64,    &u64_v, &u64_hi_a, &u64_hi_b, "%" IM_PRIu64 " ms");
        imgui_.SliderScalar("slider u64 full",      ImGuiDataType_U64,    &u64_v, &u64_min,  &u64_max,  "%" IM_PRIu64 " ms");
        imgui_.SliderScalar("slider float low",     ImGuiDataType_Float,  &f32_v, &f32_zero, &f32_one);
        imgui_.SliderScalar("slider float low log", ImGuiDataType_Float,  &f32_v, &f32_zero, &f32_one,  "%.10f", ImGuiSliderFlags_Logarithmic);
        imgui_.SliderScalar("slider float high",    ImGuiDataType_Float,  &f32_v, &f32_lo_a, &f32_hi_a, "%e");
        imgui_.SliderScalar("slider double low",    ImGuiDataType_Double, &f64_v, &f64_zero, &f64_one,  "%.10f grams");
        imgui_.SliderScalar("slider double low log",ImGuiDataType_Double, &f64_v, &f64_zero, &f64_one,  "%.10f", ImGuiSliderFlags_Logarithmic);
        imgui_.SliderScalar("slider double high",   ImGuiDataType_Double, &f64_v, &f64_lo_a, &f64_hi_a, "%e grams");

        imgui_.Text("Sliders (reverse)");
        imgui_.SliderScalar("slider s8 reverse",    ImGuiDataType_S8,   &s8_v,  &s8_max,    &s8_min,   "%d");
        imgui_.SliderScalar("slider u8 reverse",    ImGuiDataType_U8,   &u8_v,  &u8_max,    &u8_min,   "%u");
        imgui_.SliderScalar("slider s32 reverse",   ImGuiDataType_S32,  &s32_v, &s32_fifty, &s32_zero, "%d");
        imgui_.SliderScalar("slider u32 reverse",   ImGuiDataType_U32,  &u32_v, &u32_fifty, &u32_zero, "%u");
        imgui_.SliderScalar("slider s64 reverse",   ImGuiDataType_S64,  &s64_v, &s64_fifty, &s64_zero, "%" IM_PRId64);
        imgui_.SliderScalar("slider u64 reverse",   ImGuiDataType_U64,  &u64_v, &u64_fifty, &u64_zero, "%" IM_PRIu64 " ms");

        static bool inputs_step = true;
        imgui_.Text("Inputs");
        imgui_.Checkbox("Show step buttons", &inputs_step);
        imgui_.InputScalar("input s8",      ImGuiDataType_S8,     &s8_v,  inputs_step ? &s8_one  : NULL, NULL, "%d");
        imgui_.InputScalar("input u8",      ImGuiDataType_U8,     &u8_v,  inputs_step ? &u8_one  : NULL, NULL, "%u");
        imgui_.InputScalar("input s16",     ImGuiDataType_S16,    &s16_v, inputs_step ? &s16_one : NULL, NULL, "%d");
        imgui_.InputScalar("input u16",     ImGuiDataType_U16,    &u16_v, inputs_step ? &u16_one : NULL, NULL, "%u");
        imgui_.InputScalar("input s32",     ImGuiDataType_S32,    &s32_v, inputs_step ? &s32_one : NULL, NULL, "%d");
        imgui_.InputScalar("input s32 hex", ImGuiDataType_S32,    &s32_v, inputs_step ? &s32_one : NULL, NULL, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
        imgui_.InputScalar("input u32",     ImGuiDataType_U32,    &u32_v, inputs_step ? &u32_one : NULL, NULL, "%u");
        imgui_.InputScalar("input u32 hex", ImGuiDataType_U32,    &u32_v, inputs_step ? &u32_one : NULL, NULL, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
        imgui_.InputScalar("input s64",     ImGuiDataType_S64,    &s64_v, inputs_step ? &s64_one : NULL);
        imgui_.InputScalar("input u64",     ImGuiDataType_U64,    &u64_v, inputs_step ? &u64_one : NULL);
        imgui_.InputScalar("input float",   ImGuiDataType_Float,  &f32_v, inputs_step ? &f32_one : NULL);
        imgui_.InputScalar("input double",  ImGuiDataType_Double, &f64_v, inputs_step ? &f64_one : NULL);

        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Multi-component Widgets"))
    {
        static float vec4f[4] = { 0.10f, 0.20f, 0.30f, 0.44f };
        static int vec4i[4] = { 1, 5, 100, 255 };

        imgui_.InputFloat2("input float2", vec4f);
        imgui_.DragFloat2("drag float2", vec4f, 0.01f, 0.0f, 1.0f);
        imgui_.SliderFloat2("slider float2", vec4f, 0.0f, 1.0f);
        imgui_.InputInt2("input int2", vec4i);
        imgui_.DragInt2("drag int2", vec4i, 1, 0, 255);
        imgui_.SliderInt2("slider int2", vec4i, 0, 255);
        imgui_.Spacing();

        imgui_.InputFloat3("input float3", vec4f);
        imgui_.DragFloat3("drag float3", vec4f, 0.01f, 0.0f, 1.0f);
        imgui_.SliderFloat3("slider float3", vec4f, 0.0f, 1.0f);
        imgui_.InputInt3("input int3", vec4i);
        imgui_.DragInt3("drag int3", vec4i, 1, 0, 255);
        imgui_.SliderInt3("slider int3", vec4i, 0, 255);
        imgui_.Spacing();

        imgui_.InputFloat4("input float4", vec4f);
        imgui_.DragFloat4("drag float4", vec4f, 0.01f, 0.0f, 1.0f);
        imgui_.SliderFloat4("slider float4", vec4f, 0.0f, 1.0f);
        imgui_.InputInt4("input int4", vec4i);
        imgui_.DragInt4("drag int4", vec4i, 1, 0, 255);
        imgui_.SliderInt4("slider int4", vec4i, 0, 255);

        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Vertical Sliders"))
    {
        const float spacing = 4;
        imgui_.PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));

        static int int_value = 0;
        imgui_.VSliderInt("##int", ImVec2(18, 160), &int_value, 0, 5);
        imgui_.SameLine();

        static float values[7] = { 0.0f, 0.60f, 0.35f, 0.9f, 0.70f, 0.20f, 0.0f };
        imgui_.PushID("set1");
        for (int i = 0; i < 7; i++)
        {
            if (i > 0) imgui_.SameLine();
            imgui_.PushID(i);
            imgui_.PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(i / 7.0f, 0.5f, 0.5f));
            imgui_.PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(i / 7.0f, 0.6f, 0.5f));
            imgui_.PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(i / 7.0f, 0.7f, 0.5f));
            imgui_.PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(i / 7.0f, 0.9f, 0.9f));
            imgui_.VSliderFloat("##v", ImVec2(18, 160), &values[i], 0.0f, 1.0f, "");
            if (imgui_.IsItemActive() || imgui_.IsItemHovered())
                imgui_.SetTooltip("%.3f", values[i]);
            imgui_.PopStyleColor(4);
            imgui_.PopID();
        }
        imgui_.PopID();

        imgui_.SameLine();
        imgui_.PushID("set2");
        static float values2[4] = { 0.20f, 0.80f, 0.40f, 0.25f };
        const int rows = 3;
        const ImVec2 small_slider_size(18, (float)(int)((160.0f - (rows - 1) * spacing) / rows));
        for (int nx = 0; nx < 4; nx++)
        {
            if (nx > 0) imgui_.SameLine();
            imgui_.BeginGroup();
            for (int ny = 0; ny < rows; ny++)
            {
                imgui_.PushID(nx * rows + ny);
                imgui_.VSliderFloat("##v", small_slider_size, &values2[nx], 0.0f, 1.0f, "");
                if (imgui_.IsItemActive() || imgui_.IsItemHovered())
                    imgui_.SetTooltip("%.3f", values2[nx]);
                imgui_.PopID();
            }
            imgui_.EndGroup();
        }
        imgui_.PopID();

        imgui_.SameLine();
        imgui_.PushID("set3");
        for (int i = 0; i < 4; i++)
        {
            if (i > 0) imgui_.SameLine();
            imgui_.PushID(i);
            imgui_.PushStyleVar(ImGuiStyleVar_GrabMinSize, 40);
            imgui_.VSliderFloat("##v", ImVec2(40, 160), &values[i], 0.0f, 1.0f, "%.2f\nsec");
            imgui_.PopStyleVar();
            imgui_.PopID();
        }
        imgui_.PopID();
        imgui_.PopStyleVar();
        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Drag and Drop"))
    {
        if (imgui_.TreeNode("Drag and drop in standard widgets"))
        {
            // ColorEdit widgets automatically act as drag source and drag target.
            // They are using standardized payload strings IMGUI_PAYLOAD_TYPE_COLOR_3F and IMGUI_PAYLOAD_TYPE_COLOR_4F
            // to allow your own widgets to use colors in their drag and drop interaction.
            // Also see 'Demo->Widgets->Color/Picker Widgets->Palette' demo.
            HelpMarker(imgui_, "You can drag from the color squares.");
            static float col1[3] = { 1.0f, 0.0f, 0.2f };
            static float col2[4] = { 0.4f, 0.7f, 0.0f, 0.5f };
            imgui_.ColorEdit3("color 1", col1);
            imgui_.ColorEdit4("color 2", col2);
            imgui_.TreePop();
        }

        if (imgui_.TreeNode("Drag and drop to copy/swap items"))
        {
            enum Mode
            {
                Mode_Copy,
                Mode_Move,
                Mode_Swap
            };
            static int mode = 0;
            if (imgui_.RadioButton("Copy", mode == Mode_Copy)) { mode = Mode_Copy; } imgui_.SameLine();
            if (imgui_.RadioButton("Move", mode == Mode_Move)) { mode = Mode_Move; } imgui_.SameLine();
            if (imgui_.RadioButton("Swap", mode == Mode_Swap)) { mode = Mode_Swap; }
            static const char* names[9] =
            {
                "Bobby", "Beatrice", "Betty",
                "Brianna", "Barry", "Bernard",
                "Bibi", "Blaine", "Bryn"
            };
            for (int n = 0; n < IM_ARRAYSIZE(names); n++)
            {
                imgui_.PushID(n);
                if ((n % 3) != 0)
                    imgui_.SameLine();
                imgui_.Button(names[n], ImVec2(60, 60));

                // Our buttons are both drag sources and drag targets here!
                if (imgui_.BeginDragDropSource(ImGuiDragDropFlags_None))
                {
                    // Set payload to carry the index of our item (could be anything)
                    imgui_.SetDragDropPayload("DND_DEMO_CELL", &n, sizeof(int));

                    // Display preview (could be anything, e.g. when dragging an image we could decide to display
                    // the filename and a small preview of the image, etc.)
                    if (mode == Mode_Copy) { imgui_.Text("Copy %s", names[n]); }
                    if (mode == Mode_Move) { imgui_.Text("Move %s", names[n]); }
                    if (mode == Mode_Swap) { imgui_.Text("Swap %s", names[n]); }
                    imgui_.EndDragDropSource();
                }
                if (imgui_.BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = imgui_.AcceptDragDropPayload("DND_DEMO_CELL"))
                    {
                        IM_ASSERT(payload->DataSize == sizeof(int));
                        int payload_n = *(const int*)payload->Data;
                        if (mode == Mode_Copy)
                        {
                            names[n] = names[payload_n];
                        }
                        if (mode == Mode_Move)
                        {
                            names[n] = names[payload_n];
                            names[payload_n] = "";
                        }
                        if (mode == Mode_Swap)
                        {
                            const char* tmp = names[n];
                            names[n] = names[payload_n];
                            names[payload_n] = tmp;
                        }
                    }
                    imgui_.EndDragDropTarget();
                }
                imgui_.PopID();
            }
            imgui_.TreePop();
        }

        if (imgui_.TreeNode("Drag to reorder items (simple)"))
        {
            // Simple reordering
            HelpMarker(imgui_, 
                "We don't use the drag and drop api at all here! "
                "Instead we query when the item is held but not hovered, and order items accordingly.");
            static const char* item_names[] = { "Item One", "Item Two", "Item Three", "Item Four", "Item Five" };
            for (int n = 0; n < IM_ARRAYSIZE(item_names); n++)
            {
                const char* item = item_names[n];
                imgui_.Selectable(item);

                if (imgui_.IsItemActive() && !imgui_.IsItemHovered())
                {
                    int n_next = n + (imgui_.GetMouseDragDelta(0).y < 0.f ? -1 : 1);
                    if (n_next >= 0 && n_next < IM_ARRAYSIZE(item_names))
                    {
                        item_names[n] = item_names[n_next];
                        item_names[n_next] = item;
                        imgui_.ResetMouseDragDelta();
                    }
                }
            }
            imgui_.TreePop();
        }

        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Querying Status (Edited/Active/Focused/Hovered etc.)"))
    {
        // Select an item type
        const char* item_names[] =
        {
            "Text", "Button", "Button (w/ repeat)", "Checkbox", "SliderFloat", "InputText", "InputFloat",
            "InputFloat3", "ColorEdit4", "MenuItem", "TreeNode", "TreeNode (w/ double-click)", "Combo", "ListBox"
        };
        static int item_type = 1;
        imgui_.Combo("Item Type", &item_type, item_names, IM_ARRAYSIZE(item_names), IM_ARRAYSIZE(item_names));
        imgui_.SameLine();
        HelpMarker(imgui_, "Testing how various types of items are interacting with the IsItemXXX functions. Note that the bool return value of most ImGui function is generally equivalent to calling imgui_.IsItemHovered().");

        // Submit selected item item so we can query their status in the code following it.
        bool ret = false;
        static bool b = false;
        static float col4f[4] = { 1.0f, 0.5, 0.0f, 1.0f };
        static char str[16] = {};
        if (item_type == 0) { imgui_.Text("ITEM: Text"); }                                              // Testing text items with no identifier/interaction
        if (item_type == 1) { ret = imgui_.Button("ITEM: Button"); }                                    // Testing button
        if (item_type == 2) { imgui_.PushButtonRepeat(true); ret = imgui_.Button("ITEM: Button"); imgui_.PopButtonRepeat(); } // Testing button (with repeater)
        if (item_type == 3) { ret = imgui_.Checkbox("ITEM: Checkbox", &b); }                            // Testing checkbox
        if (item_type == 4) { ret = imgui_.SliderFloat("ITEM: SliderFloat", &col4f[0], 0.0f, 1.0f); }   // Testing basic item
        if (item_type == 5) { ret = imgui_.InputText("ITEM: InputText", &str[0], IM_ARRAYSIZE(str)); }  // Testing input text (which handles tabbing)
        if (item_type == 6) { ret = imgui_.InputFloat("ITEM: InputFloat", col4f, 1.0f); }               // Testing +/- buttons on scalar input
        if (item_type == 7) { ret = imgui_.InputFloat3("ITEM: InputFloat3", col4f); }                   // Testing multi-component items (IsItemXXX flags are reported merged)
        if (item_type == 8) { ret = imgui_.ColorEdit4("ITEM: ColorEdit4", col4f); }                     // Testing multi-component items (IsItemXXX flags are reported merged)
        if (item_type == 9) { ret = imgui_.MenuItem("ITEM: MenuItem"); }                                // Testing menu item (they use ImGuiButtonFlags_PressedOnRelease button policy)
        if (item_type == 10){ ret = imgui_.TreeNode("ITEM: TreeNode"); if (ret) imgui_.TreePop(); }     // Testing tree node
        if (item_type == 11){ ret = imgui_.TreeNodeEx("ITEM: TreeNode w/ ImGuiTreeNodeFlags_OpenOnDoubleClick", ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_NoTreePushOnOpen); } // Testing tree node with ImGuiButtonFlags_PressedOnDoubleClick button policy.
        if (item_type == 12){ const char* items[] = { "Apple", "Banana", "Cherry", "Kiwi" }; static int current = 1; ret = imgui_.Combo("ITEM: Combo", &current, items, IM_ARRAYSIZE(items)); }
        if (item_type == 13){ const char* items[] = { "Apple", "Banana", "Cherry", "Kiwi" }; static int current = 1; ret = imgui_.ListBox("ITEM: ListBox", &current, items, IM_ARRAYSIZE(items), IM_ARRAYSIZE(items)); }

        // Display the values of IsItemHovered() and other common item state functions.
        // Note that the ImGuiHoveredFlags_XXX flags can be combined.
        // Because BulletText is an item itself and that would affect the output of IsItemXXX functions,
        // we query every state in a single call to avoid storing them and to simplify the code.
        imgui_.BulletText(
            "Return value = %d\n"
            "IsItemFocused() = %d\n"
            "IsItemHovered() = %d\n"
            "IsItemHovered(_AllowWhenBlockedByPopup) = %d\n"
            "IsItemHovered(_AllowWhenBlockedByActiveItem) = %d\n"
            "IsItemHovered(_AllowWhenOverlapped) = %d\n"
            "IsItemHovered(_RectOnly) = %d\n"
            "IsItemActive() = %d\n"
            "IsItemEdited() = %d\n"
            "IsItemActivated() = %d\n"
            "IsItemDeactivated() = %d\n"
            "IsItemDeactivatedAfterEdit() = %d\n"
            "IsItemVisible() = %d\n"
            "IsItemClicked() = %d\n"
            "IsItemToggledOpen() = %d\n"
            "GetItemRectMin() = (%.1f, %.1f)\n"
            "GetItemRectMax() = (%.1f, %.1f)\n"
            "GetItemRectSize() = (%.1f, %.1f)",
            ret,
            imgui_.IsItemFocused(),
            imgui_.IsItemHovered(),
            imgui_.IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup),
            imgui_.IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem),
            imgui_.IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlapped),
            imgui_.IsItemHovered(ImGuiHoveredFlags_RectOnly),
            imgui_.IsItemActive(),
            imgui_.IsItemEdited(),
            imgui_.IsItemActivated(),
            imgui_.IsItemDeactivated(),
            imgui_.IsItemDeactivatedAfterEdit(),
            imgui_.IsItemVisible(),
            imgui_.IsItemClicked(),
            imgui_.IsItemToggledOpen(),
            imgui_.GetItemRectMin().x, imgui_.GetItemRectMin().y,
            imgui_.GetItemRectMax().x, imgui_.GetItemRectMax().y,
            imgui_.GetItemRectSize().x, imgui_.GetItemRectSize().y
        );

        static bool embed_all_inside_a_child_window = false;
        imgui_.Checkbox("Embed everything inside a child window (for additional testing)", &embed_all_inside_a_child_window);
        if (embed_all_inside_a_child_window)
            imgui_.BeginChild("outer_child", ImVec2(0, imgui_.GetFontSize() * 20.0f), true);

        // Testing IsWindowFocused() function with its various flags.
        // Note that the ImGuiFocusedFlags_XXX flags can be combined.
        imgui_.BulletText(
            "IsWindowFocused() = %d\n"
            "IsWindowFocused(_ChildWindows) = %d\n"
            "IsWindowFocused(_ChildWindows|_RootWindow) = %d\n"
            "IsWindowFocused(_RootWindow) = %d\n"
            "IsWindowFocused(_AnyWindow) = %d\n",
            imgui_.IsWindowFocused(),
            imgui_.IsWindowFocused(ImGuiFocusedFlags_ChildWindows),
            imgui_.IsWindowFocused(ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_RootWindow),
            imgui_.IsWindowFocused(ImGuiFocusedFlags_RootWindow),
            imgui_.IsWindowFocused(ImGuiFocusedFlags_AnyWindow));

        // Testing IsWindowHovered() function with its various flags.
        // Note that the ImGuiHoveredFlags_XXX flags can be combined.
        imgui_.BulletText(
            "IsWindowHovered() = %d\n"
            "IsWindowHovered(_AllowWhenBlockedByPopup) = %d\n"
            "IsWindowHovered(_AllowWhenBlockedByActiveItem) = %d\n"
            "IsWindowHovered(_ChildWindows) = %d\n"
            "IsWindowHovered(_ChildWindows|_RootWindow) = %d\n"
            "IsWindowHovered(_ChildWindows|_AllowWhenBlockedByPopup) = %d\n"
            "IsWindowHovered(_RootWindow) = %d\n"
            "IsWindowHovered(_AnyWindow) = %d\n",
            imgui_.IsWindowHovered(),
            imgui_.IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup),
            imgui_.IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem),
            imgui_.IsWindowHovered(ImGuiHoveredFlags_ChildWindows),
            imgui_.IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_RootWindow),
            imgui_.IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByPopup),
            imgui_.IsWindowHovered(ImGuiHoveredFlags_RootWindow),
            imgui_.IsWindowHovered(ImGuiHoveredFlags_AnyWindow));

        imgui_.BeginChild("child", ImVec2(0, 50), true);
        imgui_.Text("This is another child window for testing the _ChildWindows flag.");
        imgui_.EndChild();
        if (embed_all_inside_a_child_window)
            imgui_.EndChild();

        static char unused_str[] = "This widget is only here to be able to tab-out of the widgets above.";
        imgui_.InputText("unused", unused_str, IM_ARRAYSIZE(unused_str), ImGuiInputTextFlags_ReadOnly);

        // Calling IsItemHovered() after begin returns the hovered status of the title bar.
        // This is useful in particular if you want to create a context menu associated to the title bar of a window.
        static bool test_window = false;
        imgui_.Checkbox("Hovered/Active tests after Begin() for title bar testing", &test_window);
        if (test_window)
        {
            imgui_.Begin("Title bar Hovered/Active tests", &test_window);
            if (imgui_.BeginPopupContextItem()) // <-- This is using IsItemHovered()
            {
                if (imgui_.MenuItem("Close")) { test_window = false; }
                imgui_.EndPopup();
            }
            imgui_.Text(
                "IsItemHovered() after begin = %d (== is title bar hovered)\n"
                "IsItemActive() after begin = %d (== is window being clicked/moved)\n",
                imgui_.IsItemHovered(), imgui_.IsItemActive());
            imgui_.End();
        }

        imgui_.TreePop();
    }
}

static void ShowDemoWindowLayout(ImGui& imgui_)
{
    if (!imgui_.CollapsingHeader("Layout & Scrolling"))
        return;

    if (imgui_.TreeNode("Child windows"))
    {
        HelpMarker(imgui_, "Use child windows to begin into a self-contained independent scrolling/clipping regions within a host window.");
        static bool disable_mouse_wheel = false;
        static bool disable_menu = false;
        imgui_.Checkbox("Disable Mouse Wheel", &disable_mouse_wheel);
        imgui_.Checkbox("Disable Menu", &disable_menu);

        // Child 1: no border, enable horizontal scrollbar
        {
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
            if (disable_mouse_wheel)
                window_flags |= ImGuiWindowFlags_NoScrollWithMouse;
            imgui_.BeginChild("ChildL", ImVec2(imgui_.GetWindowContentRegionWidth() * 0.5f, 260), false, window_flags);
            for (int i = 0; i < 100; i++)
                imgui_.Text("%04d: scrollable region", i);
            imgui_.EndChild();
        }

        imgui_.SameLine();

        // Child 2: rounded border
        {
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
            if (disable_mouse_wheel)
                window_flags |= ImGuiWindowFlags_NoScrollWithMouse;
            if (!disable_menu)
                window_flags |= ImGuiWindowFlags_MenuBar;
            imgui_.PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
            imgui_.BeginChild("ChildR", ImVec2(0, 260), true, window_flags);
            if (!disable_menu && imgui_.BeginMenuBar())
            {
                if (imgui_.BeginMenu("Menu"))
                {
                    ShowExampleMenuFile(imgui_);
                    imgui_.EndMenu();
                }
                imgui_.EndMenuBar();
            }
            if (imgui_.BeginTable("split", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings))
            {
                for (int i = 0; i < 100; i++)
                {
                    char buf[32];
                    sprintf(buf, "%03d", i);
                    imgui_.TableNextColumn();
                    imgui_.Button(buf, ImVec2(-FLT_MIN, 0.0f));
                }
                imgui_.EndTable();
            }
            imgui_.EndChild();
            imgui_.PopStyleVar();
        }

        imgui_.Separator();

        // Demonstrate a few extra things
        // - Changing ImGuiCol_ChildBg (which is transparent black in default styles)
        // - Using SetCursorPos() to position child window (the child window is an item from the POV of parent window)
        //   You can also call SetNextWindowPos() to position the child window. The parent window will effectively
        //   layout from this position.
        // - Using imgui_.GetItemRectMin/Max() to query the "item" state (because the child window is an item from
        //   the POV of the parent window). See 'Demo->Querying Status (Active/Focused/Hovered etc.)' for details.
        {
            static int offset_x = 0;
            imgui_.SetNextItemWidth(100);
            imgui_.DragInt("Offset X", &offset_x, 1.0f, -1000, 1000);

            imgui_.SetCursorPosX(imgui_.GetCursorPosX() + (float)offset_x);
            imgui_.PushStyleColor(ImGuiCol_ChildBg, IM_COL32(255, 0, 0, 100));
            imgui_.BeginChild("Red", ImVec2(200, 100), true, ImGuiWindowFlags_None);
            for (int n = 0; n < 50; n++)
                imgui_.Text("Some test %d", n);
            imgui_.EndChild();
            bool child_is_hovered = imgui_.IsItemHovered();
            ImVec2 child_rect_min = imgui_.GetItemRectMin();
            ImVec2 child_rect_max = imgui_.GetItemRectMax();
            imgui_.PopStyleColor();
            imgui_.Text("Hovered: %d", child_is_hovered);
            imgui_.Text("Rect of child window is: (%.0f,%.0f) (%.0f,%.0f)", child_rect_min.x, child_rect_min.y, child_rect_max.x, child_rect_max.y);
        }

        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Widgets Width"))
    {
        // Use SetNextItemWidth() to set the width of a single upcoming item.
        // Use PushItemWidth()/PopItemWidth() to set the width of a group of items.
        // In real code use you'll probably want to choose width values that are proportional to your font size
        // e.g. Using '20.0f * GetFontSize()' as width instead of '200.0f', etc.

        static float f = 0.0f;
        static bool show_indented_items = true;
        imgui_.Checkbox("Show indented items", &show_indented_items);

        imgui_.Text("SetNextItemWidth/PushItemWidth(100)");
        imgui_.SameLine(); HelpMarker(imgui_, "Fixed width.");
        imgui_.PushItemWidth(100);
        imgui_.DragFloat("float##1b", &f);
        if (show_indented_items)
        {
            imgui_.Indent();
            imgui_.DragFloat("float (indented)##1b", &f);
            imgui_.Unindent();
        }
        imgui_.PopItemWidth();

        imgui_.Text("SetNextItemWidth/PushItemWidth(-100)");
        imgui_.SameLine(); HelpMarker(imgui_, "Align to right edge minus 100");
        imgui_.PushItemWidth(-100);
        imgui_.DragFloat("float##2a", &f);
        if (show_indented_items)
        {
            imgui_.Indent();
            imgui_.DragFloat("float (indented)##2b", &f);
            imgui_.Unindent();
        }
        imgui_.PopItemWidth();

        imgui_.Text("SetNextItemWidth/PushItemWidth(GetContentRegionAvail().x * 0.5f)");
        imgui_.SameLine(); HelpMarker(imgui_, "Half of available width.\n(~ right-cursor_pos)\n(works within a column set)");
        imgui_.PushItemWidth(imgui_.GetContentRegionAvail().x * 0.5f);
        imgui_.DragFloat("float##3a", &f);
        if (show_indented_items)
        {
            imgui_.Indent();
            imgui_.DragFloat("float (indented)##3b", &f);
            imgui_.Unindent();
        }
        imgui_.PopItemWidth();

        imgui_.Text("SetNextItemWidth/PushItemWidth(-GetContentRegionAvail().x * 0.5f)");
        imgui_.SameLine(); HelpMarker(imgui_, "Align to right edge minus half");
        imgui_.PushItemWidth(-imgui_.GetContentRegionAvail().x * 0.5f);
        imgui_.DragFloat("float##4a", &f);
        if (show_indented_items)
        {
            imgui_.Indent();
            imgui_.DragFloat("float (indented)##4b", &f);
            imgui_.Unindent();
        }
        imgui_.PopItemWidth();

        // Demonstrate using PushItemWidth to surround three items.
        // Calling SetNextItemWidth() before each of them would have the same effect.
        imgui_.Text("SetNextItemWidth/PushItemWidth(-FLT_MIN)");
        imgui_.SameLine(); HelpMarker(imgui_, "Align to right edge");
        imgui_.PushItemWidth(-FLT_MIN);
        imgui_.DragFloat("##float5a", &f);
        if (show_indented_items)
        {
            imgui_.Indent();
            imgui_.DragFloat("float (indented)##5b", &f);
            imgui_.Unindent();
        }
        imgui_.PopItemWidth();

        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Basic Horizontal Layout"))
    {
        imgui_.TextWrapped("(Use imgui_.SameLine() to keep adding items to the right of the preceding item)");

        // Text
        imgui_.Text("Two items: Hello"); imgui_.SameLine();
        imgui_.TextColored(ImVec4(1,1,0,1), "Sailor");

        // Adjust spacing
        imgui_.Text("More spacing: Hello"); imgui_.SameLine(0, 20);
        imgui_.TextColored(ImVec4(1,1,0,1), "Sailor");

        // Button
        imgui_.AlignTextToFramePadding();
        imgui_.Text("Normal buttons"); imgui_.SameLine();
        imgui_.Button("Banana"); imgui_.SameLine();
        imgui_.Button("Apple"); imgui_.SameLine();
        imgui_.Button("Corniflower");

        // Button
        imgui_.Text("Small buttons"); imgui_.SameLine();
        imgui_.SmallButton("Like this one"); imgui_.SameLine();
        imgui_.Text("can fit within a text block.");

        // Aligned to arbitrary position. Easy/cheap column.
        imgui_.Text("Aligned");
        imgui_.SameLine(150); imgui_.Text("x=150");
        imgui_.SameLine(300); imgui_.Text("x=300");
        imgui_.Text("Aligned");
        imgui_.SameLine(150); imgui_.SmallButton("x=150");
        imgui_.SameLine(300); imgui_.SmallButton("x=300");

        // Checkbox
        static bool c1 = false, c2 = false, c3 = false, c4 = false;
        imgui_.Checkbox("My", &c1); imgui_.SameLine();
        imgui_.Checkbox("Tailor", &c2); imgui_.SameLine();
        imgui_.Checkbox("Is", &c3); imgui_.SameLine();
        imgui_.Checkbox("Rich", &c4);

        // Various
        static float f0 = 1.0f, f1 = 2.0f, f2 = 3.0f;
        imgui_.PushItemWidth(80);
        const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD" };
        static int item = -1;
        imgui_.Combo("Combo", &item, items, IM_ARRAYSIZE(items)); imgui_.SameLine();
        imgui_.SliderFloat("X", &f0, 0.0f, 5.0f); imgui_.SameLine();
        imgui_.SliderFloat("Y", &f1, 0.0f, 5.0f); imgui_.SameLine();
        imgui_.SliderFloat("Z", &f2, 0.0f, 5.0f);
        imgui_.PopItemWidth();

        imgui_.PushItemWidth(80);
        imgui_.Text("Lists:");
        static int selection[4] = { 0, 1, 2, 3 };
        for (int i = 0; i < 4; i++)
        {
            if (i > 0) imgui_.SameLine();
            imgui_.PushID(i);
            imgui_.ListBox("", &selection[i], items, IM_ARRAYSIZE(items));
            imgui_.PopID();
            //if (imgui_.IsItemHovered()) imgui_.SetTooltip("ListBox %d hovered", i);
        }
        imgui_.PopItemWidth();

        // Dummy
        ImVec2 button_sz(40, 40);
        imgui_.Button("A", button_sz); imgui_.SameLine();
        imgui_.Dummy(button_sz); imgui_.SameLine();
        imgui_.Button("B", button_sz);

        // Manually wrapping
        // (we should eventually provide this as an automatic layout feature, but for now you can do it manually)
        imgui_.Text("Manually wrapping:");
        ImGuiStyle& style = imgui_.GetStyle();
        int buttons_count = 20;
        float window_visible_x2 = imgui_.GetWindowPos().x + imgui_.GetWindowContentRegionMax().x;
        for (int n = 0; n < buttons_count; n++)
        {
            imgui_.PushID(n);
            imgui_.Button("Box", button_sz);
            float last_button_x2 = imgui_.GetItemRectMax().x;
            float next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_sz.x; // Expected position if next button was on same line
            if (n + 1 < buttons_count && next_button_x2 < window_visible_x2)
                imgui_.SameLine();
            imgui_.PopID();
        }

        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Groups"))
    {
        HelpMarker(imgui_, 
            "BeginGroup() basically locks the horizontal position for new line. "
            "EndGroup() bundles the whole group so that you can use \"item\" functions such as "
            "IsItemHovered()/IsItemActive() or SameLine() etc. on the whole group.");
        imgui_.BeginGroup();
        {
            imgui_.BeginGroup();
            imgui_.Button("AAA");
            imgui_.SameLine();
            imgui_.Button("BBB");
            imgui_.SameLine();
            imgui_.BeginGroup();
            imgui_.Button("CCC");
            imgui_.Button("DDD");
            imgui_.EndGroup();
            imgui_.SameLine();
            imgui_.Button("EEE");
            imgui_.EndGroup();
            if (imgui_.IsItemHovered())
                imgui_.SetTooltip("First group hovered");
        }
        // Capture the group size and create widgets using the same size
        ImVec2 size = imgui_.GetItemRectSize();
        const float values[5] = { 0.5f, 0.20f, 0.80f, 0.60f, 0.25f };
        imgui_.PlotHistogram("##values", values, IM_ARRAYSIZE(values), 0, NULL, 0.0f, 1.0f, size);

        imgui_.Button("ACTION", ImVec2((size.x - imgui_.GetStyle().ItemSpacing.x) * 0.5f, size.y));
        imgui_.SameLine();
        imgui_.Button("REACTION", ImVec2((size.x - imgui_.GetStyle().ItemSpacing.x) * 0.5f, size.y));
        imgui_.EndGroup();
        imgui_.SameLine();

        imgui_.Button("LEVERAGE\nBUZZWORD", size);
        imgui_.SameLine();

        if (imgui_.BeginListBox("List", size))
        {
            imgui_.Selectable("Selected", true);
            imgui_.Selectable("Not Selected", false);
            imgui_.EndListBox();
        }

        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Text Baseline Alignment"))
    {
        {
            imgui_.BulletText("Text baseline:");
            imgui_.SameLine(); HelpMarker(imgui_, 
                "This is testing the vertical alignment that gets applied on text to keep it aligned with widgets. "
                "Lines only composed of text or \"small\" widgets use less vertical space than lines with framed widgets.");
            imgui_.Indent();

            imgui_.Text("KO Blahblah"); imgui_.SameLine();
            imgui_.Button("Some framed item"); imgui_.SameLine();
            HelpMarker(imgui_, "Baseline of button will look misaligned with text..");

            // If your line starts with text, call AlignTextToFramePadding() to align text to upcoming widgets.
            // (because we don't know what's coming after the Text() statement, we need to move the text baseline
            // down by FramePadding.y ahead of time)
            imgui_.AlignTextToFramePadding();
            imgui_.Text("OK Blahblah"); imgui_.SameLine();
            imgui_.Button("Some framed item"); imgui_.SameLine();
            HelpMarker(imgui_, "We call AlignTextToFramePadding() to vertically align the text baseline by +FramePadding.y");

            // SmallButton() uses the same vertical padding as Text
            imgui_.Button("TEST##1"); imgui_.SameLine();
            imgui_.Text("TEST"); imgui_.SameLine();
            imgui_.SmallButton("TEST##2");

            // If your line starts with text, call AlignTextToFramePadding() to align text to upcoming widgets.
            imgui_.AlignTextToFramePadding();
            imgui_.Text("Text aligned to framed item"); imgui_.SameLine();
            imgui_.Button("Item##1"); imgui_.SameLine();
            imgui_.Text("Item"); imgui_.SameLine();
            imgui_.SmallButton("Item##2"); imgui_.SameLine();
            imgui_.Button("Item##3");

            imgui_.Unindent();
        }

        imgui_.Spacing();

        {
            imgui_.BulletText("Multi-line text:");
            imgui_.Indent();
            imgui_.Text("One\nTwo\nThree"); imgui_.SameLine();
            imgui_.Text("Hello\nWorld"); imgui_.SameLine();
            imgui_.Text("Banana");

            imgui_.Text("Banana"); imgui_.SameLine();
            imgui_.Text("Hello\nWorld"); imgui_.SameLine();
            imgui_.Text("One\nTwo\nThree");

            imgui_.Button("HOP##1"); imgui_.SameLine();
            imgui_.Text("Banana"); imgui_.SameLine();
            imgui_.Text("Hello\nWorld"); imgui_.SameLine();
            imgui_.Text("Banana");

            imgui_.Button("HOP##2"); imgui_.SameLine();
            imgui_.Text("Hello\nWorld"); imgui_.SameLine();
            imgui_.Text("Banana");
            imgui_.Unindent();
        }

        imgui_.Spacing();

        {
            imgui_.BulletText("Misc items:");
            imgui_.Indent();

            // SmallButton() sets FramePadding to zero. Text baseline is aligned to match baseline of previous Button.
            imgui_.Button("80x80", ImVec2(80, 80));
            imgui_.SameLine();
            imgui_.Button("50x50", ImVec2(50, 50));
            imgui_.SameLine();
            imgui_.Button("Button()");
            imgui_.SameLine();
            imgui_.SmallButton("SmallButton()");

            // Tree
            const float spacing = imgui_.GetStyle().ItemInnerSpacing.x;
            imgui_.Button("Button##1");
            imgui_.SameLine(0.0f, spacing);
            if (imgui_.TreeNode("Node##1"))
            {
                // Placeholder tree data
                for (int i = 0; i < 6; i++)
                    imgui_.BulletText("Item %d..", i);
                imgui_.TreePop();
            }

            // Vertically align text node a bit lower so it'll be vertically centered with upcoming widget.
            // Otherwise you can use SmallButton() (smaller fit).
            imgui_.AlignTextToFramePadding();

            // Common mistake to avoid: if we want to SameLine after TreeNode we need to do it before we add
            // other contents below the node.
            bool node_open = imgui_.TreeNode("Node##2");
            imgui_.SameLine(0.0f, spacing); imgui_.Button("Button##2");
            if (node_open)
            {
                // Placeholder tree data
                for (int i = 0; i < 6; i++)
                    imgui_.BulletText("Item %d..", i);
                imgui_.TreePop();
            }

            // Bullet
            imgui_.Button("Button##3");
            imgui_.SameLine(0.0f, spacing);
            imgui_.BulletText("Bullet text");

            imgui_.AlignTextToFramePadding();
            imgui_.BulletText("Node");
            imgui_.SameLine(0.0f, spacing); imgui_.Button("Button##4");
            imgui_.Unindent();
        }

        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Scrolling"))
    {
        // Vertical scroll functions
        HelpMarker(imgui_, "Use SetScrollHereY() or SetScrollFromPosY() to scroll to a given vertical position.");

        static int track_item = 50;
        static bool enable_track = true;
        static bool enable_extra_decorations = false;
        static float scroll_to_off_px = 0.0f;
        static float scroll_to_pos_px = 200.0f;

        imgui_.Checkbox("Decoration", &enable_extra_decorations);

        imgui_.Checkbox("Track", &enable_track);
        imgui_.PushItemWidth(100);
        imgui_.SameLine(140); enable_track |= imgui_.DragInt("##item", &track_item, 0.25f, 0, 99, "Item = %d");

        bool scroll_to_off = imgui_.Button("Scroll Offset");
        imgui_.SameLine(140); scroll_to_off |= imgui_.DragFloat("##off", &scroll_to_off_px, 1.00f, 0, FLT_MAX, "+%.0f px");

        bool scroll_to_pos = imgui_.Button("Scroll To Pos");
        imgui_.SameLine(140); scroll_to_pos |= imgui_.DragFloat("##pos", &scroll_to_pos_px, 1.00f, -10, FLT_MAX, "X/Y = %.0f px");
        imgui_.PopItemWidth();

        if (scroll_to_off || scroll_to_pos)
            enable_track = false;

        ImGuiStyle& style = imgui_.GetStyle();
        float child_w = (imgui_.GetContentRegionAvail().x - 4 * style.ItemSpacing.x) / 5;
        if (child_w < 1.0f)
            child_w = 1.0f;
        imgui_.PushID("##VerticalScrolling");
        for (int i = 0; i < 5; i++)
        {
            if (i > 0) imgui_.SameLine();
            imgui_.BeginGroup();
            const char* names[] = { "Top", "25%", "Center", "75%", "Bottom" };
            imgui_.TextUnformatted(names[i]);

            const ImGuiWindowFlags child_flags = enable_extra_decorations ? ImGuiWindowFlags_MenuBar : 0;
            const ImGuiID child_id = imgui_.GetID((void*)(intptr_t)i);
            const bool child_is_visible = imgui_.BeginChild(child_id, ImVec2(child_w, 200.0f), true, child_flags);
            if (imgui_.BeginMenuBar())
            {
                imgui_.TextUnformatted("abc");
                imgui_.EndMenuBar();
            }
            if (scroll_to_off)
                imgui_.SetScrollY(scroll_to_off_px);
            if (scroll_to_pos)
                imgui_.SetScrollFromPosY(imgui_.GetCursorStartPos().y + scroll_to_pos_px, i * 0.25f);
            if (child_is_visible) // Avoid calling SetScrollHereY when running with culled items
            {
                for (int item = 0; item < 100; item++)
                {
                    if (enable_track && item == track_item)
                    {
                        imgui_.TextColored(ImVec4(1, 1, 0, 1), "Item %d", item);
                        imgui_.SetScrollHereY(i * 0.25f); // 0.0f:top, 0.5f:center, 1.0f:bottom
                    }
                    else
                    {
                        imgui_.Text("Item %d", item);
                    }
                }
            }
            float scroll_y = imgui_.GetScrollY();
            float scroll_max_y = imgui_.GetScrollMaxY();
            imgui_.EndChild();
            imgui_.Text("%.0f/%.0f", scroll_y, scroll_max_y);
            imgui_.EndGroup();
        }
        imgui_.PopID();

        // Horizontal scroll functions
        imgui_.Spacing();
        HelpMarker(imgui_, 
            "Use SetScrollHereX() or SetScrollFromPosX() to scroll to a given horizontal position.\n\n"
            "Because the clipping rectangle of most window hides half worth of WindowPadding on the "
            "left/right, using SetScrollFromPosX(+1) will usually result in clipped text whereas the "
            "equivalent SetScrollFromPosY(+1) wouldn't.");
        imgui_.PushID("##HorizontalScrolling");
        for (int i = 0; i < 5; i++)
        {
            float child_height = imgui_.GetTextLineHeight() + style.ScrollbarSize + style.WindowPadding.y * 2.0f;
            ImGuiWindowFlags child_flags = ImGuiWindowFlags_HorizontalScrollbar | (enable_extra_decorations ? ImGuiWindowFlags_AlwaysVerticalScrollbar : 0);
            ImGuiID child_id = imgui_.GetID((void*)(intptr_t)i);
            bool child_is_visible = imgui_.BeginChild(child_id, ImVec2(-100, child_height), true, child_flags);
            if (scroll_to_off)
                imgui_.SetScrollX(scroll_to_off_px);
            if (scroll_to_pos)
                imgui_.SetScrollFromPosX(imgui_.GetCursorStartPos().x + scroll_to_pos_px, i * 0.25f);
            if (child_is_visible) // Avoid calling SetScrollHereY when running with culled items
            {
                for (int item = 0; item < 100; item++)
                {
                    if (enable_track && item == track_item)
                    {
                        imgui_.TextColored(ImVec4(1, 1, 0, 1), "Item %d", item);
                        imgui_.SetScrollHereX(i * 0.25f); // 0.0f:left, 0.5f:center, 1.0f:right
                    }
                    else
                    {
                        imgui_.Text("Item %d", item);
                    }
                    imgui_.SameLine();
                }
            }
            float scroll_x = imgui_.GetScrollX();
            float scroll_max_x = imgui_.GetScrollMaxX();
            imgui_.EndChild();
            imgui_.SameLine();
            const char* names[] = { "Left", "25%", "Center", "75%", "Right" };
            imgui_.Text("%s\n%.0f/%.0f", names[i], scroll_x, scroll_max_x);
            imgui_.Spacing();
        }
        imgui_.PopID();

        // Miscellaneous Horizontal Scrolling Demo
        HelpMarker(imgui_, 
            "Horizontal scrolling for a window is enabled via the ImGuiWindowFlags_HorizontalScrollbar flag.\n\n"
            "You may want to also explicitly specify content width by using SetNextWindowContentWidth() before Begin().");
        static int lines = 7;
        imgui_.SliderInt("Lines", &lines, 1, 15);
        imgui_.PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        imgui_.PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
        ImVec2 scrolling_child_size = ImVec2(0, imgui_.GetFrameHeightWithSpacing() * 7 + 30);
        imgui_.BeginChild("scrolling", scrolling_child_size, true, ImGuiWindowFlags_HorizontalScrollbar);
        for (int line = 0; line < lines; line++)
        {
            // Display random stuff. For the sake of this trivial demo we are using basic Button() + SameLine()
            // If you want to create your own time line for a real application you may be better off manipulating
            // the cursor position yourself, aka using SetCursorPos/SetCursorScreenPos to position the widgets
            // yourself. You may also want to use the lower-level ImDrawList API.
            int num_buttons = 10 + ((line & 1) ? line * 9 : line * 3);
            for (int n = 0; n < num_buttons; n++)
            {
                if (n > 0) imgui_.SameLine();
                imgui_.PushID(n + line * 1000);
                char num_buf[16];
                sprintf(num_buf, "%d", n);
                const char* label = (!(n % 15)) ? "FizzBuzz" : (!(n % 3)) ? "Fizz" : (!(n % 5)) ? "Buzz" : num_buf;
                float hue = n * 0.05f;
                imgui_.PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue, 0.6f, 0.6f));
                imgui_.PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue, 0.7f, 0.7f));
                imgui_.PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue, 0.8f, 0.8f));
                imgui_.Button(label, ImVec2(40.0f + sinf((float)(line + n)) * 20.0f, 0.0f));
                imgui_.PopStyleColor(3);
                imgui_.PopID();
            }
        }
        float scroll_x = imgui_.GetScrollX();
        float scroll_max_x = imgui_.GetScrollMaxX();
        imgui_.EndChild();
        imgui_.PopStyleVar(2);
        float scroll_x_delta = 0.0f;
        imgui_.SmallButton("<<");
        if (imgui_.IsItemActive())
            scroll_x_delta = -imgui_.GetIO().DeltaTime * 1000.0f;
        imgui_.SameLine();
        imgui_.Text("Scroll from code"); imgui_.SameLine();
        imgui_.SmallButton(">>");
        if (imgui_.IsItemActive())
            scroll_x_delta = +imgui_.GetIO().DeltaTime * 1000.0f;
        imgui_.SameLine();
        imgui_.Text("%.0f/%.0f", scroll_x, scroll_max_x);
        if (scroll_x_delta != 0.0f)
        {
            // Demonstrate a trick: you can use Begin to set yourself in the context of another window
            // (here we are already out of your child window)
            imgui_.BeginChild("scrolling");
            imgui_.SetScrollX(imgui_.GetScrollX() + scroll_x_delta);
            imgui_.EndChild();
        }
        imgui_.Spacing();

        static bool show_horizontal_contents_size_demo_window = false;
        imgui_.Checkbox("Show Horizontal contents size demo window", &show_horizontal_contents_size_demo_window);

        if (show_horizontal_contents_size_demo_window)
        {
            static bool show_h_scrollbar = true;
            static bool show_button = true;
            static bool show_tree_nodes = true;
            static bool show_text_wrapped = false;
            static bool show_columns = true;
            static bool show_tab_bar = true;
            static bool show_child = false;
            static bool explicit_content_size = false;
            static float contents_size_x = 300.0f;
            if (explicit_content_size)
                imgui_.SetNextWindowContentSize(ImVec2(contents_size_x, 0.0f));
            imgui_.Begin("Horizontal contents size demo window", &show_horizontal_contents_size_demo_window, show_h_scrollbar ? ImGuiWindowFlags_HorizontalScrollbar : 0);
            imgui_.PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
            imgui_.PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 0));
            HelpMarker(imgui_, "Test of different widgets react and impact the work rectangle growing when horizontal scrolling is enabled.\n\nUse 'Metrics->Tools->Show windows rectangles' to visualize rectangles.");
            imgui_.Checkbox("H-scrollbar", &show_h_scrollbar);
            imgui_.Checkbox("Button", &show_button);            // Will grow contents size (unless explicitly overwritten)
            imgui_.Checkbox("Tree nodes", &show_tree_nodes);    // Will grow contents size and display highlight over full width
            imgui_.Checkbox("Text wrapped", &show_text_wrapped);// Will grow and use contents size
            imgui_.Checkbox("Columns", &show_columns);          // Will use contents size
            imgui_.Checkbox("Tab bar", &show_tab_bar);          // Will use contents size
            imgui_.Checkbox("Child", &show_child);              // Will grow and use contents size
            imgui_.Checkbox("Explicit content size", &explicit_content_size);
            imgui_.Text("Scroll %.1f/%.1f %.1f/%.1f", imgui_.GetScrollX(), imgui_.GetScrollMaxX(), imgui_.GetScrollY(), imgui_.GetScrollMaxY());
            if (explicit_content_size)
            {
                imgui_.SameLine();
                imgui_.SetNextItemWidth(100);
                imgui_.DragFloat("##csx", &contents_size_x);
                ImVec2 p = imgui_.GetCursorScreenPos();
                imgui_.GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 10, p.y + 10), IM_COL32_WHITE);
                imgui_.GetWindowDrawList()->AddRectFilled(ImVec2(p.x + contents_size_x - 10, p.y), ImVec2(p.x + contents_size_x, p.y + 10), IM_COL32_WHITE);
                imgui_.Dummy(ImVec2(0, 10));
            }
            imgui_.PopStyleVar(2);
            imgui_.Separator();
            if (show_button)
            {
                imgui_.Button("this is a 300-wide button", ImVec2(300, 0));
            }
            if (show_tree_nodes)
            {
                bool open = true;
                if (imgui_.TreeNode("this is a tree node"))
                {
                    if (imgui_.TreeNode("another one of those tree node..."))
                    {
                        imgui_.Text("Some tree contents");
                        imgui_.TreePop();
                    }
                    imgui_.TreePop();
                }
                imgui_.CollapsingHeader("CollapsingHeader", &open);
            }
            if (show_text_wrapped)
            {
                imgui_.TextWrapped("This text should automatically wrap on the edge of the work rectangle.");
            }
            if (show_columns)
            {
                imgui_.Text("Tables:");
                if (imgui_.BeginTable("table", 4, ImGuiTableFlags_Borders))
                {
                    for (int n = 0; n < 4; n++)
                    {
                        imgui_.TableNextColumn();
                        imgui_.Text("Width %.2f", imgui_.GetContentRegionAvail().x);
                    }
                    imgui_.EndTable();
                }
                imgui_.Text("Columns:");
                imgui_.Columns(4);
                for (int n = 0; n < 4; n++)
                {
                    imgui_.Text("Width %.2f", imgui_.GetColumnWidth());
                    imgui_.NextColumn();
                }
                imgui_.Columns(1);
            }
            if (show_tab_bar && imgui_.BeginTabBar("Hello"))
            {
                if (imgui_.BeginTabItem("OneOneOne")) { imgui_.EndTabItem(); }
                if (imgui_.BeginTabItem("TwoTwoTwo")) { imgui_.EndTabItem(); }
                if (imgui_.BeginTabItem("ThreeThreeThree")) { imgui_.EndTabItem(); }
                if (imgui_.BeginTabItem("FourFourFour")) { imgui_.EndTabItem(); }
                imgui_.EndTabBar();
            }
            if (show_child)
            {
                imgui_.BeginChild("child", ImVec2(0, 0), true);
                imgui_.EndChild();
            }
            imgui_.End();
        }

        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Clipping"))
    {
        static ImVec2 size(100.0f, 100.0f);
        static ImVec2 offset(30.0f, 30.0f);
        imgui_.DragFloat2("size", (float*)&size, 0.5f, 1.0f, 200.0f, "%.0f");
        imgui_.TextWrapped("(Click and drag to scroll)");

        for (int n = 0; n < 3; n++)
        {
            if (n > 0)
                imgui_.SameLine();
            imgui_.PushID(n);
            imgui_.BeginGroup(); // Lock X position

            imgui_.InvisibleButton("##empty", size);
            if (imgui_.IsItemActive() && imgui_.IsMouseDragging(ImGuiMouseButton_Left))
            {
                offset.x += imgui_.GetIO().MouseDelta.x;
                offset.y += imgui_.GetIO().MouseDelta.y;
            }
            const ImVec2 p0 = imgui_.GetItemRectMin();
            const ImVec2 p1 = imgui_.GetItemRectMax();
            const char* text_str = "Line 1 hello\nLine 2 clip me!";
            const ImVec2 text_pos = ImVec2(p0.x + offset.x, p0.y + offset.y);
            ImDrawList* draw_list = imgui_.GetWindowDrawList();

            switch (n)
            {
            case 0:
                HelpMarker(imgui_, 
                    "Using imgui_.PushClipRect():\n"
                    "Will alter ImGui hit-testing logic + ImDrawList rendering.\n"
                    "(use this if you want your clipping rectangle to affect interactions)");
                imgui_.PushClipRect(p0, p1, true);
                draw_list->AddRectFilled(p0, p1, IM_COL32(90, 90, 120, 255));
                draw_list->AddText(text_pos, IM_COL32_WHITE, text_str);
                imgui_.PopClipRect();
                break;
            case 1:
                HelpMarker(imgui_, 
                    "Using ImDrawList::PushClipRect():\n"
                    "Will alter ImDrawList rendering only.\n"
                    "(use this as a shortcut if you are only using ImDrawList calls)");
                draw_list->PushClipRect(p0, p1, true);
                draw_list->AddRectFilled(p0, p1, IM_COL32(90, 90, 120, 255));
                draw_list->AddText(text_pos, IM_COL32_WHITE, text_str);
                draw_list->PopClipRect();
                break;
            case 2:
                HelpMarker(imgui_, 
                    "Using ImDrawList::AddText() with a fine ClipRect:\n"
                    "Will alter only this specific ImDrawList::AddText() rendering.\n"
                    "(this is often used internally to avoid altering the clipping rectangle and minimize draw calls)");
                ImVec4 clip_rect(p0.x, p0.y, p1.x, p1.y); // AddText() takes a ImVec4* here so let's convert.
                draw_list->AddRectFilled(p0, p1, IM_COL32(90, 90, 120, 255));
                draw_list->AddText(imgui_.GetFont(), imgui_.GetFontSize(), text_pos, IM_COL32_WHITE, text_str, NULL, 0.0f, &clip_rect);
                break;
            }
            imgui_.EndGroup();
            imgui_.PopID();
        }

        imgui_.TreePop();
    }
}

static void ShowDemoWindowPopups(ImGui& imgui_)
{
    if (!imgui_.CollapsingHeader("Popups & Modal windows"))
        return;

    // The properties of popups windows are:
    // - They block normal mouse hovering detection outside them. (*)
    // - Unless modal, they can be closed by clicking anywhere outside them, or by pressing ESCAPE.
    // - Their visibility state (~bool) is held internally by Dear ImGui instead of being held by the programmer as
    //   we are used to with regular Begin() calls. User can manipulate the visibility state by calling OpenPopup().
    // (*) One can use IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) to bypass it and detect hovering even
    //     when normally blocked by a popup.
    // Those three properties are connected. The library needs to hold their visibility state BECAUSE it can close
    // popups at any time.

    // Typical use for regular windows:
    //   bool my_tool_is_active = false; if (imgui_.Button("Open")) my_tool_is_active = true; [...] if (my_tool_is_active) Begin("My Tool", &my_tool_is_active) { [...] } End();
    // Typical use for popups:
    //   if (imgui_.Button("Open")) imgui_.OpenPopup("MyPopup"); if (imgui_.BeginPopup("MyPopup") { [...] EndPopup(); }

    // With popups we have to go through a library call (here OpenPopup) to manipulate the visibility state.
    // This may be a bit confusing at first but it should quickly make sense. Follow on the examples below.

    if (imgui_.TreeNode("Popups"))
    {
        imgui_.TextWrapped(
            "When a popup is active, it inhibits interacting with windows that are behind the popup. "
            "Clicking outside the popup closes it.");

        static int selected_fish = -1;
        const char* names[] = { "Bream", "Haddock", "Mackerel", "Pollock", "Tilefish" };
        static bool toggles[] = { true, false, false, false, false };

        // Simple selection popup (if you want to show the current selection inside the Button itself,
        // you may want to build a string using the "###" operator to preserve a constant ID with a variable label)
        if (imgui_.Button("Select.."))
            imgui_.OpenPopup("my_select_popup");
        imgui_.SameLine();
        imgui_.TextUnformatted(selected_fish == -1 ? "<None>" : names[selected_fish]);
        if (imgui_.BeginPopup("my_select_popup"))
        {
            imgui_.Text("Aquarium");
            imgui_.Separator();
            for (int i = 0; i < IM_ARRAYSIZE(names); i++)
                if (imgui_.Selectable(names[i]))
                    selected_fish = i;
            imgui_.EndPopup();
        }

        // Showing a menu with toggles
        if (imgui_.Button("Toggle.."))
            imgui_.OpenPopup("my_toggle_popup");
        if (imgui_.BeginPopup("my_toggle_popup"))
        {
            for (int i = 0; i < IM_ARRAYSIZE(names); i++)
                imgui_.MenuItem(names[i], "", &toggles[i]);
            if (imgui_.BeginMenu("Sub-menu"))
            {
                imgui_.MenuItem("Click me");
                imgui_.EndMenu();
            }

            imgui_.Separator();
            imgui_.Text("Tooltip here");
            if (imgui_.IsItemHovered())
                imgui_.SetTooltip("I am a tooltip over a popup");

            if (imgui_.Button("Stacked Popup"))
                imgui_.OpenPopup("another popup");
            if (imgui_.BeginPopup("another popup"))
            {
                for (int i = 0; i < IM_ARRAYSIZE(names); i++)
                    imgui_.MenuItem(names[i], "", &toggles[i]);
                if (imgui_.BeginMenu("Sub-menu"))
                {
                    imgui_.MenuItem("Click me");
                    if (imgui_.Button("Stacked Popup"))
                        imgui_.OpenPopup("another popup");
                    if (imgui_.BeginPopup("another popup"))
                    {
                        imgui_.Text("I am the last one here.");
                        imgui_.EndPopup();
                    }
                    imgui_.EndMenu();
                }
                imgui_.EndPopup();
            }
            imgui_.EndPopup();
        }

        // Call the more complete ShowExampleMenuFile which we use in various places of this demo
        if (imgui_.Button("File Menu.."))
            imgui_.OpenPopup("my_file_popup");
        if (imgui_.BeginPopup("my_file_popup"))
        {
            ShowExampleMenuFile(imgui_);
            imgui_.EndPopup();
        }

        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Context menus"))
    {
        // BeginPopupContextItem() is a helper to provide common/simple popup behavior of essentially doing:
        //    if (IsItemHovered() && IsMouseReleased(ImGuiMouseButton_Right))
        //       OpenPopup(id);
        //    return BeginPopup(id);
        // For more advanced uses you may want to replicate and customize this code.
        // See details in BeginPopupContextItem().
        static float value = 0.5f;
        imgui_.Text("Value = %.3f (<-- right-click here)", value);
        if (imgui_.BeginPopupContextItem("item context menu"))
        {
            if (imgui_.Selectable("Set to zero")) value = 0.0f;
            if (imgui_.Selectable("Set to PI")) value = 3.1415f;
            imgui_.SetNextItemWidth(-FLT_MIN);
            imgui_.DragFloat("##Value", &value, 0.1f, 0.0f, 0.0f);
            imgui_.EndPopup();
        }

        // We can also use OpenPopupOnItemClick() which is the same as BeginPopupContextItem() but without the
        // Begin() call. So here we will make it that clicking on the text field with the right mouse button (1)
        // will toggle the visibility of the popup above.
        imgui_.Text("(You can also right-click me to open the same popup as above.)");
        imgui_.OpenPopupOnItemClick("item context menu", 1);

        // When used after an item that has an ID (e.g.Button), we can skip providing an ID to BeginPopupContextItem().
        // BeginPopupContextItem() will use the last item ID as the popup ID.
        // In addition here, we want to include your editable label inside the button label.
        // We use the ### operator to override the ID (read FAQ about ID for details)
        static char name[32] = "Label1";
        char buf[64];
        sprintf(buf, "Button: %s###Button", name); // ### operator override ID ignoring the preceding label
        imgui_.Button(buf);
        if (imgui_.BeginPopupContextItem())
        {
            imgui_.Text("Edit name:");
            imgui_.InputText("##edit", name, IM_ARRAYSIZE(name));
            if (imgui_.Button("Close"))
                imgui_.CloseCurrentPopup();
            imgui_.EndPopup();
        }
        imgui_.SameLine(); imgui_.Text("(<-- right-click here)");

        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Modals"))
    {
        imgui_.TextWrapped("Modal windows are like popups but the user cannot close them by clicking outside.");

        if (imgui_.Button("Delete.."))
            imgui_.OpenPopup("Delete?");

        // Always center this window when appearing
        ImVec2 center = imgui_.GetMainViewport()->GetCenter();
        imgui_.SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (imgui_.BeginPopupModal("Delete?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            imgui_.Text("All those beautiful files will be deleted.\nThis operation cannot be undone!\n\n");
            imgui_.Separator();

            //static int unused_i = 0;
            //imgui_.Combo("Combo", &unused_i, "Delete\0Delete harder\0");

            static bool dont_ask_me_next_time = false;
            imgui_.PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            imgui_.Checkbox("Don't ask me next time", &dont_ask_me_next_time);
            imgui_.PopStyleVar();

            if (imgui_.Button("OK", ImVec2(120, 0))) { imgui_.CloseCurrentPopup(); }
            imgui_.SetItemDefaultFocus();
            imgui_.SameLine();
            if (imgui_.Button("Cancel", ImVec2(120, 0))) { imgui_.CloseCurrentPopup(); }
            imgui_.EndPopup();
        }

        if (imgui_.Button("Stacked modals.."))
            imgui_.OpenPopup("Stacked 1");
        if (imgui_.BeginPopupModal("Stacked 1", NULL, ImGuiWindowFlags_MenuBar))
        {
            if (imgui_.BeginMenuBar())
            {
                if (imgui_.BeginMenu("File"))
                {
                    if (imgui_.MenuItem("Some menu item")) {}
                    imgui_.EndMenu();
                }
                imgui_.EndMenuBar();
            }
            imgui_.Text("Hello from Stacked The First\nUsing style.Colors[ImGuiCol_ModalWindowDimBg] behind it.");

            // Testing behavior of widgets stacking their own regular popups over the modal.
            static int item = 1;
            static float color[4] = { 0.4f, 0.7f, 0.0f, 0.5f };
            imgui_.Combo("Combo", &item, "aaaa\0bbbb\0cccc\0dddd\0eeee\0\0");
            imgui_.ColorEdit4("color", color);

            if (imgui_.Button("Add another modal.."))
                imgui_.OpenPopup("Stacked 2");

            // Also demonstrate passing a bool* to BeginPopupModal(), this will create a regular close button which
            // will close the popup. Note that the visibility state of popups is owned by imgui, so the input value
            // of the bool actually doesn't matter here.
            bool unused_open = true;
            if (imgui_.BeginPopupModal("Stacked 2", &unused_open))
            {
                imgui_.Text("Hello from Stacked The Second!");
                if (imgui_.Button("Close"))
                    imgui_.CloseCurrentPopup();
                imgui_.EndPopup();
            }

            if (imgui_.Button("Close"))
                imgui_.CloseCurrentPopup();
            imgui_.EndPopup();
        }

        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Menus inside a regular window"))
    {
        imgui_.TextWrapped("Below we are testing adding menu items to a regular window. It's rather unusual but should work!");
        imgui_.Separator();

        // Note: As a quirk in this very specific example, we want to differentiate the parent of this menu from the
        // parent of the various popup menus above. To do so we are encloding the items in a PushID()/PopID() block
        // to make them two different menusets. If we don't, opening any popup above and hovering our menu here would
        // open it. This is because once a menu is active, we allow to switch to a sibling menu by just hovering on it,
        // which is the desired behavior for regular menus.
        imgui_.PushID("foo");
        imgui_.MenuItem("Menu item", "CTRL+M");
        if (imgui_.BeginMenu("Menu inside a regular window"))
        {
            ShowExampleMenuFile(imgui_);
            imgui_.EndMenu();
        }
        imgui_.PopID();
        imgui_.Separator();
        imgui_.TreePop();
    }
}

// Dummy data structure that we use for the Table demo.
// (pre-C++11 doesn't allow us to instantiate ImVector<MyItem> template if this structure if defined inside the demo function)
namespace
{
// We are passing our own identifier to TableSetupColumn() to facilitate identifying columns in the sorting code.
// This identifier will be passed down into ImGuiTableSortSpec::ColumnUserID.
// But it is possible to omit the user id parameter of TableSetupColumn() and just use the column index instead! (ImGuiTableSortSpec::ColumnIndex)
// If you don't use sorting, you will generally never care about giving column an ID!
enum MyItemColumnID
{
    MyItemColumnID_ID,
    MyItemColumnID_Name,
    MyItemColumnID_Action,
    MyItemColumnID_Quantity,
    MyItemColumnID_Description
};

struct MyItem
{
    int         ID;
    const char* Name;
    int         Quantity;

    // We have a problem which is affecting _only this demo_ and should not affect your code:
    // As we don't rely on std:: or other third-party library to compile dear imgui, we only have reliable access to qsort(),
    // however qsort doesn't allow passing user data to comparing function.
    // As a workaround, we are storing the sort specs in a static/global for the comparing function to access.
    // In your own use case you would probably pass the sort specs to your sorting/comparing functions directly and not use a global.
    // We could technically call ImGui::TableGetSortSpecs() in CompareWithSortSpecs(), but considering that this function is called
    // very often by the sorting algorithm it would be a little wasteful.
    static const ImGuiTableSortSpecs* s_current_sort_specs;

    // Compare function to be used by qsort()
    static int IMGUI_CDECL CompareWithSortSpecs(const void* lhs, const void* rhs)
    {
        const MyItem* a = (const MyItem*)lhs;
        const MyItem* b = (const MyItem*)rhs;
        for (int n = 0; n < s_current_sort_specs->SpecsCount; n++)
        {
            // Here we identify columns using the ColumnUserID value that we ourselves passed to TableSetupColumn()
            // We could also choose to identify columns based on their index (sort_spec->ColumnIndex), which is simpler!
            const ImGuiTableColumnSortSpecs* sort_spec = &s_current_sort_specs->Specs[n];
            int delta = 0;
            switch (sort_spec->ColumnUserID)
            {
            case MyItemColumnID_ID:             delta = (a->ID - b->ID);                break;
            case MyItemColumnID_Name:           delta = (strcmp(a->Name, b->Name));     break;
            case MyItemColumnID_Quantity:       delta = (a->Quantity - b->Quantity);    break;
            case MyItemColumnID_Description:    delta = (strcmp(a->Name, b->Name));     break;
            default: IM_ASSERT(0); break;
            }
            if (delta > 0)
                return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? +1 : -1;
            if (delta < 0)
                return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? -1 : +1;
        }

        // qsort() is instable so always return a way to differenciate items.
        // Your own compare function may want to avoid fallback on implicit sort specs e.g. a Name compare if it wasn't already part of the sort specs.
        return (a->ID - b->ID);
    }
};
const ImGuiTableSortSpecs* MyItem::s_current_sort_specs = NULL;
}

// Make the UI compact because there are so many fields
static void PushStyleCompact(ImGui& imgui_)
{
    ImGuiStyle& style = imgui_.GetStyle();
    imgui_.PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, (float)(int)(style.FramePadding.y * 0.60f)));
    imgui_.PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, (float)(int)(style.ItemSpacing.y * 0.60f)));
}

static void PopStyleCompact(ImGui& imgui_)
{
    imgui_.PopStyleVar(2);
}

// Show a combo box with a choice of sizing policies
static void EditTableSizingFlags(ImGui& imgui_, ImGuiTableFlags* p_flags)
{
    struct EnumDesc { ImGuiTableFlags Value; const char* Name; const char* Tooltip; };
    static const EnumDesc policies[] =
    {
        { ImGuiTableFlags_None,               "Default",                            "Use default sizing policy:\n- ImGuiTableFlags_SizingFixedFit if ScrollX is on or if host window has ImGuiWindowFlags_AlwaysAutoResize.\n- ImGuiTableFlags_SizingStretchSame otherwise." },
        { ImGuiTableFlags_SizingFixedFit,     "ImGuiTableFlags_SizingFixedFit",     "Columns default to _WidthFixed (if resizable) or _WidthAuto (if not resizable), matching contents width." },
        { ImGuiTableFlags_SizingFixedSame,    "ImGuiTableFlags_SizingFixedSame",    "Columns are all the same width, matching the maximum contents width.\nImplicitly disable ImGuiTableFlags_Resizable and enable ImGuiTableFlags_NoKeepColumnsVisible." },
        { ImGuiTableFlags_SizingStretchProp,  "ImGuiTableFlags_SizingStretchProp",  "Columns default to _WidthStretch with weights proportional to their widths." },
        { ImGuiTableFlags_SizingStretchSame,  "ImGuiTableFlags_SizingStretchSame",  "Columns default to _WidthStretch with same weights." }
    };
    int idx;
    for (idx = 0; idx < IM_ARRAYSIZE(policies); idx++)
        if (policies[idx].Value == (*p_flags & ImGuiTableFlags_SizingMask_))
            break;
    const char* preview_text = (idx < IM_ARRAYSIZE(policies)) ? policies[idx].Name + (idx > 0 ? strlen("ImGuiTableFlags") : 0) : "";
    if (imgui_.BeginCombo("Sizing Policy", preview_text))
    {
        for (int n = 0; n < IM_ARRAYSIZE(policies); n++)
            if (imgui_.Selectable(policies[n].Name, idx == n))
                *p_flags = (*p_flags & ~ImGuiTableFlags_SizingMask_) | policies[n].Value;
        imgui_.EndCombo();
    }
    imgui_.SameLine();
    imgui_.TextDisabled("(?)");
    if (imgui_.IsItemHovered())
    {
        imgui_.BeginTooltip();
        imgui_.PushTextWrapPos(imgui_.GetFontSize() * 50.0f);
        for (int m = 0; m < IM_ARRAYSIZE(policies); m++)
        {
            imgui_.Separator();
            imgui_.Text("%s:", policies[m].Name);
            imgui_.Separator();
            imgui_.SetCursorPosX(imgui_.GetCursorPosX() + imgui_.GetStyle().IndentSpacing * 0.5f);
            imgui_.TextUnformatted(policies[m].Tooltip);
        }
        imgui_.PopTextWrapPos();
        imgui_.EndTooltip();
    }
}

static void EditTableColumnsFlags(ImGui& imgui_, ImGuiTableColumnFlags* p_flags)
{
    imgui_.CheckboxFlags("_DefaultHide", p_flags, ImGuiTableColumnFlags_DefaultHide);
    imgui_.CheckboxFlags("_DefaultSort", p_flags, ImGuiTableColumnFlags_DefaultSort);
    if (imgui_.CheckboxFlags("_WidthStretch", p_flags, ImGuiTableColumnFlags_WidthStretch))
        *p_flags &= ~(ImGuiTableColumnFlags_WidthMask_ ^ ImGuiTableColumnFlags_WidthStretch);
    if (imgui_.CheckboxFlags("_WidthFixed", p_flags, ImGuiTableColumnFlags_WidthFixed))
        *p_flags &= ~(ImGuiTableColumnFlags_WidthMask_ ^ ImGuiTableColumnFlags_WidthFixed);
    imgui_.CheckboxFlags("_NoResize", p_flags, ImGuiTableColumnFlags_NoResize);
    imgui_.CheckboxFlags("_NoReorder", p_flags, ImGuiTableColumnFlags_NoReorder);
    imgui_.CheckboxFlags("_NoHide", p_flags, ImGuiTableColumnFlags_NoHide);
    imgui_.CheckboxFlags("_NoClip", p_flags, ImGuiTableColumnFlags_NoClip);
    imgui_.CheckboxFlags("_NoSort", p_flags, ImGuiTableColumnFlags_NoSort);
    imgui_.CheckboxFlags("_NoSortAscending", p_flags, ImGuiTableColumnFlags_NoSortAscending);
    imgui_.CheckboxFlags("_NoSortDescending", p_flags, ImGuiTableColumnFlags_NoSortDescending);
    imgui_.CheckboxFlags("_NoHeaderWidth", p_flags, ImGuiTableColumnFlags_NoHeaderWidth);
    imgui_.CheckboxFlags("_PreferSortAscending", p_flags, ImGuiTableColumnFlags_PreferSortAscending);
    imgui_.CheckboxFlags("_PreferSortDescending", p_flags, ImGuiTableColumnFlags_PreferSortDescending);
    imgui_.CheckboxFlags("_IndentEnable", p_flags, ImGuiTableColumnFlags_IndentEnable); imgui_.SameLine(); HelpMarker(imgui_, "Default for column 0");
    imgui_.CheckboxFlags("_IndentDisable", p_flags, ImGuiTableColumnFlags_IndentDisable); imgui_.SameLine(); HelpMarker(imgui_, "Default for column >0");
}

static void ShowTableColumnsStatusFlags(ImGui& imgui_, ImGuiTableColumnFlags flags)
{
    imgui_.CheckboxFlags("_IsEnabled", &flags, ImGuiTableColumnFlags_IsEnabled);
    imgui_.CheckboxFlags("_IsVisible", &flags, ImGuiTableColumnFlags_IsVisible);
    imgui_.CheckboxFlags("_IsSorted", &flags, ImGuiTableColumnFlags_IsSorted);
    imgui_.CheckboxFlags("_IsHovered", &flags, ImGuiTableColumnFlags_IsHovered);
}

static void ShowDemoWindowTables(ImGui& imgui_)
{
    //imgui_.SetNextItemOpen(true, ImGuiCond_Once);
    if (!imgui_.CollapsingHeader("Tables & Columns"))
        return;

    // Using those as a base value to create width/height that are factor of the size of our font
    const float TEXT_BASE_WIDTH = imgui_.CalcTextSize("A").x;
    const float TEXT_BASE_HEIGHT = imgui_.GetTextLineHeightWithSpacing();

    imgui_.PushID("Tables");

    int open_action = -1;
    if (imgui_.Button("Open all"))
        open_action = 1;
    imgui_.SameLine();
    if (imgui_.Button("Close all"))
        open_action = 0;
    imgui_.SameLine();

    // Options
    static bool disable_indent = false;
    imgui_.Checkbox("Disable tree indentation", &disable_indent);
    imgui_.SameLine();
    HelpMarker(imgui_, "Disable the indenting of tree nodes so demo tables can use the full window width.");
    imgui_.Separator();
    if (disable_indent)
        imgui_.PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);

    // About Styling of tables
    // Most settings are configured on a per-table basis via the flags passed to BeginTable() and TableSetupColumns APIs.
    // There are however a few settings that a shared and part of the ImGuiStyle structure:
    //   style.CellPadding                          // Padding within each cell
    //   style.Colors[ImGuiCol_TableHeaderBg]       // Table header background
    //   style.Colors[ImGuiCol_TableBorderStrong]   // Table outer and header borders
    //   style.Colors[ImGuiCol_TableBorderLight]    // Table inner borders
    //   style.Colors[ImGuiCol_TableRowBg]          // Table row background when ImGuiTableFlags_RowBg is enabled (even rows)
    //   style.Colors[ImGuiCol_TableRowBgAlt]       // Table row background when ImGuiTableFlags_RowBg is enabled (odds rows)

    // Demos
    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Basic"))
    {
        // Here we will showcase three different ways to output a table.
        // They are very simple variations of a same thing!

        // [Method 1] Using TableNextRow() to create a new row, and TableSetColumnIndex() to select the column.
        // In many situations, this is the most flexible and easy to use pattern.
        HelpMarker(imgui_, "Using TableNextRow() + calling TableSetColumnIndex() _before_ each cell, in a loop.");
        if (imgui_.BeginTable("table1", 3))
        {
            for (int row = 0; row < 4; row++)
            {
                imgui_.TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    imgui_.TableSetColumnIndex(column);
                    imgui_.Text("Row %d Column %d", row, column);
                }
            }
            imgui_.EndTable();
        }

        // [Method 2] Using TableNextColumn() called multiple times, instead of using a for loop + TableSetColumnIndex().
        // This is generally more convenient when you have code manually submitting the contents of each columns.
        HelpMarker(imgui_, "Using TableNextRow() + calling TableNextColumn() _before_ each cell, manually.");
        if (imgui_.BeginTable("table2", 3))
        {
            for (int row = 0; row < 4; row++)
            {
                imgui_.TableNextRow();
                imgui_.TableNextColumn();
                imgui_.Text("Row %d", row);
                imgui_.TableNextColumn();
                imgui_.Text("Some contents");
                imgui_.TableNextColumn();
                imgui_.Text("123.456");
            }
            imgui_.EndTable();
        }

        // [Method 3] We call TableNextColumn() _before_ each cell. We never call TableNextRow(),
        // as TableNextColumn() will automatically wrap around and create new roes as needed.
        // This is generally more convenient when your cells all contains the same type of data.
        HelpMarker(imgui_,
            "Only using TableNextColumn(), which tends to be convenient for tables where every cells contains the same type of contents.\n"
            "This is also more similar to the old NextColumn() function of the Columns API, and provided to facilitate the Columns->Tables API transition.");
        if (imgui_.BeginTable("table3", 3))
        {
            for (int item = 0; item < 14; item++)
            {
                imgui_.TableNextColumn();
                imgui_.Text("Item %d", item);
            }
            imgui_.EndTable();
        }

        imgui_.TreePop();
    }

    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Borders, background"))
    {
        // Expose a few Borders related flags interactively
        enum ContentsType { CT_Text, CT_FillButton };
        static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
        static bool display_headers = false;
        static int contents_type = CT_Text;

        PushStyleCompact(imgui_);
        imgui_.CheckboxFlags("ImGuiTableFlags_RowBg", &flags, ImGuiTableFlags_RowBg);
        imgui_.CheckboxFlags("ImGuiTableFlags_Borders", &flags, ImGuiTableFlags_Borders);
        imgui_.SameLine(); HelpMarker(imgui_, "ImGuiTableFlags_Borders\n = ImGuiTableFlags_BordersInnerV\n | ImGuiTableFlags_BordersOuterV\n | ImGuiTableFlags_BordersInnerV\n | ImGuiTableFlags_BordersOuterH");
        imgui_.Indent();

        imgui_.CheckboxFlags("ImGuiTableFlags_BordersH", &flags, ImGuiTableFlags_BordersH);
        imgui_.Indent();
        imgui_.CheckboxFlags("ImGuiTableFlags_BordersOuterH", &flags, ImGuiTableFlags_BordersOuterH);
        imgui_.CheckboxFlags("ImGuiTableFlags_BordersInnerH", &flags, ImGuiTableFlags_BordersInnerH);
        imgui_.Unindent();

        imgui_.CheckboxFlags("ImGuiTableFlags_BordersV", &flags, ImGuiTableFlags_BordersV);
        imgui_.Indent();
        imgui_.CheckboxFlags("ImGuiTableFlags_BordersOuterV", &flags, ImGuiTableFlags_BordersOuterV);
        imgui_.CheckboxFlags("ImGuiTableFlags_BordersInnerV", &flags, ImGuiTableFlags_BordersInnerV);
        imgui_.Unindent();

        imgui_.CheckboxFlags("ImGuiTableFlags_BordersOuter", &flags, ImGuiTableFlags_BordersOuter);
        imgui_.CheckboxFlags("ImGuiTableFlags_BordersInner", &flags, ImGuiTableFlags_BordersInner);
        imgui_.Unindent();

        imgui_.AlignTextToFramePadding(); imgui_.Text("Cell contents:");
        imgui_.SameLine(); imgui_.RadioButton("Text", &contents_type, CT_Text);
        imgui_.SameLine(); imgui_.RadioButton("FillButton", &contents_type, CT_FillButton);
        imgui_.Checkbox("Display headers", &display_headers);
        imgui_.CheckboxFlags("ImGuiTableFlags_NoBordersInBody", &flags, ImGuiTableFlags_NoBordersInBody); imgui_.SameLine(); HelpMarker(imgui_, "Disable vertical borders in columns Body (borders will always appears in Headers");
        PopStyleCompact(imgui_);

        if (imgui_.BeginTable("table1", 3, flags))
        {
            // Display headers so we can inspect their interaction with borders.
            // (Headers are not the main purpose of this section of the demo, so we are not elaborating on them too much. See other sections for details)
            if (display_headers)
            {
                imgui_.TableSetupColumn("One");
                imgui_.TableSetupColumn("Two");
                imgui_.TableSetupColumn("Three");
                imgui_.TableHeadersRow();
            }

            for (int row = 0; row < 5; row++)
            {
                imgui_.TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    imgui_.TableSetColumnIndex(column);
                    char buf[32];
                    sprintf(buf, "Hello %d,%d", column, row);
                    if (contents_type == CT_Text)
                        imgui_.TextUnformatted(buf);
                    else if (contents_type)
                        imgui_.Button(buf, ImVec2(-FLT_MIN, 0.0f));
                }
            }
            imgui_.EndTable();
        }
        imgui_.TreePop();
    }

    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Resizable, stretch"))
    {
        // By default, if we don't enable ScrollX the sizing policy for each columns is "Stretch"
        // Each columns maintain a sizing weight, and they will occupy all available width.
        static ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_ContextMenuInBody;
        PushStyleCompact(imgui_);
        imgui_.CheckboxFlags("ImGuiTableFlags_Resizable", &flags, ImGuiTableFlags_Resizable);
        imgui_.CheckboxFlags("ImGuiTableFlags_BordersV", &flags, ImGuiTableFlags_BordersV);
        imgui_.SameLine(); HelpMarker(imgui_, "Using the _Resizable flag automatically enables the _BordersInnerV flag as well, this is why the resize borders are still showing when unchecking this.");
        PopStyleCompact(imgui_);

        if (imgui_.BeginTable("table1", 3, flags))
        {
            for (int row = 0; row < 5; row++)
            {
                imgui_.TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    imgui_.TableSetColumnIndex(column);
                    imgui_.Text("Hello %d,%d", column, row);
                }
            }
            imgui_.EndTable();
        }
        imgui_.TreePop();
    }

    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Resizable, fixed"))
    {
        // Here we use ImGuiTableFlags_SizingFixedFit (even though _ScrollX is not set)
        // So columns will adopt the "Fixed" policy and will maintain a fixed width regardless of the whole available width (unless table is small)
        // If there is not enough available width to fit all columns, they will however be resized down.
        // FIXME-TABLE: Providing a stretch-on-init would make sense especially for tables which don't have saved settings
        HelpMarker(imgui_,
            "Using _Resizable + _SizingFixedFit flags.\n"
            "Fixed-width columns generally makes more sense if you want to use horizontal scrolling.\n\n"
            "Double-click a column border to auto-fit the column to its contents.");
        PushStyleCompact(imgui_);
        static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_ContextMenuInBody;
        imgui_.CheckboxFlags("ImGuiTableFlags_NoHostExtendX", &flags, ImGuiTableFlags_NoHostExtendX);
        PopStyleCompact(imgui_);

        if (imgui_.BeginTable("table1", 3, flags))
        {
            for (int row = 0; row < 5; row++)
            {
                imgui_.TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    imgui_.TableSetColumnIndex(column);
                    imgui_.Text("Hello %d,%d", column, row);
                }
            }
            imgui_.EndTable();
        }
        imgui_.TreePop();
    }

    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Resizable, mixed"))
    {
        HelpMarker(imgui_,
            "Using TableSetupColumn() to alter resizing policy on a per-column basis.\n\n"
            "When combining Fixed and Stretch columns, generally you only want one, maybe two trailing columns to use _WidthStretch.");
        static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;

        if (imgui_.BeginTable("table1", 3, flags))
        {
            imgui_.TableSetupColumn("AAA", ImGuiTableColumnFlags_WidthFixed);
            imgui_.TableSetupColumn("BBB", ImGuiTableColumnFlags_WidthFixed);
            imgui_.TableSetupColumn("CCC", ImGuiTableColumnFlags_WidthStretch);
            imgui_.TableHeadersRow();
            for (int row = 0; row < 5; row++)
            {
                imgui_.TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    imgui_.TableSetColumnIndex(column);
                    imgui_.Text("%s %d,%d", (column == 2) ? "Stretch" : "Fixed", column, row);
                }
            }
            imgui_.EndTable();
        }
        if (imgui_.BeginTable("table2", 6, flags))
        {
            imgui_.TableSetupColumn("AAA", ImGuiTableColumnFlags_WidthFixed);
            imgui_.TableSetupColumn("BBB", ImGuiTableColumnFlags_WidthFixed);
            imgui_.TableSetupColumn("CCC", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultHide);
            imgui_.TableSetupColumn("DDD", ImGuiTableColumnFlags_WidthStretch);
            imgui_.TableSetupColumn("EEE", ImGuiTableColumnFlags_WidthStretch);
            imgui_.TableSetupColumn("FFF", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultHide);
            imgui_.TableHeadersRow();
            for (int row = 0; row < 5; row++)
            {
                imgui_.TableNextRow();
                for (int column = 0; column < 6; column++)
                {
                    imgui_.TableSetColumnIndex(column);
                    imgui_.Text("%s %d,%d", (column >= 3) ? "Stretch" : "Fixed", column, row);
                }
            }
            imgui_.EndTable();
        }
        imgui_.TreePop();
    }

    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Reorderable, hideable, with headers"))
    {
        HelpMarker(imgui_,
            "Click and drag column headers to reorder columns.\n\n"
            "Right-click on a header to open a context menu.");
        static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;
        PushStyleCompact(imgui_);
        imgui_.CheckboxFlags("ImGuiTableFlags_Resizable", &flags, ImGuiTableFlags_Resizable);
        imgui_.CheckboxFlags("ImGuiTableFlags_Reorderable", &flags, ImGuiTableFlags_Reorderable);
        imgui_.CheckboxFlags("ImGuiTableFlags_Hideable", &flags, ImGuiTableFlags_Hideable);
        imgui_.CheckboxFlags("ImGuiTableFlags_NoBordersInBody", &flags, ImGuiTableFlags_NoBordersInBody);
        imgui_.CheckboxFlags("ImGuiTableFlags_NoBordersInBodyUntilResize", &flags, ImGuiTableFlags_NoBordersInBodyUntilResize); imgui_.SameLine(); HelpMarker(imgui_, "Disable vertical borders in columns Body until hovered for resize (borders will always appears in Headers)");
        PopStyleCompact(imgui_);

        if (imgui_.BeginTable("table1", 3, flags))
        {
            // Submit columns name with TableSetupColumn() and call TableHeadersRow() to create a row with a header in each column.
            // (Later we will show how TableSetupColumn() has other uses, optional flags, sizing weight etc.)
            imgui_.TableSetupColumn("One");
            imgui_.TableSetupColumn("Two");
            imgui_.TableSetupColumn("Three");
            imgui_.TableHeadersRow();
            for (int row = 0; row < 6; row++)
            {
                imgui_.TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    imgui_.TableSetColumnIndex(column);
                    imgui_.Text("Hello %d,%d", column, row);
                }
            }
            imgui_.EndTable();
        }

        // Use outer_size.x == 0.0f instead of default to make the table as tight as possible (only valid when no scrolling and no stretch column)
        if (imgui_.BeginTable("table2", 3, flags | ImGuiTableFlags_SizingFixedFit, ImVec2(0.0f, 0.0f)))
        {
            imgui_.TableSetupColumn("One");
            imgui_.TableSetupColumn("Two");
            imgui_.TableSetupColumn("Three");
            imgui_.TableHeadersRow();
            for (int row = 0; row < 6; row++)
            {
                imgui_.TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    imgui_.TableSetColumnIndex(column);
                    imgui_.Text("Fixed %d,%d", column, row);
                }
            }
            imgui_.EndTable();
        }
        imgui_.TreePop();
    }

    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Padding"))
    {
        // First example: showcase use of padding flags and effect of BorderOuterV/BorderInnerV on X padding.
        // We don't expose BorderOuterH/BorderInnerH here because they have no effect on X padding.
        HelpMarker(imgui_,
            "We often want outer padding activated when any using features which makes the edges of a column visible:\n"
            "e.g.:\n"
            "- BorderOuterV\n"
            "- any form of row selection\n"
            "Because of this, activating BorderOuterV sets the default to PadOuterX. Using PadOuterX or NoPadOuterX you can override the default.\n\n"
            "Actual padding values are using style.CellPadding.\n\n"
            "In this demo we don't show horizontal borders to emphasis how they don't affect default horizontal padding.");

        static ImGuiTableFlags flags1 = ImGuiTableFlags_BordersV;
        PushStyleCompact(imgui_);
        imgui_.CheckboxFlags("ImGuiTableFlags_PadOuterX", &flags1, ImGuiTableFlags_PadOuterX);
        imgui_.SameLine(); HelpMarker(imgui_, "Enable outer-most padding (default if ImGuiTableFlags_BordersOuterV is set)");
        imgui_.CheckboxFlags("ImGuiTableFlags_NoPadOuterX", &flags1, ImGuiTableFlags_NoPadOuterX);
        imgui_.SameLine(); HelpMarker(imgui_, "Disable outer-most padding (default if ImGuiTableFlags_BordersOuterV is not set)");
        imgui_.CheckboxFlags("ImGuiTableFlags_NoPadInnerX", &flags1, ImGuiTableFlags_NoPadInnerX);
        imgui_.SameLine(); HelpMarker(imgui_, "Disable inner padding between columns (double inner padding if BordersOuterV is on, single inner padding if BordersOuterV is off)");
        imgui_.CheckboxFlags("ImGuiTableFlags_BordersOuterV", &flags1, ImGuiTableFlags_BordersOuterV);
        imgui_.CheckboxFlags("ImGuiTableFlags_BordersInnerV", &flags1, ImGuiTableFlags_BordersInnerV);
        static bool show_headers = false;
        imgui_.Checkbox("show_headers", &show_headers);
        PopStyleCompact(imgui_);

        if (imgui_.BeginTable("table_padding", 3, flags1))
        {
            if (show_headers)
            {
                imgui_.TableSetupColumn("One");
                imgui_.TableSetupColumn("Two");
                imgui_.TableSetupColumn("Three");
                imgui_.TableHeadersRow();
            }

            for (int row = 0; row < 5; row++)
            {
                imgui_.TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    imgui_.TableSetColumnIndex(column);
                    if (row == 0)
                    {
                        imgui_.Text("Avail %.2f", imgui_.GetContentRegionAvail().x);
                    }
                    else
                    {
                        char buf[32];
                        sprintf(buf, "Hello %d,%d", column, row);
                        imgui_.Button(buf, ImVec2(-FLT_MIN, 0.0f));
                    }
                    //if (imgui_.TableGetColumnFlags() & ImGuiTableColumnFlags_IsHovered)
                    //    imgui_.TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(0, 100, 0, 255));
                }
            }
            imgui_.EndTable();
        }

        // Second example: set style.CellPadding to (0.0) or a custom value.
        // FIXME-TABLE: Vertical border effectively not displayed the same way as horizontal one...
        HelpMarker(imgui_, "Setting style.CellPadding to (0,0) or a custom value.");
        static ImGuiTableFlags flags2 = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
        static ImVec2 cell_padding(0.0f, 0.0f);
        static bool show_widget_frame_bg = true;

        PushStyleCompact(imgui_);
        imgui_.CheckboxFlags("ImGuiTableFlags_Borders", &flags2, ImGuiTableFlags_Borders);
        imgui_.CheckboxFlags("ImGuiTableFlags_BordersH", &flags2, ImGuiTableFlags_BordersH);
        imgui_.CheckboxFlags("ImGuiTableFlags_BordersV", &flags2, ImGuiTableFlags_BordersV);
        imgui_.CheckboxFlags("ImGuiTableFlags_BordersInner", &flags2, ImGuiTableFlags_BordersInner);
        imgui_.CheckboxFlags("ImGuiTableFlags_BordersOuter", &flags2, ImGuiTableFlags_BordersOuter);
        imgui_.CheckboxFlags("ImGuiTableFlags_RowBg", &flags2, ImGuiTableFlags_RowBg);
        imgui_.CheckboxFlags("ImGuiTableFlags_Resizable", &flags2, ImGuiTableFlags_Resizable);
        imgui_.Checkbox("show_widget_frame_bg", &show_widget_frame_bg);
        imgui_.SliderFloat2("CellPadding", &cell_padding.x, 0.0f, 10.0f, "%.0f");
        PopStyleCompact(imgui_);

        imgui_.PushStyleVar(ImGuiStyleVar_CellPadding, cell_padding);
        if (imgui_.BeginTable("table_padding_2", 3, flags2))
        {
            static char text_bufs[3 * 5][16]; // Mini text storage for 3x5 cells
            static bool init = true;
            if (!show_widget_frame_bg)
                imgui_.PushStyleColor(ImGuiCol_FrameBg, 0);
            for (int cell = 0; cell < 3 * 5; cell++)
            {
                imgui_.TableNextColumn();
                if (init)
                    strcpy(text_bufs[cell], "edit me");
                imgui_.SetNextItemWidth(-FLT_MIN);
                imgui_.PushID(cell);
                imgui_.InputText("##cell", text_bufs[cell], IM_ARRAYSIZE(text_bufs[cell]));
                imgui_.PopID();
            }
            if (!show_widget_frame_bg)
                imgui_.PopStyleColor();
            init = false;
            imgui_.EndTable();
        }
        imgui_.PopStyleVar();

        imgui_.TreePop();
    }

    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Sizing policies"))
    {
        static ImGuiTableFlags flags1 = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_ContextMenuInBody;
        PushStyleCompact(imgui_);
        imgui_.CheckboxFlags("ImGuiTableFlags_Resizable", &flags1, ImGuiTableFlags_Resizable);
        imgui_.CheckboxFlags("ImGuiTableFlags_NoHostExtendX", &flags1, ImGuiTableFlags_NoHostExtendX);
        PopStyleCompact(imgui_);

        static ImGuiTableFlags sizing_policy_flags[4] = { ImGuiTableFlags_SizingFixedFit, ImGuiTableFlags_SizingFixedSame, ImGuiTableFlags_SizingStretchProp, ImGuiTableFlags_SizingStretchSame };
        for (int table_n = 0; table_n < 4; table_n++)
        {
            imgui_.PushID(table_n);
            imgui_.SetNextItemWidth(TEXT_BASE_WIDTH * 30);
            EditTableSizingFlags(imgui_, &sizing_policy_flags[table_n]);

            // To make it easier to understand the different sizing policy,
            // For each policy: we display one table where the columns have equal contents width, and one where the columns have different contents width.
            if (imgui_.BeginTable("table1", 3, sizing_policy_flags[table_n] | flags1))
            {
                for (int row = 0; row < 3; row++)
                {
                    imgui_.TableNextRow();
                    imgui_.TableNextColumn(); imgui_.Text("Oh dear");
                    imgui_.TableNextColumn(); imgui_.Text("Oh dear");
                    imgui_.TableNextColumn(); imgui_.Text("Oh dear");
                }
                imgui_.EndTable();
            }
            if (imgui_.BeginTable("table2", 3, sizing_policy_flags[table_n] | flags1))
            {
                for (int row = 0; row < 3; row++)
                {
                    imgui_.TableNextRow();
                    imgui_.TableNextColumn(); imgui_.Text("AAAA");
                    imgui_.TableNextColumn(); imgui_.Text("BBBBBBBB");
                    imgui_.TableNextColumn(); imgui_.Text("CCCCCCCCCCCC");
                }
                imgui_.EndTable();
            }
            imgui_.PopID();
        }

        imgui_.Spacing();
        imgui_.TextUnformatted("Advanced");
        imgui_.SameLine();
        HelpMarker(imgui_, "This section allows you to interact and see the effect of various sizing policies depending on whether Scroll is enabled and the contents of your columns.");

        enum ContentsType { CT_ShowWidth, CT_ShortText, CT_LongText, CT_Button, CT_FillButton, CT_InputText };
        static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
        static int contents_type = CT_ShowWidth;
        static int column_count = 3;

        PushStyleCompact(imgui_);
        imgui_.PushID("Advanced");
        imgui_.PushItemWidth(TEXT_BASE_WIDTH * 30);
        EditTableSizingFlags(imgui_, &flags);
        imgui_.Combo("Contents", &contents_type, "Show width\0Short Text\0Long Text\0Button\0Fill Button\0InputText\0");
        if (contents_type == CT_FillButton)
        {
            imgui_.SameLine();
            HelpMarker(imgui_, "Be mindful that using right-alignment (e.g. size.x = -FLT_MIN) creates a feedback loop where contents width can feed into auto-column width can feed into contents width.");
        }
        imgui_.DragInt("Columns", &column_count, 0.1f, 1, 64, "%d", ImGuiSliderFlags_AlwaysClamp);
        imgui_.CheckboxFlags("ImGuiTableFlags_Resizable", &flags, ImGuiTableFlags_Resizable);
        imgui_.CheckboxFlags("ImGuiTableFlags_PreciseWidths", &flags, ImGuiTableFlags_PreciseWidths);
        imgui_.SameLine(); HelpMarker(imgui_, "Disable distributing remainder width to stretched columns (width allocation on a 100-wide table with 3 columns: Without this flag: 33,33,34. With this flag: 33,33,33). With larger number of columns, resizing will appear to be less smooth.");
        imgui_.CheckboxFlags("ImGuiTableFlags_ScrollX", &flags, ImGuiTableFlags_ScrollX);
        imgui_.CheckboxFlags("ImGuiTableFlags_ScrollY", &flags, ImGuiTableFlags_ScrollY);
        imgui_.CheckboxFlags("ImGuiTableFlags_NoClip", &flags, ImGuiTableFlags_NoClip);
        imgui_.PopItemWidth();
        imgui_.PopID();
        PopStyleCompact(imgui_);

        if (imgui_.BeginTable("table2", column_count, flags, ImVec2(0.0f, TEXT_BASE_HEIGHT * 7)))
        {
            for (int cell = 0; cell < 10 * column_count; cell++)
            {
                imgui_.TableNextColumn();
                int column = imgui_.TableGetColumnIndex();
                int row = imgui_.TableGetRowIndex();

                imgui_.PushID(cell);
                char label[32];
                static char text_buf[32] = "";
                sprintf(label, "Hello %d,%d", column, row);
                switch (contents_type)
                {
                case CT_ShortText:  imgui_.TextUnformatted(label); break;
                case CT_LongText:   imgui_.Text("Some %s text %d,%d\nOver two lines..", column == 0 ? "long" : "longeeer", column, row); break;
                case CT_ShowWidth:  imgui_.Text("W: %.1f", imgui_.GetContentRegionAvail().x); break;
                case CT_Button:     imgui_.Button(label); break;
                case CT_FillButton: imgui_.Button(label, ImVec2(-FLT_MIN, 0.0f)); break;
                case CT_InputText:  imgui_.SetNextItemWidth(-FLT_MIN); imgui_.InputText("##", text_buf, IM_ARRAYSIZE(text_buf)); break;
                }
                imgui_.PopID();
            }
            imgui_.EndTable();
        }
        imgui_.TreePop();
    }

    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Vertical scrolling, with clipping"))
    {
        HelpMarker(imgui_, "Here we activate ScrollY, which will create a child window container to allow hosting scrollable contents.\n\nWe also demonstrate using ImGuiListClipper to virtualize the submission of many items.");
        static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;

        PushStyleCompact(imgui_);
        imgui_.CheckboxFlags("ImGuiTableFlags_ScrollY", &flags, ImGuiTableFlags_ScrollY);
        PopStyleCompact(imgui_);

        // When using ScrollX or ScrollY we need to specify a size for our table container!
        // Otherwise by default the table will fit all available space, like a BeginChild() call.
        ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 8);
        if (imgui_.BeginTable("table_scrolly", 3, flags, outer_size))
        {
            imgui_.TableSetupScrollFreeze(0, 1); // Make top row always visible
            imgui_.TableSetupColumn("One", ImGuiTableColumnFlags_None);
            imgui_.TableSetupColumn("Two", ImGuiTableColumnFlags_None);
            imgui_.TableSetupColumn("Three", ImGuiTableColumnFlags_None);
            imgui_.TableHeadersRow();

            // Demonstrate using clipper for large vertical lists
            ImGuiListClipper clipper(imgui_);
            clipper.Begin(1000);
            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                {
                    imgui_.TableNextRow();
                    for (int column = 0; column < 3; column++)
                    {
                        imgui_.TableSetColumnIndex(column);
                        imgui_.Text("Hello %d,%d", column, row);
                    }
                }
            }
            imgui_.EndTable();
        }
        imgui_.TreePop();
    }

    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Horizontal scrolling"))
    {
        HelpMarker(imgui_,
            "When ScrollX is enabled, the default sizing policy becomes ImGuiTableFlags_SizingFixedFit, "
            "as automatically stretching columns doesn't make much sense with horizontal scrolling.\n\n"
            "Also note that as of the current version, you will almost always want to enable ScrollY along with ScrollX,"
            "because the container window won't automatically extend vertically to fix contents (this may be improved in future versions).");
        static ImGuiTableFlags flags = ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        static int freeze_cols = 1;
        static int freeze_rows = 1;

        PushStyleCompact(imgui_);
        imgui_.CheckboxFlags("ImGuiTableFlags_Resizable", &flags, ImGuiTableFlags_Resizable);
        imgui_.CheckboxFlags("ImGuiTableFlags_ScrollX", &flags, ImGuiTableFlags_ScrollX);
        imgui_.CheckboxFlags("ImGuiTableFlags_ScrollY", &flags, ImGuiTableFlags_ScrollY);
        imgui_.SetNextItemWidth(imgui_.GetFrameHeight());
        imgui_.DragInt("freeze_cols", &freeze_cols, 0.2f, 0, 9, NULL, ImGuiSliderFlags_NoInput);
        imgui_.SetNextItemWidth(imgui_.GetFrameHeight());
        imgui_.DragInt("freeze_rows", &freeze_rows, 0.2f, 0, 9, NULL, ImGuiSliderFlags_NoInput);
        PopStyleCompact(imgui_);

        // When using ScrollX or ScrollY we need to specify a size for our table container!
        // Otherwise by default the table will fit all available space, like a BeginChild() call.
        ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 8);
        if (imgui_.BeginTable("table_scrollx", 7, flags, outer_size))
        {
            imgui_.TableSetupScrollFreeze(freeze_cols, freeze_rows);
            imgui_.TableSetupColumn("Line #", ImGuiTableColumnFlags_NoHide); // Make the first column not hideable to match our use of TableSetupScrollFreeze()
            imgui_.TableSetupColumn("One");
            imgui_.TableSetupColumn("Two");
            imgui_.TableSetupColumn("Three");
            imgui_.TableSetupColumn("Four");
            imgui_.TableSetupColumn("Five");
            imgui_.TableSetupColumn("Six");
            imgui_.TableHeadersRow();
            for (int row = 0; row < 20; row++)
            {
                imgui_.TableNextRow();
                for (int column = 0; column < 7; column++)
                {
                    // Both TableNextColumn() and TableSetColumnIndex() return true when a column is visible or performing width measurement.
                    // Because here we know that:
                    // - A) all our columns are contributing the same to row height
                    // - B) column 0 is always visible,
                    // We only always submit this one column and can skip others.
                    // More advanced per-column clipping behaviors may benefit from polling the status flags via TableGetColumnFlags().
                    if (!imgui_.TableSetColumnIndex(column) && column > 0)
                        continue;
                    if (column == 0)
                        imgui_.Text("Line %d", row);
                    else
                        imgui_.Text("Hello world %d,%d", column, row);
                }
            }
            imgui_.EndTable();
        }

        imgui_.Spacing();
        imgui_.TextUnformatted("Stretch + ScrollX");
        imgui_.SameLine();
        HelpMarker(imgui_,
            "Showcase using Stretch columns + ScrollX together: "
            "this is rather unusual and only makes sense when specifying an 'inner_width' for the table!\n"
            "Without an explicit value, inner_width is == outer_size.x and therefore using Stretch columns + ScrollX together doesn't make sense.");
        static ImGuiTableFlags flags2 = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg | ImGuiTableFlags_ContextMenuInBody;
        static float inner_width = 1000.0f;
        PushStyleCompact(imgui_);
        imgui_.PushID("flags3");
        imgui_.PushItemWidth(TEXT_BASE_WIDTH * 30);
        imgui_.CheckboxFlags("ImGuiTableFlags_ScrollX", &flags2, ImGuiTableFlags_ScrollX);
        imgui_.DragFloat("inner_width", &inner_width, 1.0f, 0.0f, FLT_MAX, "%.1f");
        imgui_.PopItemWidth();
        imgui_.PopID();
        PopStyleCompact(imgui_);
        if (imgui_.BeginTable("table2", 7, flags2, outer_size, inner_width))
        {
            for (int cell = 0; cell < 20 * 7; cell++)
            {
                imgui_.TableNextColumn();
                imgui_.Text("Hello world %d,%d", imgui_.TableGetColumnIndex(), imgui_.TableGetRowIndex());
            }
            imgui_.EndTable();
        }
        imgui_.TreePop();
    }

    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Columns flags"))
    {
        // Create a first table just to show all the options/flags we want to make visible in our example!
        const int column_count = 3;
        const char* column_names[column_count] = { "One", "Two", "Three" };
        static ImGuiTableColumnFlags column_flags[column_count] = { ImGuiTableColumnFlags_DefaultSort, ImGuiTableColumnFlags_None, ImGuiTableColumnFlags_DefaultHide };
        static ImGuiTableColumnFlags column_flags_out[column_count] = { 0, 0, 0 }; // Output from TableGetColumnFlags()

        if (imgui_.BeginTable("table_columns_flags_checkboxes", column_count, ImGuiTableFlags_None))
        {
            PushStyleCompact(imgui_);
            for (int column = 0; column < column_count; column++)
            {
                imgui_.TableNextColumn();
                imgui_.PushID(column);
                imgui_.AlignTextToFramePadding(); // FIXME-TABLE: Workaround for wrong text baseline propagation
                imgui_.Text("'%s'", column_names[column]);
                imgui_.Spacing();
                imgui_.Text("Input flags:");
                EditTableColumnsFlags(imgui_, &column_flags[column]);
                imgui_.Spacing();
                imgui_.Text("Output flags:");
                ShowTableColumnsStatusFlags(imgui_, column_flags_out[column]);
                imgui_.PopID();
            }
            PopStyleCompact(imgui_);
            imgui_.EndTable();
        }

        // Create the real table we care about for the example!
        // We use a scrolling table to be able to showcase the difference between the _IsEnabled and _IsVisible flags above, otherwise in
        // a non-scrolling table columns are always visible (unless using ImGuiTableFlags_NoKeepColumnsVisible + resizing the parent window down)
        const ImGuiTableFlags flags
            = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY
            | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV
            | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable;
        ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 9);
        if (imgui_.BeginTable("table_columns_flags", column_count, flags, outer_size))
        {
            for (int column = 0; column < column_count; column++)
                imgui_.TableSetupColumn(column_names[column], column_flags[column]);
            imgui_.TableHeadersRow();
            for (int column = 0; column < column_count; column++)
                column_flags_out[column] = imgui_.TableGetColumnFlags(column);
            float indent_step = (float)((int)TEXT_BASE_WIDTH / 2);
            for (int row = 0; row < 8; row++)
            {
                imgui_.Indent(indent_step); // Add some indentation to demonstrate usage of per-column IndentEnable/IndentDisable flags.
                imgui_.TableNextRow();
                for (int column = 0; column < column_count; column++)
                {
                    imgui_.TableSetColumnIndex(column);
                    imgui_.Text("%s %s", (column == 0) ? "Indented" : "Hello", imgui_.TableGetColumnName(column));
                }
            }
            imgui_.Unindent(indent_step * 8.0f);

            imgui_.EndTable();
        }
        imgui_.TreePop();
    }

    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Columns widths"))
    {
        HelpMarker(imgui_, "Using TableSetupColumn() to setup default width.");

        static ImGuiTableFlags flags1 = ImGuiTableFlags_Borders | ImGuiTableFlags_NoBordersInBodyUntilResize;
        PushStyleCompact(imgui_);
        imgui_.CheckboxFlags("ImGuiTableFlags_Resizable", &flags1, ImGuiTableFlags_Resizable);
        imgui_.CheckboxFlags("ImGuiTableFlags_NoBordersInBodyUntilResize", &flags1, ImGuiTableFlags_NoBordersInBodyUntilResize);
        PopStyleCompact(imgui_);
        if (imgui_.BeginTable("table1", 3, flags1))
        {
            // We could also set ImGuiTableFlags_SizingFixedFit on the table and all columns will default to ImGuiTableColumnFlags_WidthFixed.
            imgui_.TableSetupColumn("one", ImGuiTableColumnFlags_WidthFixed, 100.0f); // Default to 100.0f
            imgui_.TableSetupColumn("two", ImGuiTableColumnFlags_WidthFixed, 200.0f); // Default to 200.0f
            imgui_.TableSetupColumn("three", ImGuiTableColumnFlags_WidthFixed);       // Default to auto
            imgui_.TableHeadersRow();
            for (int row = 0; row < 4; row++)
            {
                imgui_.TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    imgui_.TableSetColumnIndex(column);
                    if (row == 0)
                        imgui_.Text("(w: %5.1f)", imgui_.GetContentRegionAvail().x);
                    else
                        imgui_.Text("Hello %d,%d", column, row);
                }
            }
            imgui_.EndTable();
        }

        HelpMarker(imgui_, "Using TableSetupColumn() to setup explicit width.\n\nUnless _NoKeepColumnsVisible is set, fixed columns with set width may still be shrunk down if there's not enough space in the host.");

        static ImGuiTableFlags flags2 = ImGuiTableFlags_None;
        PushStyleCompact(imgui_);
        imgui_.CheckboxFlags("ImGuiTableFlags_NoKeepColumnsVisible", &flags2, ImGuiTableFlags_NoKeepColumnsVisible);
        imgui_.CheckboxFlags("ImGuiTableFlags_BordersInnerV", &flags2, ImGuiTableFlags_BordersInnerV);
        imgui_.CheckboxFlags("ImGuiTableFlags_BordersOuterV", &flags2, ImGuiTableFlags_BordersOuterV);
        PopStyleCompact(imgui_);
        if (imgui_.BeginTable("table2", 4, flags2))
        {
            // We could also set ImGuiTableFlags_SizingFixedFit on the table and all columns will default to ImGuiTableColumnFlags_WidthFixed.
            imgui_.TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            imgui_.TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 15.0f);
            imgui_.TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 30.0f);
            imgui_.TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 15.0f);
            for (int row = 0; row < 5; row++)
            {
                imgui_.TableNextRow();
                for (int column = 0; column < 4; column++)
                {
                    imgui_.TableSetColumnIndex(column);
                    if (row == 0)
                        imgui_.Text("(w: %5.1f)", imgui_.GetContentRegionAvail().x);
                    else
                        imgui_.Text("Hello %d,%d", column, row);
                }
            }
            imgui_.EndTable();
        }
        imgui_.TreePop();
    }

    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Nested tables"))
    {
        HelpMarker(imgui_, "This demonstrate embedding a table into another table cell.");

        if (imgui_.BeginTable("table_nested1", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable))
        {
            imgui_.TableSetupColumn("A0");
            imgui_.TableSetupColumn("A1");
            imgui_.TableHeadersRow();

            imgui_.TableNextColumn();
            imgui_.Text("A0 Row 0");
            {
                float rows_height = TEXT_BASE_HEIGHT * 2;
                if (imgui_.BeginTable("table_nested2", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable))
                {
                    imgui_.TableSetupColumn("B0");
                    imgui_.TableSetupColumn("B1");
                    imgui_.TableHeadersRow();

                    imgui_.TableNextRow(ImGuiTableRowFlags_None, rows_height);
                    imgui_.TableNextColumn();
                    imgui_.Text("B0 Row 0");
                    imgui_.TableNextColumn();
                    imgui_.Text("B1 Row 0");
                    imgui_.TableNextRow(ImGuiTableRowFlags_None, rows_height);
                    imgui_.TableNextColumn();
                    imgui_.Text("B0 Row 1");
                    imgui_.TableNextColumn();
                    imgui_.Text("B1 Row 1");

                    imgui_.EndTable();
                }
            }
            imgui_.TableNextColumn(); imgui_.Text("A1 Row 0");
            imgui_.TableNextColumn(); imgui_.Text("A0 Row 1");
            imgui_.TableNextColumn(); imgui_.Text("A1 Row 1");
            imgui_.EndTable();
        }
        imgui_.TreePop();
    }

    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Row height"))
    {
        HelpMarker(imgui_, "You can pass a 'min_row_height' to TableNextRow().\n\nRows are padded with 'style.CellPadding.y' on top and bottom, so effectively the minimum row height will always be >= 'style.CellPadding.y * 2.0f'.\n\nWe cannot honor a _maximum_ row height as that would requires a unique clipping rectangle per row.");
        if (imgui_.BeginTable("table_row_height", 1, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV))
        {
            for (int row = 0; row < 10; row++)
            {
                float min_row_height = (float)(int)(TEXT_BASE_HEIGHT * 0.30f * row);
                imgui_.TableNextRow(ImGuiTableRowFlags_None, min_row_height);
                imgui_.TableNextColumn();
                imgui_.Text("min_row_height = %.2f", min_row_height);
            }
            imgui_.EndTable();
        }
        imgui_.TreePop();
    }

    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Outer size"))
    {
        // Showcasing use of ImGuiTableFlags_NoHostExtendX and ImGuiTableFlags_NoHostExtendY
        // Important to that note how the two flags have slightly different behaviors!
        imgui_.Text("Using NoHostExtendX and NoHostExtendY:");
        PushStyleCompact(imgui_);
        static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX;
        imgui_.CheckboxFlags("ImGuiTableFlags_NoHostExtendX", &flags, ImGuiTableFlags_NoHostExtendX);
        imgui_.SameLine(); HelpMarker(imgui_, "Make outer width auto-fit to columns, overriding outer_size.x value.\n\nOnly available when ScrollX/ScrollY are disabled and Stretch columns are not used.");
        imgui_.CheckboxFlags("ImGuiTableFlags_NoHostExtendY", &flags, ImGuiTableFlags_NoHostExtendY);
        imgui_.SameLine(); HelpMarker(imgui_, "Make outer height stop exactly at outer_size.y (prevent auto-extending table past the limit).\n\nOnly available when ScrollX/ScrollY are disabled. Data below the limit will be clipped and not visible.");
        PopStyleCompact(imgui_);

        ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 5.5f);
        if (imgui_.BeginTable("table1", 3, flags, outer_size))
        {
            for (int row = 0; row < 10; row++)
            {
                imgui_.TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    imgui_.TableNextColumn();
                    imgui_.Text("Cell %d,%d", column, row);
                }
            }
            imgui_.EndTable();
        }
        imgui_.SameLine();
        imgui_.Text("Hello!");

        imgui_.Spacing();

        imgui_.Text("Using explicit size:");
        if (imgui_.BeginTable("table2", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, ImVec2(TEXT_BASE_WIDTH * 30, 0.0f)))
        {
            for (int row = 0; row < 5; row++)
            {
                imgui_.TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    imgui_.TableNextColumn();
                    imgui_.Text("Cell %d,%d", column, row);
                }
            }
            imgui_.EndTable();
        }
        imgui_.SameLine();
        if (imgui_.BeginTable("table3", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, ImVec2(TEXT_BASE_WIDTH * 30, 0.0f)))
        {
            for (int row = 0; row < 3; row++)
            {
                imgui_.TableNextRow(0, TEXT_BASE_HEIGHT * 1.5f);
                for (int column = 0; column < 3; column++)
                {
                    imgui_.TableNextColumn();
                    imgui_.Text("Cell %d,%d", column, row);
                }
            }
            imgui_.EndTable();
        }

        imgui_.TreePop();
    }

    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Background color"))
    {
        static ImGuiTableFlags flags = ImGuiTableFlags_RowBg;
        static int row_bg_type = 1;
        static int row_bg_target = 1;
        static int cell_bg_type = 1;

        PushStyleCompact(imgui_);
        imgui_.CheckboxFlags("ImGuiTableFlags_Borders", &flags, ImGuiTableFlags_Borders);
        imgui_.CheckboxFlags("ImGuiTableFlags_RowBg", &flags, ImGuiTableFlags_RowBg);
        imgui_.SameLine(); HelpMarker(imgui_, "ImGuiTableFlags_RowBg automatically sets RowBg0 to alternative colors pulled from the Style.");
        imgui_.Combo("row bg type", (int*)&row_bg_type, "None\0Red\0Gradient\0");
        imgui_.Combo("row bg target", (int*)&row_bg_target, "RowBg0\0RowBg1\0"); imgui_.SameLine(); HelpMarker(imgui_, "Target RowBg0 to override the alternating odd/even colors,\nTarget RowBg1 to blend with them.");
        imgui_.Combo("cell bg type", (int*)&cell_bg_type, "None\0Blue\0"); imgui_.SameLine(); HelpMarker(imgui_, "We are colorizing cells to B1->C2 here.");
        IM_ASSERT(row_bg_type >= 0 && row_bg_type <= 2);
        IM_ASSERT(row_bg_target >= 0 && row_bg_target <= 1);
        IM_ASSERT(cell_bg_type >= 0 && cell_bg_type <= 1);
        PopStyleCompact(imgui_);

        if (imgui_.BeginTable("table1", 5, flags))
        {
            for (int row = 0; row < 6; row++)
            {
                imgui_.TableNextRow();

                // Demonstrate setting a row background color with 'imgui_.TableSetBgColor(ImGuiTableBgTarget_RowBgX, ...)'
                // We use a transparent color so we can see the one behind in case our target is RowBg1 and RowBg0 was already targeted by the ImGuiTableFlags_RowBg flag.
                if (row_bg_type != 0)
                {
                    ImU32 row_bg_color = imgui_.GetColorU32(row_bg_type == 1 ? ImVec4(0.7f, 0.3f, 0.3f, 0.65f) : ImVec4(0.2f + row * 0.1f, 0.2f, 0.2f, 0.65f)); // Flat or Gradient?
                    imgui_.TableSetBgColor(ImGuiTableBgTarget_RowBg0 + row_bg_target, row_bg_color);
                }

                // Fill cells
                for (int column = 0; column < 5; column++)
                {
                    imgui_.TableSetColumnIndex(column);
                    imgui_.Text("%c%c", 'A' + row, '0' + column);

                    // Change background of Cells B1->C2
                    // Demonstrate setting a cell background color with 'imgui_.TableSetBgColor(ImGuiTableBgTarget_CellBg, ...)'
                    // (the CellBg color will be blended over the RowBg and ColumnBg colors)
                    // We can also pass a column number as a third parameter to TableSetBgColor() and do this outside the column loop.
                    if (row >= 1 && row <= 2 && column >= 1 && column <= 2 && cell_bg_type == 1)
                    {
                        ImU32 cell_bg_color = imgui_.GetColorU32(ImVec4(0.3f, 0.3f, 0.7f, 0.65f));
                        imgui_.TableSetBgColor(ImGuiTableBgTarget_CellBg, cell_bg_color);
                    }
                }
            }
            imgui_.EndTable();
        }
        imgui_.TreePop();
    }

    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Tree view"))
    {
        static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;

        if (imgui_.BeginTable("3ways", 3, flags))
        {
            // The first column will use the default _WidthStretch when ScrollX is Off and _WidthFixed when ScrollX is On
            imgui_.TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            imgui_.TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
            imgui_.TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 18.0f);
            imgui_.TableHeadersRow();

            // Simple storage to output a dummy file-system.
            struct MyTreeNode
            {
                ImGui& imgui;

                const char*     Name;
                const char*     Type;
                int             Size;
                int             ChildIdx;
                int             ChildCount;

                void DisplayNode(const MyTreeNode* node, const MyTreeNode* all_nodes)
                {
                    imgui.TableNextRow();
                    imgui.TableNextColumn();
                    const bool is_folder = (node->ChildCount > 0);
                    if (is_folder)
                    {
                        bool open = imgui.TreeNodeEx(node->Name, ImGuiTreeNodeFlags_SpanFullWidth);
                        imgui.TableNextColumn();
                        imgui.TextDisabled("--");
                        imgui.TableNextColumn();
                        imgui.TextUnformatted(node->Type);
                        if (open)
                        {
                            for (int child_n = 0; child_n < node->ChildCount; child_n++)
                                DisplayNode(&all_nodes[node->ChildIdx + child_n], all_nodes);
                            imgui.TreePop();
                        }
                    }
                    else
                    {
                        imgui.TreeNodeEx(node->Name, ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth);
                        imgui.TableNextColumn();
                        imgui.Text("%d", node->Size);
                        imgui.TableNextColumn();
                        imgui.TextUnformatted(node->Type);
                    }
                }
            };

            static const MyTreeNode nodes[] =
            {
                { imgui_, "Root",                         "Folder",       -1,       1, 3    }, // 0
                { imgui_, "Music",                        "Folder",       -1,       4, 2    }, // 1
                { imgui_, "Textures",                     "Folder",       -1,       6, 3    }, // 2
                { imgui_, "desktop.ini",                  "System file",  1024,    -1,-1    }, // 3
                { imgui_, "File1_a.wav",                  "Audio file",   123000,  -1,-1    }, // 4
                { imgui_, "File1_b.wav",                  "Audio file",   456000,  -1,-1    }, // 5
                { imgui_, "Image001.png",                 "Image file",   203128,  -1,-1    }, // 6
                { imgui_, "Copy of Image001.png",         "Image file",   203256,  -1,-1    }, // 7
                { imgui_, "Copy of Image001 (Final2).png","Image file",   203512,  -1,-1    }, // 8
            };

            MyTreeNode{ imgui_ }.DisplayNode(&nodes[0], nodes);

            imgui_.EndTable();
        }
        imgui_.TreePop();
    }

    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Item width"))
    {
        HelpMarker(imgui_,
            "Showcase using PushItemWidth() and how it is preserved on a per-column basis.\n\n"
            "Note that on auto-resizing non-resizable fixed columns, querying the content width for e.g. right-alignment doesn't make sense.");
        if (imgui_.BeginTable("table_item_width", 3, ImGuiTableFlags_Borders))
        {
            imgui_.TableSetupColumn("small");
            imgui_.TableSetupColumn("half");
            imgui_.TableSetupColumn("right-align");
            imgui_.TableHeadersRow();

            for (int row = 0; row < 3; row++)
            {
                imgui_.TableNextRow();
                if (row == 0)
                {
                    // Setup ItemWidth once (instead of setting up every time, which is also possible but less efficient)
                    imgui_.TableSetColumnIndex(0);
                    imgui_.PushItemWidth(TEXT_BASE_WIDTH * 3.0f); // Small
                    imgui_.TableSetColumnIndex(1);
                    imgui_.PushItemWidth(-imgui_.GetContentRegionAvail().x * 0.5f);
                    imgui_.TableSetColumnIndex(2);
                    imgui_.PushItemWidth(-FLT_MIN); // Right-aligned
                }

                // Draw our contents
                static float dummy_f = 0.0f;
                imgui_.PushID(row);
                imgui_.TableSetColumnIndex(0);
                imgui_.SliderFloat("float0", &dummy_f, 0.0f, 1.0f);
                imgui_.TableSetColumnIndex(1);
                imgui_.SliderFloat("float1", &dummy_f, 0.0f, 1.0f);
                imgui_.TableSetColumnIndex(2);
                imgui_.SliderFloat("float2", &dummy_f, 0.0f, 1.0f);
                imgui_.PopID();
            }
            imgui_.EndTable();
        }
        imgui_.TreePop();
    }

    // Demonstrate using TableHeader() calls instead of TableHeadersRow()
    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Custom headers"))
    {
        const int COLUMNS_COUNT = 3;
        if (imgui_.BeginTable("table_custom_headers", COLUMNS_COUNT, ImGuiTableFlags_Borders | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable))
        {
            imgui_.TableSetupColumn("Apricot");
            imgui_.TableSetupColumn("Banana");
            imgui_.TableSetupColumn("Cherry");

            // Dummy entire-column selection storage
            // FIXME: It would be nice to actually demonstrate full-featured selection using those checkbox.
            static bool column_selected[3] = {};

            // Instead of calling TableHeadersRow() we'll submit custom headers ourselves
            imgui_.TableNextRow(ImGuiTableRowFlags_Headers);
            for (int column = 0; column < COLUMNS_COUNT; column++)
            {
                imgui_.TableSetColumnIndex(column);
                const char* column_name = imgui_.TableGetColumnName(column); // Retrieve name passed to TableSetupColumn()
                imgui_.PushID(column);
                imgui_.PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                imgui_.Checkbox("##checkall", &column_selected[column]);
                imgui_.PopStyleVar();
                imgui_.SameLine(0.0f, imgui_.GetStyle().ItemInnerSpacing.x);
                imgui_.TableHeader(column_name);
                imgui_.PopID();
            }

            for (int row = 0; row < 5; row++)
            {
                imgui_.TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    char buf[32];
                    sprintf(buf, "Cell %d,%d", column, row);
                    imgui_.TableSetColumnIndex(column);
                    imgui_.Selectable(buf, column_selected[column]);
                }
            }
            imgui_.EndTable();
        }
        imgui_.TreePop();
    }

    // Demonstrate creating custom context menus inside columns, while playing it nice with context menus provided by TableHeadersRow()/TableHeader()
    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Context menus"))
    {
        HelpMarker(imgui_, "By default, right-clicking over a TableHeadersRow()/TableHeader() line will open the default context-menu.\nUsing ImGuiTableFlags_ContextMenuInBody we also allow right-clicking over columns body.");
        static ImGuiTableFlags flags1 = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders | ImGuiTableFlags_ContextMenuInBody;

        PushStyleCompact(imgui_);
        imgui_.CheckboxFlags("ImGuiTableFlags_ContextMenuInBody", &flags1, ImGuiTableFlags_ContextMenuInBody);
        PopStyleCompact(imgui_);

        // Context Menus: first example
        // [1.1] Right-click on the TableHeadersRow() line to open the default table context menu.
        // [1.2] Right-click in columns also open the default table context menu (if ImGuiTableFlags_ContextMenuInBody is set)
        const int COLUMNS_COUNT = 3;
        if (imgui_.BeginTable("table_context_menu", COLUMNS_COUNT, flags1))
        {
            imgui_.TableSetupColumn("One");
            imgui_.TableSetupColumn("Two");
            imgui_.TableSetupColumn("Three");

            // [1.1]] Right-click on the TableHeadersRow() line to open the default table context menu.
            imgui_.TableHeadersRow();

            // Submit dummy contents
            for (int row = 0; row < 4; row++)
            {
                imgui_.TableNextRow();
                for (int column = 0; column < COLUMNS_COUNT; column++)
                {
                    imgui_.TableSetColumnIndex(column);
                    imgui_.Text("Cell %d,%d", column, row);
                }
            }
            imgui_.EndTable();
        }

        // Context Menus: second example
        // [2.1] Right-click on the TableHeadersRow() line to open the default table context menu.
        // [2.2] Right-click on the ".." to open a custom popup
        // [2.3] Right-click in columns to open another custom popup
        HelpMarker(imgui_, "Demonstrate mixing table context menu (over header), item context button (over button) and custom per-colum context menu (over column body).");
        ImGuiTableFlags flags2 = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders;
        if (imgui_.BeginTable("table_context_menu_2", COLUMNS_COUNT, flags2))
        {
            imgui_.TableSetupColumn("One");
            imgui_.TableSetupColumn("Two");
            imgui_.TableSetupColumn("Three");

            // [2.1] Right-click on the TableHeadersRow() line to open the default table context menu.
            imgui_.TableHeadersRow();
            for (int row = 0; row < 4; row++)
            {
                imgui_.TableNextRow();
                for (int column = 0; column < COLUMNS_COUNT; column++)
                {
                    // Submit dummy contents
                    imgui_.TableSetColumnIndex(column);
                    imgui_.Text("Cell %d,%d", column, row);
                    imgui_.SameLine();

                    // [2.2] Right-click on the ".." to open a custom popup
                    imgui_.PushID(row * COLUMNS_COUNT + column);
                    imgui_.SmallButton("..");
                    if (imgui_.BeginPopupContextItem())
                    {
                        imgui_.Text("This is the popup for Button(\"..\") in Cell %d,%d", column, row);
                        if (imgui_.Button("Close"))
                            imgui_.CloseCurrentPopup();
                        imgui_.EndPopup();
                    }
                    imgui_.PopID();
                }
            }

            // [2.3] Right-click anywhere in columns to open another custom popup
            // (instead of testing for !IsAnyItemHovered() we could also call OpenPopup() with ImGuiPopupFlags_NoOpenOverExistingPopup
            // to manage popup priority as the popups triggers, here "are we hovering a column" are overlapping)
            int hovered_column = -1;
            for (int column = 0; column < COLUMNS_COUNT + 1; column++)
            {
                imgui_.PushID(column);
                if (imgui_.TableGetColumnFlags(column) & ImGuiTableColumnFlags_IsHovered)
                    hovered_column = column;
                if (hovered_column == column && !imgui_.IsAnyItemHovered() && imgui_.IsMouseReleased(1))
                    imgui_.OpenPopup("MyPopup");
                if (imgui_.BeginPopup("MyPopup"))
                {
                    if (column == COLUMNS_COUNT)
                        imgui_.Text("This is a custom popup for unused space after the last column.");
                    else
                        imgui_.Text("This is a custom popup for Column %d", column);
                    if (imgui_.Button("Close"))
                        imgui_.CloseCurrentPopup();
                    imgui_.EndPopup();
                }
                imgui_.PopID();
            }

            imgui_.EndTable();
            imgui_.Text("Hovered column: %d", hovered_column);
        }
        imgui_.TreePop();
    }

    // Demonstrate creating multiple tables with the same ID
    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Synced instances"))
    {
        HelpMarker(imgui_, "Multiple tables with the same identifier will share their settings, width, visibility, order etc.");
        for (int n = 0; n < 3; n++)
        {
            char buf[32];
            sprintf(buf, "Synced Table %d", n);
            bool open = imgui_.CollapsingHeader(buf, ImGuiTreeNodeFlags_DefaultOpen);
            if (open && imgui_.BeginTable("Table", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings))
            {
                imgui_.TableSetupColumn("One");
                imgui_.TableSetupColumn("Two");
                imgui_.TableSetupColumn("Three");
                imgui_.TableHeadersRow();
                for (int cell = 0; cell < 9; cell++)
                {
                    imgui_.TableNextColumn();
                    imgui_.Text("this cell %d", cell);
                }
                imgui_.EndTable();
            }
        }
        imgui_.TreePop();
    }

    // Demonstrate using Sorting facilities
    // This is a simplified version of the "Advanced" example, where we mostly focus on the code necessary to handle sorting.
    // Note that the "Advanced" example also showcase manually triggering a sort (e.g. if item quantities have been modified)
    static const char* template_items_names[] =
    {
        "Banana", "Apple", "Cherry", "Watermelon", "Grapefruit", "Strawberry", "Mango",
        "Kiwi", "Orange", "Pineapple", "Blueberry", "Plum", "Coconut", "Pear", "Apricot"
    };
    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Sorting"))
    {
        // Create item list
        static ImVector<MyItem> items(imgui_);
        if (items.Size == 0)
        {
            items.resize(50, MyItem());
            for (int n = 0; n < items.Size; n++)
            {
                const int template_n = n % IM_ARRAYSIZE(template_items_names);
                MyItem& item = items[n];
                item.ID = n;
                item.Name = template_items_names[template_n];
                item.Quantity = (n * n - n) % 20; // Assign default quantities
            }
        }

        // Options
        static ImGuiTableFlags flags =
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
            | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody
            | ImGuiTableFlags_ScrollY;
        PushStyleCompact(imgui_);
        imgui_.CheckboxFlags("ImGuiTableFlags_SortMulti", &flags, ImGuiTableFlags_SortMulti);
        imgui_.SameLine(); HelpMarker(imgui_, "When sorting is enabled: hold shift when clicking headers to sort on multiple column. TableGetSortSpecs() may return specs where (SpecsCount > 1).");
        imgui_.CheckboxFlags("ImGuiTableFlags_SortTristate", &flags, ImGuiTableFlags_SortTristate);
        imgui_.SameLine(); HelpMarker(imgui_, "When sorting is enabled: allow no sorting, disable default sorting. TableGetSortSpecs() may return specs where (SpecsCount == 0).");
        PopStyleCompact(imgui_);

        if (imgui_.BeginTable("table_sorting", 4, flags, ImVec2(0.0f, TEXT_BASE_HEIGHT * 15), 0.0f))
        {
            // Declare columns
            // We use the "user_id" parameter of TableSetupColumn() to specify a user id that will be stored in the sort specifications.
            // This is so our sort function can identify a column given our own identifier. We could also identify them based on their index!
            // Demonstrate using a mixture of flags among available sort-related flags:
            // - ImGuiTableColumnFlags_DefaultSort
            // - ImGuiTableColumnFlags_NoSort / ImGuiTableColumnFlags_NoSortAscending / ImGuiTableColumnFlags_NoSortDescending
            // - ImGuiTableColumnFlags_PreferSortAscending / ImGuiTableColumnFlags_PreferSortDescending
            imgui_.TableSetupColumn("ID",       ImGuiTableColumnFlags_DefaultSort          | ImGuiTableColumnFlags_WidthFixed,   0.0f, MyItemColumnID_ID);
            imgui_.TableSetupColumn("Name",                                                  ImGuiTableColumnFlags_WidthFixed,   0.0f, MyItemColumnID_Name);
            imgui_.TableSetupColumn("Action",   ImGuiTableColumnFlags_NoSort               | ImGuiTableColumnFlags_WidthFixed,   0.0f, MyItemColumnID_Action);
            imgui_.TableSetupColumn("Quantity", ImGuiTableColumnFlags_PreferSortDescending | ImGuiTableColumnFlags_WidthStretch, 0.0f, MyItemColumnID_Quantity);
            imgui_.TableSetupScrollFreeze(0, 1); // Make row always visible
            imgui_.TableHeadersRow();

            // Sort our data if sort specs have been changed!
            if (ImGuiTableSortSpecs* sorts_specs = imgui_.TableGetSortSpecs())
                if (sorts_specs->SpecsDirty)
                {
                    MyItem::s_current_sort_specs = sorts_specs; // Store in variable accessible by the sort function.
                    if (items.Size > 1)
                        qsort(&items[0], (size_t)items.Size, sizeof(items[0]), MyItem::CompareWithSortSpecs);
                    MyItem::s_current_sort_specs = NULL;
                    sorts_specs->SpecsDirty = false;
                }

            // Demonstrate using clipper for large vertical lists
            ImGuiListClipper clipper(imgui_);
            clipper.Begin(items.Size);
            while (clipper.Step())
                for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
                {
                    // Display a data item
                    MyItem* item = &items[row_n];
                    imgui_.PushID(item->ID);
                    imgui_.TableNextRow();
                    imgui_.TableNextColumn();
                    imgui_.Text("%04d", item->ID);
                    imgui_.TableNextColumn();
                    imgui_.TextUnformatted(item->Name);
                    imgui_.TableNextColumn();
                    imgui_.SmallButton("None");
                    imgui_.TableNextColumn();
                    imgui_.Text("%d", item->Quantity);
                    imgui_.PopID();
                }
            imgui_.EndTable();
        }
        imgui_.TreePop();
    }

    // In this example we'll expose most table flags and settings.
    // For specific flags and settings refer to the corresponding section for more detailed explanation.
    // This section is mostly useful to experiment with combining certain flags or settings with each others.
    //imgui_.SetNextItemOpen(true, ImGuiCond_Once); // [DEBUG]
    if (open_action != -1)
        imgui_.SetNextItemOpen(open_action != 0);
    if (imgui_.TreeNode("Advanced"))
    {
        static ImGuiTableFlags flags =
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable
            | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
            | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_NoBordersInBody
            | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY
            | ImGuiTableFlags_SizingFixedFit;

        enum ContentsType { CT_Text, CT_Button, CT_SmallButton, CT_FillButton, CT_Selectable, CT_SelectableSpanRow };
        static int contents_type = CT_SelectableSpanRow;
        const char* contents_type_names[] = { "Text", "Button", "SmallButton", "FillButton", "Selectable", "Selectable (span row)" };
        static int freeze_cols = 1;
        static int freeze_rows = 1;
        static int items_count = IM_ARRAYSIZE(template_items_names) * 2;
        static ImVec2 outer_size_value = ImVec2(0.0f, TEXT_BASE_HEIGHT * 12);
        static float row_min_height = 0.0f; // Auto
        static float inner_width_with_scroll = 0.0f; // Auto-extend
        static bool outer_size_enabled = true;
        static bool show_headers = true;
        static bool show_wrapped_text = false;
        //static ImGuiTextFilter filter;
        //imgui_.SetNextItemOpen(true, ImGuiCond_Once); // FIXME-TABLE: Enabling this results in initial clipped first pass on table which tend to affects column sizing
        if (imgui_.TreeNode("Options"))
        {
            // Make the UI compact because there are so many fields
            PushStyleCompact(imgui_);
            imgui_.PushItemWidth(TEXT_BASE_WIDTH * 28.0f);

            if (imgui_.TreeNodeEx("Features:", ImGuiTreeNodeFlags_DefaultOpen))
            {
                imgui_.CheckboxFlags("ImGuiTableFlags_Resizable", &flags, ImGuiTableFlags_Resizable);
                imgui_.CheckboxFlags("ImGuiTableFlags_Reorderable", &flags, ImGuiTableFlags_Reorderable);
                imgui_.CheckboxFlags("ImGuiTableFlags_Hideable", &flags, ImGuiTableFlags_Hideable);
                imgui_.CheckboxFlags("ImGuiTableFlags_Sortable", &flags, ImGuiTableFlags_Sortable);
                imgui_.CheckboxFlags("ImGuiTableFlags_NoSavedSettings", &flags, ImGuiTableFlags_NoSavedSettings);
                imgui_.CheckboxFlags("ImGuiTableFlags_ContextMenuInBody", &flags, ImGuiTableFlags_ContextMenuInBody);
                imgui_.TreePop();
            }

            if (imgui_.TreeNodeEx("Decorations:", ImGuiTreeNodeFlags_DefaultOpen))
            {
                imgui_.CheckboxFlags("ImGuiTableFlags_RowBg", &flags, ImGuiTableFlags_RowBg);
                imgui_.CheckboxFlags("ImGuiTableFlags_BordersV", &flags, ImGuiTableFlags_BordersV);
                imgui_.CheckboxFlags("ImGuiTableFlags_BordersOuterV", &flags, ImGuiTableFlags_BordersOuterV);
                imgui_.CheckboxFlags("ImGuiTableFlags_BordersInnerV", &flags, ImGuiTableFlags_BordersInnerV);
                imgui_.CheckboxFlags("ImGuiTableFlags_BordersH", &flags, ImGuiTableFlags_BordersH);
                imgui_.CheckboxFlags("ImGuiTableFlags_BordersOuterH", &flags, ImGuiTableFlags_BordersOuterH);
                imgui_.CheckboxFlags("ImGuiTableFlags_BordersInnerH", &flags, ImGuiTableFlags_BordersInnerH);
                imgui_.CheckboxFlags("ImGuiTableFlags_NoBordersInBody", &flags, ImGuiTableFlags_NoBordersInBody); imgui_.SameLine(); HelpMarker(imgui_, "Disable vertical borders in columns Body (borders will always appears in Headers");
                imgui_.CheckboxFlags("ImGuiTableFlags_NoBordersInBodyUntilResize", &flags, ImGuiTableFlags_NoBordersInBodyUntilResize); imgui_.SameLine(); HelpMarker(imgui_, "Disable vertical borders in columns Body until hovered for resize (borders will always appears in Headers)");
                imgui_.TreePop();
            }

            if (imgui_.TreeNodeEx("Sizing:", ImGuiTreeNodeFlags_DefaultOpen))
            {
                EditTableSizingFlags(imgui_, &flags);
                imgui_.SameLine(); HelpMarker(imgui_, "In the Advanced demo we override the policy of each column so those table-wide settings have less effect that typical.");
                imgui_.CheckboxFlags("ImGuiTableFlags_NoHostExtendX", &flags, ImGuiTableFlags_NoHostExtendX);
                imgui_.SameLine(); HelpMarker(imgui_, "Make outer width auto-fit to columns, overriding outer_size.x value.\n\nOnly available when ScrollX/ScrollY are disabled and Stretch columns are not used.");
                imgui_.CheckboxFlags("ImGuiTableFlags_NoHostExtendY", &flags, ImGuiTableFlags_NoHostExtendY);
                imgui_.SameLine(); HelpMarker(imgui_, "Make outer height stop exactly at outer_size.y (prevent auto-extending table past the limit).\n\nOnly available when ScrollX/ScrollY are disabled. Data below the limit will be clipped and not visible.");
                imgui_.CheckboxFlags("ImGuiTableFlags_NoKeepColumnsVisible", &flags, ImGuiTableFlags_NoKeepColumnsVisible);
                imgui_.SameLine(); HelpMarker(imgui_, "Only available if ScrollX is disabled.");
                imgui_.CheckboxFlags("ImGuiTableFlags_PreciseWidths", &flags, ImGuiTableFlags_PreciseWidths);
                imgui_.SameLine(); HelpMarker(imgui_, "Disable distributing remainder width to stretched columns (width allocation on a 100-wide table with 3 columns: Without this flag: 33,33,34. With this flag: 33,33,33). With larger number of columns, resizing will appear to be less smooth.");
                imgui_.CheckboxFlags("ImGuiTableFlags_NoClip", &flags, ImGuiTableFlags_NoClip);
                imgui_.SameLine(); HelpMarker(imgui_, "Disable clipping rectangle for every individual columns (reduce draw command count, items will be able to overflow into other columns). Generally incompatible with ScrollFreeze options.");
                imgui_.TreePop();
            }

            if (imgui_.TreeNodeEx("Padding:", ImGuiTreeNodeFlags_DefaultOpen))
            {
                imgui_.CheckboxFlags("ImGuiTableFlags_PadOuterX", &flags, ImGuiTableFlags_PadOuterX);
                imgui_.CheckboxFlags("ImGuiTableFlags_NoPadOuterX", &flags, ImGuiTableFlags_NoPadOuterX);
                imgui_.CheckboxFlags("ImGuiTableFlags_NoPadInnerX", &flags, ImGuiTableFlags_NoPadInnerX);
                imgui_.TreePop();
            }

            if (imgui_.TreeNodeEx("Scrolling:", ImGuiTreeNodeFlags_DefaultOpen))
            {
                imgui_.CheckboxFlags("ImGuiTableFlags_ScrollX", &flags, ImGuiTableFlags_ScrollX);
                imgui_.SameLine();
                imgui_.SetNextItemWidth(imgui_.GetFrameHeight());
                imgui_.DragInt("freeze_cols", &freeze_cols, 0.2f, 0, 9, NULL, ImGuiSliderFlags_NoInput);
                imgui_.CheckboxFlags("ImGuiTableFlags_ScrollY", &flags, ImGuiTableFlags_ScrollY);
                imgui_.SameLine();
                imgui_.SetNextItemWidth(imgui_.GetFrameHeight());
                imgui_.DragInt("freeze_rows", &freeze_rows, 0.2f, 0, 9, NULL, ImGuiSliderFlags_NoInput);
                imgui_.TreePop();
            }

            if (imgui_.TreeNodeEx("Sorting:", ImGuiTreeNodeFlags_DefaultOpen))
            {
                imgui_.CheckboxFlags("ImGuiTableFlags_SortMulti", &flags, ImGuiTableFlags_SortMulti);
                imgui_.SameLine(); HelpMarker(imgui_, "When sorting is enabled: hold shift when clicking headers to sort on multiple column. TableGetSortSpecs() may return specs where (SpecsCount > 1).");
                imgui_.CheckboxFlags("ImGuiTableFlags_SortTristate", &flags, ImGuiTableFlags_SortTristate);
                imgui_.SameLine(); HelpMarker(imgui_, "When sorting is enabled: allow no sorting, disable default sorting. TableGetSortSpecs() may return specs where (SpecsCount == 0).");
                imgui_.TreePop();
            }

            if (imgui_.TreeNodeEx("Other:", ImGuiTreeNodeFlags_DefaultOpen))
            {
                imgui_.Checkbox("show_headers", &show_headers);
                imgui_.Checkbox("show_wrapped_text", &show_wrapped_text);

                imgui_.DragFloat2("##OuterSize", &outer_size_value.x);
                imgui_.SameLine(0.0f, imgui_.GetStyle().ItemInnerSpacing.x);
                imgui_.Checkbox("outer_size", &outer_size_enabled);
                imgui_.SameLine();
                HelpMarker(imgui_, "If scrolling is disabled (ScrollX and ScrollY not set):\n"
                    "- The table is output directly in the parent window.\n"
                    "- OuterSize.x < 0.0f will right-align the table.\n"
                    "- OuterSize.x = 0.0f will narrow fit the table unless there are any Stretch column.\n"
                    "- OuterSize.y then becomes the minimum size for the table, which will extend vertically if there are more rows (unless NoHostExtendY is set).");

                // From a user point of view we will tend to use 'inner_width' differently depending on whether our table is embedding scrolling.
                // To facilitate toying with this demo we will actually pass 0.0f to the BeginTable() when ScrollX is disabled.
                imgui_.DragFloat("inner_width (when ScrollX active)", &inner_width_with_scroll, 1.0f, 0.0f, FLT_MAX);

                imgui_.DragFloat("row_min_height", &row_min_height, 1.0f, 0.0f, FLT_MAX);
                imgui_.SameLine(); HelpMarker(imgui_, "Specify height of the Selectable item.");

                imgui_.DragInt("items_count", &items_count, 0.1f, 0, 9999);
                imgui_.Combo("items_type (first column)", &contents_type, contents_type_names, IM_ARRAYSIZE(contents_type_names));
                //filter.Draw("filter");
                imgui_.TreePop();
            }

            imgui_.PopItemWidth();
            PopStyleCompact(imgui_);
            imgui_.Spacing();
            imgui_.TreePop();
        }

        // Update item list if we changed the number of items
        static ImVector<MyItem> items(imgui_);
        static ImVector<int> selection(imgui_);
        static bool items_need_sort = false;
        if (items.Size != items_count)
        {
            items.resize(items_count, MyItem());
            for (int n = 0; n < items_count; n++)
            {
                const int template_n = n % IM_ARRAYSIZE(template_items_names);
                MyItem& item = items[n];
                item.ID = n;
                item.Name = template_items_names[template_n];
                item.Quantity = (template_n == 3) ? 10 : (template_n == 4) ? 20 : 0; // Assign default quantities
            }
        }

        const ImDrawList* parent_draw_list = imgui_.GetWindowDrawList();
        const int parent_draw_list_draw_cmd_count = parent_draw_list->CmdBuffer.Size;
        ImVec2 table_scroll_cur, table_scroll_max; // For debug display
        const ImDrawList* table_draw_list = NULL;  // "

        // Submit table
        const float inner_width_to_use = (flags & ImGuiTableFlags_ScrollX) ? inner_width_with_scroll : 0.0f;
        if (imgui_.BeginTable("table_advanced", 6, flags, outer_size_enabled ? outer_size_value : ImVec2(0, 0), inner_width_to_use))
        {
            // Declare columns
            // We use the "user_id" parameter of TableSetupColumn() to specify a user id that will be stored in the sort specifications.
            // This is so our sort function can identify a column given our own identifier. We could also identify them based on their index!
            imgui_.TableSetupColumn("ID",           ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.0f, MyItemColumnID_ID);
            imgui_.TableSetupColumn("Name",         ImGuiTableColumnFlags_WidthFixed, 0.0f, MyItemColumnID_Name);
            imgui_.TableSetupColumn("Action",       ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, MyItemColumnID_Action);
            imgui_.TableSetupColumn("Quantity",     ImGuiTableColumnFlags_PreferSortDescending, 0.0f, MyItemColumnID_Quantity);
            imgui_.TableSetupColumn("Description",  (flags & ImGuiTableFlags_NoHostExtendX) ? 0 : ImGuiTableColumnFlags_WidthStretch, 0.0f, MyItemColumnID_Description);
            imgui_.TableSetupColumn("Hidden",       ImGuiTableColumnFlags_DefaultHide | ImGuiTableColumnFlags_NoSort);
            imgui_.TableSetupScrollFreeze(freeze_cols, freeze_rows);

            // Sort our data if sort specs have been changed!
            ImGuiTableSortSpecs* sorts_specs = imgui_.TableGetSortSpecs();
            if (sorts_specs && sorts_specs->SpecsDirty)
                items_need_sort = true;
            if (sorts_specs && items_need_sort && items.Size > 1)
            {
                MyItem::s_current_sort_specs = sorts_specs; // Store in variable accessible by the sort function.
                qsort(&items[0], (size_t)items.Size, sizeof(items[0]), MyItem::CompareWithSortSpecs);
                MyItem::s_current_sort_specs = NULL;
                sorts_specs->SpecsDirty = false;
            }
            items_need_sort = false;

            // Take note of whether we are currently sorting based on the Quantity field,
            // we will use this to trigger sorting when we know the data of this column has been modified.
            const bool sorts_specs_using_quantity = (imgui_.TableGetColumnFlags(3) & ImGuiTableColumnFlags_IsSorted) != 0;

            // Show headers
            if (show_headers)
                imgui_.TableHeadersRow();

            // Show data
            // FIXME-TABLE FIXME-NAV: How we can get decent up/down even though we have the buttons here?
            imgui_.PushButtonRepeat(true);
#if 1
            // Demonstrate using clipper for large vertical lists
            ImGuiListClipper clipper(imgui_);
            clipper.Begin(items.Size);
            while (clipper.Step())
            {
                for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
#else
            // Without clipper
            {
                for (int row_n = 0; row_n < items.Size; row_n++)
#endif
                {
                    MyItem* item = &items[row_n];
                    //if (!filter.PassFilter(item->Name))
                    //    continue;

                    const bool item_is_selected = selection.contains(item->ID);
                    imgui_.PushID(item->ID);
                    imgui_.TableNextRow(ImGuiTableRowFlags_None, row_min_height);

                    // For the demo purpose we can select among different type of items submitted in the first column
                    imgui_.TableSetColumnIndex(0);
                    char label[32];
                    sprintf(label, "%04d", item->ID);
                    if (contents_type == CT_Text)
                        imgui_.TextUnformatted(label);
                    else if (contents_type == CT_Button)
                        imgui_.Button(label);
                    else if (contents_type == CT_SmallButton)
                        imgui_.SmallButton(label);
                    else if (contents_type == CT_FillButton)
                        imgui_.Button(label, ImVec2(-FLT_MIN, 0.0f));
                    else if (contents_type == CT_Selectable || contents_type == CT_SelectableSpanRow)
                    {
                        ImGuiSelectableFlags selectable_flags = (contents_type == CT_SelectableSpanRow) ? ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap : ImGuiSelectableFlags_None;
                        if (imgui_.Selectable(label, item_is_selected, selectable_flags, ImVec2(0, row_min_height)))
                        {
                            if (imgui_.GetIO().KeyCtrl)
                            {
                                if (item_is_selected)
                                    selection.find_erase_unsorted(item->ID);
                                else
                                    selection.push_back(item->ID);
                            }
                            else
                            {
                                selection.clear();
                                selection.push_back(item->ID);
                            }
                        }
                    }

                    if (imgui_.TableSetColumnIndex(1))
                        imgui_.TextUnformatted(item->Name);

                    // Here we demonstrate marking our data set as needing to be sorted again if we modified a quantity,
                    // and we are currently sorting on the column showing the Quantity.
                    // To avoid triggering a sort while holding the button, we only trigger it when the button has been released.
                    // You will probably need a more advanced system in your code if you want to automatically sort when a specific entry changes.
                    if (imgui_.TableSetColumnIndex(2))
                    {
                        if (imgui_.SmallButton("Chop")) { item->Quantity += 1; }
                        if (sorts_specs_using_quantity && imgui_.IsItemDeactivated()) { items_need_sort = true; }
                        imgui_.SameLine();
                        if (imgui_.SmallButton("Eat")) { item->Quantity -= 1; }
                        if (sorts_specs_using_quantity && imgui_.IsItemDeactivated()) { items_need_sort = true; }
                    }

                    if (imgui_.TableSetColumnIndex(3))
                        imgui_.Text("%d", item->Quantity);

                    imgui_.TableSetColumnIndex(4);
                    if (show_wrapped_text)
                        imgui_.TextWrapped("Lorem ipsum dolor sit amet");
                    else
                        imgui_.Text("Lorem ipsum dolor sit amet");

                    if (imgui_.TableSetColumnIndex(5))
                        imgui_.Text("1234");

                    imgui_.PopID();
                }
            }
            imgui_.PopButtonRepeat();

            // Store some info to display debug details below
            table_scroll_cur = ImVec2(imgui_.GetScrollX(), imgui_.GetScrollY());
            table_scroll_max = ImVec2(imgui_.GetScrollMaxX(), imgui_.GetScrollMaxY());
            table_draw_list = imgui_.GetWindowDrawList();
            imgui_.EndTable();
        }
        static bool show_debug_details = false;
        imgui_.Checkbox("Debug details", &show_debug_details);
        if (show_debug_details && table_draw_list)
        {
            imgui_.SameLine(0.0f, 0.0f);
            const int table_draw_list_draw_cmd_count = table_draw_list->CmdBuffer.Size;
            if (table_draw_list == parent_draw_list)
                imgui_.Text(": DrawCmd: +%d (in same window)",
                    table_draw_list_draw_cmd_count - parent_draw_list_draw_cmd_count);
            else
                imgui_.Text(": DrawCmd: +%d (in child window), Scroll: (%.f/%.f) (%.f/%.f)",
                    table_draw_list_draw_cmd_count - 1, table_scroll_cur.x, table_scroll_max.x, table_scroll_cur.y, table_scroll_max.y);
        }
        imgui_.TreePop();
    }

    imgui_.PopID();

    ShowDemoWindowColumns(imgui_);

    if (disable_indent)
        imgui_.PopStyleVar();
}

// Demonstrate old/legacy Columns API!
// [2020: Columns are under-featured and not maintained. Prefer using the more flexible and powerful BeginTable() API!]
static void ShowDemoWindowColumns(ImGui& imgui_)
{
    bool open = imgui_.TreeNode("Legacy Columns API");
    imgui_.SameLine();
    HelpMarker(imgui_, "Columns() is an old API! Prefer using the more flexible and powerful BeginTable() API!");
    if (!open)
        return;

    // Basic columns
    if (imgui_.TreeNode("Basic"))
    {
        imgui_.Text("Without border:");
        imgui_.Columns(3, "mycolumns3", false);  // 3-ways, no border
        imgui_.Separator();
        for (int n = 0; n < 14; n++)
        {
            char label[32];
            sprintf(label, "Item %d", n);
            if (imgui_.Selectable(label)) {}
            //if (imgui_.Button(label, ImVec2(-FLT_MIN,0.0f))) {}
            imgui_.NextColumn();
        }
        imgui_.Columns(1);
        imgui_.Separator();

        imgui_.Text("With border:");
        imgui_.Columns(4, "mycolumns"); // 4-ways, with border
        imgui_.Separator();
        imgui_.Text("ID"); imgui_.NextColumn();
        imgui_.Text("Name"); imgui_.NextColumn();
        imgui_.Text("Path"); imgui_.NextColumn();
        imgui_.Text("Hovered"); imgui_.NextColumn();
        imgui_.Separator();
        const char* names[3] = { "One", "Two", "Three" };
        const char* paths[3] = { "/path/one", "/path/two", "/path/three" };
        static int selected = -1;
        for (int i = 0; i < 3; i++)
        {
            char label[32];
            sprintf(label, "%04d", i);
            if (imgui_.Selectable(label, selected == i, ImGuiSelectableFlags_SpanAllColumns))
                selected = i;
            bool hovered = imgui_.IsItemHovered();
            imgui_.NextColumn();
            imgui_.Text(names[i]); imgui_.NextColumn();
            imgui_.Text(paths[i]); imgui_.NextColumn();
            imgui_.Text("%d", hovered); imgui_.NextColumn();
        }
        imgui_.Columns(1);
        imgui_.Separator();
        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Borders"))
    {
        // NB: Future columns API should allow automatic horizontal borders.
        static bool h_borders = true;
        static bool v_borders = true;
        static int columns_count = 4;
        const int lines_count = 3;
        imgui_.SetNextItemWidth(imgui_.GetFontSize() * 8);
        imgui_.DragInt("##columns_count", &columns_count, 0.1f, 2, 10, "%d columns");
        if (columns_count < 2)
            columns_count = 2;
        imgui_.SameLine();
        imgui_.Checkbox("horizontal", &h_borders);
        imgui_.SameLine();
        imgui_.Checkbox("vertical", &v_borders);
        imgui_.Columns(columns_count, NULL, v_borders);
        for (int i = 0; i < columns_count * lines_count; i++)
        {
            if (h_borders && imgui_.GetColumnIndex() == 0)
                imgui_.Separator();
            imgui_.Text("%c%c%c", 'a' + i, 'a' + i, 'a' + i);
            imgui_.Text("Width %.2f", imgui_.GetColumnWidth());
            imgui_.Text("Avail %.2f", imgui_.GetContentRegionAvail().x);
            imgui_.Text("Offset %.2f", imgui_.GetColumnOffset());
            imgui_.Text("Long text that is likely to clip");
            imgui_.Button("Button", ImVec2(-FLT_MIN, 0.0f));
            imgui_.NextColumn();
        }
        imgui_.Columns(1);
        if (h_borders)
            imgui_.Separator();
        imgui_.TreePop();
    }

    // Create multiple items in a same cell before switching to next column
    if (imgui_.TreeNode("Mixed items"))
    {
        imgui_.Columns(3, "mixed");
        imgui_.Separator();

        imgui_.Text("Hello");
        imgui_.Button("Banana");
        imgui_.NextColumn();

        imgui_.Text("ImGui");
        imgui_.Button("Apple");
        static float foo = 1.0f;
        imgui_.InputFloat("red", &foo, 0.05f, 0, "%.3f");
        imgui_.Text("An extra line here.");
        imgui_.NextColumn();

        imgui_.Text("Sailor");
        imgui_.Button("Corniflower");
        static float bar = 1.0f;
        imgui_.InputFloat("blue", &bar, 0.05f, 0, "%.3f");
        imgui_.NextColumn();

        if (imgui_.CollapsingHeader("Category A")) { imgui_.Text("Blah blah blah"); } imgui_.NextColumn();
        if (imgui_.CollapsingHeader("Category B")) { imgui_.Text("Blah blah blah"); } imgui_.NextColumn();
        if (imgui_.CollapsingHeader("Category C")) { imgui_.Text("Blah blah blah"); } imgui_.NextColumn();
        imgui_.Columns(1);
        imgui_.Separator();
        imgui_.TreePop();
    }

    // Word wrapping
    if (imgui_.TreeNode("Word-wrapping"))
    {
        imgui_.Columns(2, "word-wrapping");
        imgui_.Separator();
        imgui_.TextWrapped("The quick brown fox jumps over the lazy dog.");
        imgui_.TextWrapped("Hello Left");
        imgui_.NextColumn();
        imgui_.TextWrapped("The quick brown fox jumps over the lazy dog.");
        imgui_.TextWrapped("Hello Right");
        imgui_.Columns(1);
        imgui_.Separator();
        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Horizontal Scrolling"))
    {
        imgui_.SetNextWindowContentSize(ImVec2(1500.0f, 0.0f));
        ImVec2 child_size = ImVec2(0, imgui_.GetFontSize() * 20.0f);
        imgui_.BeginChild("##ScrollingRegion", child_size, false, ImGuiWindowFlags_HorizontalScrollbar);
        imgui_.Columns(10);

        // Also demonstrate using clipper for large vertical lists
        int ITEMS_COUNT = 2000;
        ImGuiListClipper clipper(imgui_);
        clipper.Begin(ITEMS_COUNT);
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                for (int j = 0; j < 10; j++)
                {
                    imgui_.Text("Line %d Column %d...", i, j);
                    imgui_.NextColumn();
                }
        }
        imgui_.Columns(1);
        imgui_.EndChild();
        imgui_.TreePop();
    }

    if (imgui_.TreeNode("Tree"))
    {
        imgui_.Columns(2, "tree", true);
        for (int x = 0; x < 3; x++)
        {
            bool open1 = imgui_.TreeNode((void*)(intptr_t)x, "Node%d", x);
            imgui_.NextColumn();
            imgui_.Text("Node contents");
            imgui_.NextColumn();
            if (open1)
            {
                for (int y = 0; y < 3; y++)
                {
                    bool open2 = imgui_.TreeNode((void*)(intptr_t)y, "Node%d.%d", x, y);
                    imgui_.NextColumn();
                    imgui_.Text("Node contents");
                    if (open2)
                    {
                        imgui_.Text("Even more contents");
                        if (imgui_.TreeNode("Tree in column"))
                        {
                            imgui_.Text("The quick brown fox jumps over the lazy dog");
                            imgui_.TreePop();
                        }
                    }
                    imgui_.NextColumn();
                    if (open2)
                        imgui_.TreePop();
                }
                imgui_.TreePop();
            }
        }
        imgui_.Columns(1);
        imgui_.TreePop();
    }

    imgui_.TreePop();
}

static void ShowDemoWindowMisc(ImGui& imgui_)
{
    if (imgui_.CollapsingHeader("Filtering"))
    {
        // Helper class to easy setup a text filter.
        // You may want to implement a more feature-full filtering scheme in your own application.
        static ImGuiTextFilter filter(imgui_);
        imgui_.Text("Filter usage:\n"
                    "  \"\"         display all lines\n"
                    "  \"xxx\"      display lines containing \"xxx\"\n"
                    "  \"xxx,yyy\"  display lines containing \"xxx\" or \"yyy\"\n"
                    "  \"-xxx\"     hide lines containing \"xxx\"");
        filter.Draw();
        const char* lines[] = { "aaa1.c", "bbb1.c", "ccc1.c", "aaa2.cpp", "bbb2.cpp", "ccc2.cpp", "abc.h", "hello, world" };
        for (int i = 0; i < IM_ARRAYSIZE(lines); i++)
            if (filter.PassFilter(lines[i]))
                imgui_.BulletText("%s", lines[i]);
    }

    if (imgui_.CollapsingHeader("Inputs, Navigation & Focus"))
    {
        ImGuiIO& io = imgui_.GetIO();

        // Display ImGuiIO output flags
        imgui_.Text("WantCaptureMouse: %d", io.WantCaptureMouse);
        imgui_.Text("WantCaptureKeyboard: %d", io.WantCaptureKeyboard);
        imgui_.Text("WantTextInput: %d", io.WantTextInput);
        imgui_.Text("WantSetMousePos: %d", io.WantSetMousePos);
        imgui_.Text("NavActive: %d, NavVisible: %d", io.NavActive, io.NavVisible);

        // Display Mouse state
        if (imgui_.TreeNode("Mouse State"))
        {
            if (imgui_.IsMousePosValid())
                imgui_.Text("Mouse pos: (%g, %g)", io.MousePos.x, io.MousePos.y);
            else
                imgui_.Text("Mouse pos: <INVALID>");
            imgui_.Text("Mouse delta: (%g, %g)", io.MouseDelta.x, io.MouseDelta.y);
            imgui_.Text("Mouse down:");     for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++) if (imgui_.IsMouseDown(i))         { imgui_.SameLine(); imgui_.Text("b%d (%.02f secs)", i, io.MouseDownDuration[i]); }
            imgui_.Text("Mouse clicked:");  for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++) if (imgui_.IsMouseClicked(i))      { imgui_.SameLine(); imgui_.Text("b%d", i); }
            imgui_.Text("Mouse dblclick:"); for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++) if (imgui_.IsMouseDoubleClicked(i)){ imgui_.SameLine(); imgui_.Text("b%d", i); }
            imgui_.Text("Mouse released:"); for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++) if (imgui_.IsMouseReleased(i))     { imgui_.SameLine(); imgui_.Text("b%d", i); }
            imgui_.Text("Mouse wheel: %.1f", io.MouseWheel);
            imgui_.Text("Pen Pressure: %.1f", io.PenPressure); // Note: currently unused
            imgui_.TreePop();
        }

        // Display Keyboard/Mouse state
        if (imgui_.TreeNode("Keyboard & Navigation State"))
        {
            imgui_.Text("Keys down:");          for (int i = 0; i < IM_ARRAYSIZE(io.KeysDown); i++) if (imgui_.IsKeyDown(i))        { imgui_.SameLine(); imgui_.Text("%d (0x%X) (%.02f secs)", i, i, io.KeysDownDuration[i]); }
            imgui_.Text("Keys pressed:");       for (int i = 0; i < IM_ARRAYSIZE(io.KeysDown); i++) if (imgui_.IsKeyPressed(i))     { imgui_.SameLine(); imgui_.Text("%d (0x%X)", i, i); }
            imgui_.Text("Keys release:");       for (int i = 0; i < IM_ARRAYSIZE(io.KeysDown); i++) if (imgui_.IsKeyReleased(i))    { imgui_.SameLine(); imgui_.Text("%d (0x%X)", i, i); }
            imgui_.Text("Keys mods: %s%s%s%s", io.KeyCtrl ? "CTRL " : "", io.KeyShift ? "SHIFT " : "", io.KeyAlt ? "ALT " : "", io.KeySuper ? "SUPER " : "");
            imgui_.Text("Chars queue:");        for (int i = 0; i < io.InputQueueCharacters.Size; i++) { ImWchar c = io.InputQueueCharacters[i]; imgui_.SameLine();  imgui_.Text("\'%c\' (0x%04X)", (c > ' ' && c <= 255) ? (char)c : '?', c); } // FIXME: We should convert 'c' to UTF-8 here but the functions are not public.

            imgui_.Text("NavInputs down:");     for (int i = 0; i < IM_ARRAYSIZE(io.NavInputs); i++) if (io.NavInputs[i] > 0.0f)              { imgui_.SameLine(); imgui_.Text("[%d] %.2f (%.02f secs)", i, io.NavInputs[i], io.NavInputsDownDuration[i]); }
            imgui_.Text("NavInputs pressed:");  for (int i = 0; i < IM_ARRAYSIZE(io.NavInputs); i++) if (io.NavInputsDownDuration[i] == 0.0f) { imgui_.SameLine(); imgui_.Text("[%d]", i); }

            imgui_.Button("Hovering me sets the\nkeyboard capture flag");
            if (imgui_.IsItemHovered())
                imgui_.CaptureKeyboardFromApp(true);
            imgui_.SameLine();
            imgui_.Button("Holding me clears the\nthe keyboard capture flag");
            if (imgui_.IsItemActive())
                imgui_.CaptureKeyboardFromApp(false);
            imgui_.TreePop();
        }

        if (imgui_.TreeNode("Tabbing"))
        {
            imgui_.Text("Use TAB/SHIFT+TAB to cycle through keyboard editable fields.");
            static char buf[32] = "hello";
            imgui_.InputText("1", buf, IM_ARRAYSIZE(buf));
            imgui_.InputText("2", buf, IM_ARRAYSIZE(buf));
            imgui_.InputText("3", buf, IM_ARRAYSIZE(buf));
            imgui_.PushAllowKeyboardFocus(false);
            imgui_.InputText("4 (tab skip)", buf, IM_ARRAYSIZE(buf));
            //imgui_.SameLine(); HelpMarker(imgui_, "Use imgui_.PushAllowKeyboardFocus(bool) to disable tabbing through certain widgets.");
            imgui_.PopAllowKeyboardFocus();
            imgui_.InputText("5", buf, IM_ARRAYSIZE(buf));
            imgui_.TreePop();
        }

        if (imgui_.TreeNode("Focus from code"))
        {
            bool focus_1 = imgui_.Button("Focus on 1"); imgui_.SameLine();
            bool focus_2 = imgui_.Button("Focus on 2"); imgui_.SameLine();
            bool focus_3 = imgui_.Button("Focus on 3");
            int has_focus = 0;
            static char buf[128] = "click on a button to set focus";

            if (focus_1) imgui_.SetKeyboardFocusHere();
            imgui_.InputText("1", buf, IM_ARRAYSIZE(buf));
            if (imgui_.IsItemActive()) has_focus = 1;

            if (focus_2) imgui_.SetKeyboardFocusHere();
            imgui_.InputText("2", buf, IM_ARRAYSIZE(buf));
            if (imgui_.IsItemActive()) has_focus = 2;

            imgui_.PushAllowKeyboardFocus(false);
            if (focus_3) imgui_.SetKeyboardFocusHere();
            imgui_.InputText("3 (tab skip)", buf, IM_ARRAYSIZE(buf));
            if (imgui_.IsItemActive()) has_focus = 3;
            imgui_.PopAllowKeyboardFocus();

            if (has_focus)
                imgui_.Text("Item with focus: %d", has_focus);
            else
                imgui_.Text("Item with focus: <none>");

            // Use >= 0 parameter to SetKeyboardFocusHere() to focus an upcoming item
            static float f3[3] = { 0.0f, 0.0f, 0.0f };
            int focus_ahead = -1;
            if (imgui_.Button("Focus on X")) { focus_ahead = 0; } imgui_.SameLine();
            if (imgui_.Button("Focus on Y")) { focus_ahead = 1; } imgui_.SameLine();
            if (imgui_.Button("Focus on Z")) { focus_ahead = 2; }
            if (focus_ahead != -1) imgui_.SetKeyboardFocusHere(focus_ahead);
            imgui_.SliderFloat3("Float3", &f3[0], 0.0f, 1.0f);

            imgui_.TextWrapped("NB: Cursor & selection are preserved when refocusing last used item in code.");
            imgui_.TreePop();
        }

        if (imgui_.TreeNode("Dragging"))
        {
            imgui_.TextWrapped("You can use imgui_.GetMouseDragDelta(0) to query for the dragged amount on any widget.");
            for (int button = 0; button < 3; button++)
            {
                imgui_.Text("IsMouseDragging(%d):", button);
                imgui_.Text("  w/ default threshold: %d,", imgui_.IsMouseDragging(button));
                imgui_.Text("  w/ zero threshold: %d,", imgui_.IsMouseDragging(button, 0.0f));
                imgui_.Text("  w/ large threshold: %d,", imgui_.IsMouseDragging(button, 20.0f));
            }

            imgui_.Button("Drag Me");
            if (imgui_.IsItemActive())
                imgui_.GetForegroundDrawList()->AddLine(io.MouseClickedPos[0], io.MousePos, imgui_.GetColorU32(ImGuiCol_Button), 4.0f); // Draw a line between the button and the mouse cursor

            // Drag operations gets "unlocked" when the mouse has moved past a certain threshold
            // (the default threshold is stored in io.MouseDragThreshold). You can request a lower or higher
            // threshold using the second parameter of IsMouseDragging() and GetMouseDragDelta().
            ImVec2 value_raw = imgui_.GetMouseDragDelta(0, 0.0f);
            ImVec2 value_with_lock_threshold = imgui_.GetMouseDragDelta(0);
            ImVec2 mouse_delta = io.MouseDelta;
            imgui_.Text("GetMouseDragDelta(0):");
            imgui_.Text("  w/ default threshold: (%.1f, %.1f)", value_with_lock_threshold.x, value_with_lock_threshold.y);
            imgui_.Text("  w/ zero threshold: (%.1f, %.1f)", value_raw.x, value_raw.y);
            imgui_.Text("io.MouseDelta: (%.1f, %.1f)", mouse_delta.x, mouse_delta.y);
            imgui_.TreePop();
        }

        if (imgui_.TreeNode("Mouse cursors"))
        {
            const char* mouse_cursors_names[] = { "Arrow", "TextInput", "ResizeAll", "ResizeNS", "ResizeEW", "ResizeNESW", "ResizeNWSE", "Hand", "NotAllowed" };
            IM_ASSERT(IM_ARRAYSIZE(mouse_cursors_names) == ImGuiMouseCursor_COUNT);

            ImGuiMouseCursor current = imgui_.GetMouseCursor();
            imgui_.Text("Current mouse cursor = %d: %s", current, mouse_cursors_names[current]);
            imgui_.Text("Hover to see mouse cursors:");
            imgui_.SameLine(); HelpMarker(imgui_,
                "Your application can render a different mouse cursor based on what imgui_.GetMouseCursor() returns. "
                "If software cursor rendering (io.MouseDrawCursor) is set ImGui will draw the right cursor for you, "
                "otherwise your backend needs to handle it.");
            for (int i = 0; i < ImGuiMouseCursor_COUNT; i++)
            {
                char label[32];
                sprintf(label, "Mouse cursor %d: %s", i, mouse_cursors_names[i]);
                imgui_.Bullet(); imgui_.Selectable(label, false);
                if (imgui_.IsItemHovered())
                    imgui_.SetMouseCursor(i);
            }
            imgui_.TreePop();
        }
    }
}

//-----------------------------------------------------------------------------
// [SECTION] About Window / ShowAboutWindow()
// Access from Dear ImGui Demo -> Tools -> About
//-----------------------------------------------------------------------------

void ImGui::ShowAboutWindow(bool* p_open)
{
    if (!Begin("About Dear ImGui", p_open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        End();
        return;
    }
    Text("Dear ImGui %s", GetVersion());
    Separator();
    Text("By Omar Cornut and all Dear ImGui contributors.");
    Text("Dear ImGui is licensed under the MIT License, see LICENSE for more information.");

    static bool show_config_info = false;
    Checkbox("Config/Build Information", &show_config_info);
    if (show_config_info)
    {
        ImGuiIO& io = GetIO();
        ImGuiStyle& style = GetStyle();

        bool copy_to_clipboard = Button("Copy to clipboard");
        ImVec2 child_size = ImVec2(0, GetTextLineHeightWithSpacing() * 18);
        BeginChildFrame(GetID("cfg_infos"), child_size, ImGuiWindowFlags_NoMove);
        if (copy_to_clipboard)
        {
            LogToClipboard();
            LogText("```\n"); // Back quotes will make text appears without formatting when pasting on GitHub
        }

        Text("Dear ImGui %s (%d)", IMGUI_VERSION, IMGUI_VERSION_NUM);
        Separator();
        Text("sizeof(size_t): %d, sizeof(ImDrawIdx): %d, sizeof(ImDrawVert): %d", (int)sizeof(size_t), (int)sizeof(ImDrawIdx), (int)sizeof(ImDrawVert));
        Text("define: __cplusplus=%d", (int)__cplusplus);
#ifdef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
        Text("define: IMGUI_DISABLE_OBSOLETE_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS
        Text("define: IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
        Text("define: IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_WIN32_FUNCTIONS
        Text("define: IMGUI_DISABLE_WIN32_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS
        Text("define: IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_DEFAULT_MATH_FUNCTIONS
        Text("define: IMGUI_DISABLE_DEFAULT_MATH_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS
        Text("define: IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_FILE_FUNCTIONS
        Text("define: IMGUI_DISABLE_FILE_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_DEFAULT_ALLOCATORS
        Text("define: IMGUI_DISABLE_DEFAULT_ALLOCATORS");
#endif
#ifdef IMGUI_USE_BGRA_PACKED_COLOR
        Text("define: IMGUI_USE_BGRA_PACKED_COLOR");
#endif
#ifdef _WIN32
        Text("define: _WIN32");
#endif
#ifdef _WIN64
        Text("define: _WIN64");
#endif
#ifdef __linux__
        Text("define: __linux__");
#endif
#ifdef __APPLE__
        Text("define: __APPLE__");
#endif
#ifdef _MSC_VER
        Text("define: _MSC_VER=%d", _MSC_VER);
#endif
#ifdef _MSVC_LANG
        Text("define: _MSVC_LANG=%d", (int)_MSVC_LANG);
#endif
#ifdef __MINGW32__
        Text("define: __MINGW32__");
#endif
#ifdef __MINGW64__
        Text("define: __MINGW64__");
#endif
#ifdef __GNUC__
        Text("define: __GNUC__=%d", (int)__GNUC__);
#endif
#ifdef __clang_version__
        Text("define: __clang_version__=%s", __clang_version__);
#endif
        Separator();
        Text("io.BackendPlatformName: %s", io.BackendPlatformName ? io.BackendPlatformName : "NULL");
        Text("io.BackendRendererName: %s", io.BackendRendererName ? io.BackendRendererName : "NULL");
        Text("io.ConfigFlags: 0x%08X", io.ConfigFlags);
        if (io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard)        Text(" NavEnableKeyboard");
        if (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad)         Text(" NavEnableGamepad");
        if (io.ConfigFlags & ImGuiConfigFlags_NavEnableSetMousePos)     Text(" NavEnableSetMousePos");
        if (io.ConfigFlags & ImGuiConfigFlags_NavNoCaptureKeyboard)     Text(" NavNoCaptureKeyboard");
        if (io.ConfigFlags & ImGuiConfigFlags_NoMouse)                  Text(" NoMouse");
        if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)      Text(" NoMouseCursorChange");
        if (io.MouseDrawCursor)                                         Text("io.MouseDrawCursor");
        if (io.ConfigMacOSXBehaviors)                                   Text("io.ConfigMacOSXBehaviors");
        if (io.ConfigInputTextCursorBlink)                              Text("io.ConfigInputTextCursorBlink");
        if (io.ConfigWindowsResizeFromEdges)                            Text("io.ConfigWindowsResizeFromEdges");
        if (io.ConfigWindowsMoveFromTitleBarOnly)                       Text("io.ConfigWindowsMoveFromTitleBarOnly");
        if (io.ConfigMemoryCompactTimer >= 0.0f)                        Text("io.ConfigMemoryCompactTimer = %.1f", io.ConfigMemoryCompactTimer);
        Text("io.BackendFlags: 0x%08X", io.BackendFlags);
        if (io.BackendFlags & ImGuiBackendFlags_HasGamepad)             Text(" HasGamepad");
        if (io.BackendFlags & ImGuiBackendFlags_HasMouseCursors)        Text(" HasMouseCursors");
        if (io.BackendFlags & ImGuiBackendFlags_HasSetMousePos)         Text(" HasSetMousePos");
        if (io.BackendFlags & ImGuiBackendFlags_RendererHasVtxOffset)   Text(" RendererHasVtxOffset");
        Separator();
        Text("io.Fonts: %d fonts, Flags: 0x%08X, TexSize: %d,%d", io.Fonts->Fonts.Size, io.Fonts->Flags, io.Fonts->TexWidth, io.Fonts->TexHeight);
        Text("io.DisplaySize: %.2f,%.2f", io.DisplaySize.x, io.DisplaySize.y);
        Text("io.DisplayFramebufferScale: %.2f,%.2f", io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        Separator();
        Text("style.WindowPadding: %.2f,%.2f", style.WindowPadding.x, style.WindowPadding.y);
        Text("style.WindowBorderSize: %.2f", style.WindowBorderSize);
        Text("style.FramePadding: %.2f,%.2f", style.FramePadding.x, style.FramePadding.y);
        Text("style.FrameRounding: %.2f", style.FrameRounding);
        Text("style.FrameBorderSize: %.2f", style.FrameBorderSize);
        Text("style.ItemSpacing: %.2f,%.2f", style.ItemSpacing.x, style.ItemSpacing.y);
        Text("style.ItemInnerSpacing: %.2f,%.2f", style.ItemInnerSpacing.x, style.ItemInnerSpacing.y);

        if (copy_to_clipboard)
        {
            LogText("\n```\n");
            LogFinish();
        }
        EndChildFrame();
    }
    End();
}

//-----------------------------------------------------------------------------
// [SECTION] Style Editor / ShowStyleEditor()
//-----------------------------------------------------------------------------
// - ShowStyleSelector()
// - ShowFontSelector()
// - ShowStyleEditor()
//-----------------------------------------------------------------------------

// Demo helper function to select among default colors. See ShowStyleEditor() for more advanced options.
// Here we use the simplified Combo() api that packs items into a single literal string.
// Useful for quick combo boxes where the choices are known locally.
bool ImGui::ShowStyleSelector(const char* label)
{
    static int style_idx = -1;
    if (Combo(label, &style_idx, "Dark\0Light\0Classic\0"))
    {
        switch (style_idx)
        {
        case 0: StyleColorsDark(); break;
        case 1: StyleColorsLight(); break;
        case 2: StyleColorsClassic(); break;
        }
        return true;
    }
    return false;
}

// Demo helper function to select among loaded fonts.
// Here we use the regular BeginCombo()/EndCombo() api which is more the more flexible one.
void ImGui::ShowFontSelector(const char* label)
{
    ImGuiIO& io = GetIO();
    ImFont* font_current = GetFont();
    if (BeginCombo(label, font_current->GetDebugName()))
    {
        for (int n = 0; n < io.Fonts->Fonts.Size; n++)
        {
            ImFont* font = io.Fonts->Fonts[n];
            PushID((void*)font);
            if (Selectable(font->GetDebugName(), font == font_current))
                io.FontDefault = font;
            PopID();
        }
        EndCombo();
    }
    SameLine();
    HelpMarker(*this,
        "- Load additional fonts with io.Fonts->AddFontFromFileTTF().\n"
        "- The font atlas is built when calling io.Fonts->GetTexDataAsXXXX() or io.Fonts->Build().\n"
        "- Read FAQ and docs/FONTS.md for more details.\n"
        "- If you need to add/remove fonts at runtime (e.g. for DPI change), do it before calling NewFrame().");
}

// [Internal] Display details for a single font, called by ShowStyleEditor().
static void NodeFont(ImGui& imgui_, ImFont* font)
{
    ImGuiIO& io = imgui_.GetIO();
    ImGuiStyle& style = imgui_.GetStyle();
    bool font_details_opened = imgui_.TreeNode(font, "Font: \"%s\"\n%.2f px, %d glyphs, %d file(s)",
        font->ConfigData ? font->ConfigData[0].Name : "", font->FontSize, font->Glyphs.Size, font->ConfigDataCount);
    imgui_.SameLine(); if (imgui_.SmallButton("Set as default")) { io.FontDefault = font; }
    if (!font_details_opened)
        return;

    imgui_.PushFont(font);
    imgui_.Text("The quick brown fox jumps over the lazy dog");
    imgui_.PopFont();
    imgui_.DragFloat("Font scale", &font->Scale, 0.005f, 0.3f, 2.0f, "%.1f");   // Scale only this font
    imgui_.SameLine(); HelpMarker(imgui_,
        "Note than the default embedded font is NOT meant to be scaled.\n\n"
        "Font are currently rendered into bitmaps at a given size at the time of building the atlas. "
        "You may oversample them to get some flexibility with scaling. "
        "You can also render at multiple sizes and select which one to use at runtime.\n\n"
        "(Glimmer of hope: the atlas system will be rewritten in the future to make scaling more flexible.)");
    imgui_.Text("Ascent: %f, Descent: %f, Height: %f", font->Ascent, font->Descent, font->Ascent - font->Descent);
    imgui_.Text("Fallback character: '%c' (U+%04X)", font->FallbackChar, font->FallbackChar);
    imgui_.Text("Ellipsis character: '%c' (U+%04X)", font->EllipsisChar, font->EllipsisChar);
    const int surface_sqrt = (int)sqrtf((float)font->MetricsTotalSurface);
    imgui_.Text("Texture Area: about %d px ~%dx%d px", font->MetricsTotalSurface, surface_sqrt, surface_sqrt);
    for (int config_i = 0; config_i < font->ConfigDataCount; config_i++)
        if (font->ConfigData)
            if (const ImFontConfig* cfg = &font->ConfigData[config_i])
                imgui_.BulletText("Input %d: \'%s\', Oversample: (%d,%d), PixelSnapH: %d, Offset: (%.1f,%.1f)",
                    config_i, cfg->Name, cfg->OversampleH, cfg->OversampleV, cfg->PixelSnapH, cfg->GlyphOffset.x, cfg->GlyphOffset.y);
    if (imgui_.TreeNode("Glyphs", "Glyphs (%d)", font->Glyphs.Size))
    {
        // Display all glyphs of the fonts in separate pages of 256 characters
        const ImU32 glyph_col = imgui_.GetColorU32(ImGuiCol_Text);
        for (unsigned int base = 0; base <= IM_UNICODE_CODEPOINT_MAX; base += 256)
        {
            // Skip ahead if a large bunch of glyphs are not present in the font (test in chunks of 4k)
            // This is only a small optimization to reduce the number of iterations when IM_UNICODE_MAX_CODEPOINT
            // is large // (if ImWchar==ImWchar32 we will do at least about 272 queries here)
            if (!(base & 4095) && font->IsGlyphRangeUnused(base, base + 4095))
            {
                base += 4096 - 256;
                continue;
            }

            int count = 0;
            for (unsigned int n = 0; n < 256; n++)
                if (font->FindGlyphNoFallback((ImWchar)(base + n)))
                    count++;
            if (count <= 0)
                continue;
            if (!imgui_.TreeNode((void*)(intptr_t)base, "U+%04X..U+%04X (%d %s)", base, base + 255, count, count > 1 ? "glyphs" : "glyph"))
                continue;
            float cell_size = font->FontSize * 1;
            float cell_spacing = style.ItemSpacing.y;
            ImVec2 base_pos = imgui_.GetCursorScreenPos();
            ImDrawList* draw_list = imgui_.GetWindowDrawList();
            for (unsigned int n = 0; n < 256; n++)
            {
                // We use ImFont::RenderChar as a shortcut because we don't have UTF-8 conversion functions
                // available here and thus cannot easily generate a zero-terminated UTF-8 encoded string.
                ImVec2 cell_p1(base_pos.x + (n % 16) * (cell_size + cell_spacing), base_pos.y + (n / 16) * (cell_size + cell_spacing));
                ImVec2 cell_p2(cell_p1.x + cell_size, cell_p1.y + cell_size);
                const ImFontGlyph* glyph = font->FindGlyphNoFallback((ImWchar)(base + n));
                draw_list->AddRect(cell_p1, cell_p2, glyph ? IM_COL32(255, 255, 255, 100) : IM_COL32(255, 255, 255, 50));
                if (glyph)
                    font->RenderChar(draw_list, cell_size, cell_p1, glyph_col, (ImWchar)(base + n));
                if (glyph && imgui_.IsMouseHoveringRect(cell_p1, cell_p2))
                {
                    imgui_.BeginTooltip();
                    imgui_.Text("Codepoint: U+%04X", base + n);
                    imgui_.Separator();
                    imgui_.Text("Visible: %d", glyph->Visible);
                    imgui_.Text("AdvanceX: %.1f", glyph->AdvanceX);
                    imgui_.Text("Pos: (%.2f,%.2f)->(%.2f,%.2f)", glyph->X0, glyph->Y0, glyph->X1, glyph->Y1);
                    imgui_.Text("UV: (%.3f,%.3f)->(%.3f,%.3f)", glyph->U0, glyph->V0, glyph->U1, glyph->V1);
                    imgui_.EndTooltip();
                }
            }
            imgui_.Dummy(ImVec2((cell_size + cell_spacing) * 16, (cell_size + cell_spacing) * 16));
            imgui_.TreePop();
        }
        imgui_.TreePop();
    }
    imgui_.TreePop();
}

void ImGui::ShowStyleEditor(ImGuiStyle* ref)
{
    // You can pass in a reference ImGuiStyle structure to compare to, revert to and save to
    // (without a reference style pointer, we will use one compared locally as a reference)
    ImGuiStyle& style = GetStyle();
    static ImGuiStyle ref_saved_style(*this);

    // Default to using internal storage as reference
    static bool init = true;
    if (init && ref == NULL)
        ref_saved_style = style;
    init = false;
    if (ref == NULL)
        ref = &ref_saved_style;

    PushItemWidth(GetWindowWidth() * 0.50f);

    if (ShowStyleSelector("Colors##Selector"))
        ref_saved_style = style;
    ShowFontSelector("Fonts##Selector");

    // Simplified Settings (expose floating-pointer border sizes as boolean representing 0.0f or 1.0f)
    if (SliderFloat("FrameRounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f"))
        style.GrabRounding = style.FrameRounding; // Make GrabRounding always the same value as FrameRounding
    { bool border = (style.WindowBorderSize > 0.0f); if (Checkbox("WindowBorder", &border)) { style.WindowBorderSize = border ? 1.0f : 0.0f; } }
    SameLine();
    { bool border = (style.FrameBorderSize > 0.0f);  if (Checkbox("FrameBorder",  &border)) { style.FrameBorderSize  = border ? 1.0f : 0.0f; } }
    SameLine();
    { bool border = (style.PopupBorderSize > 0.0f);  if (Checkbox("PopupBorder",  &border)) { style.PopupBorderSize  = border ? 1.0f : 0.0f; } }

    // Save/Revert button
    if (Button("Save Ref"))
        *ref = ref_saved_style = style;
    SameLine();
    if (Button("Revert Ref"))
        style = *ref;
    SameLine();
    HelpMarker(*this,
        "Save/Revert in local non-persistent storage. Default Colors definition are not affected. "
        "Use \"Export\" below to save them somewhere.");

    Separator();

    if (BeginTabBar("##tabs", ImGuiTabBarFlags_None))
    {
        if (BeginTabItem("Sizes"))
        {
            Text("Main");
            SliderFloat2("WindowPadding", (float*)&style.WindowPadding, 0.0f, 20.0f, "%.0f");
            SliderFloat2("FramePadding", (float*)&style.FramePadding, 0.0f, 20.0f, "%.0f");
            SliderFloat2("CellPadding", (float*)&style.CellPadding, 0.0f, 20.0f, "%.0f");
            SliderFloat2("ItemSpacing", (float*)&style.ItemSpacing, 0.0f, 20.0f, "%.0f");
            SliderFloat2("ItemInnerSpacing", (float*)&style.ItemInnerSpacing, 0.0f, 20.0f, "%.0f");
            SliderFloat2("TouchExtraPadding", (float*)&style.TouchExtraPadding, 0.0f, 10.0f, "%.0f");
            SliderFloat("IndentSpacing", &style.IndentSpacing, 0.0f, 30.0f, "%.0f");
            SliderFloat("ScrollbarSize", &style.ScrollbarSize, 1.0f, 20.0f, "%.0f");
            SliderFloat("GrabMinSize", &style.GrabMinSize, 1.0f, 20.0f, "%.0f");
            Text("Borders");
            SliderFloat("WindowBorderSize", &style.WindowBorderSize, 0.0f, 1.0f, "%.0f");
            SliderFloat("ChildBorderSize", &style.ChildBorderSize, 0.0f, 1.0f, "%.0f");
            SliderFloat("PopupBorderSize", &style.PopupBorderSize, 0.0f, 1.0f, "%.0f");
            SliderFloat("FrameBorderSize", &style.FrameBorderSize, 0.0f, 1.0f, "%.0f");
            SliderFloat("TabBorderSize", &style.TabBorderSize, 0.0f, 1.0f, "%.0f");
            Text("Rounding");
            SliderFloat("WindowRounding", &style.WindowRounding, 0.0f, 12.0f, "%.0f");
            SliderFloat("ChildRounding", &style.ChildRounding, 0.0f, 12.0f, "%.0f");
            SliderFloat("FrameRounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f");
            SliderFloat("PopupRounding", &style.PopupRounding, 0.0f, 12.0f, "%.0f");
            SliderFloat("ScrollbarRounding", &style.ScrollbarRounding, 0.0f, 12.0f, "%.0f");
            SliderFloat("GrabRounding", &style.GrabRounding, 0.0f, 12.0f, "%.0f");
            SliderFloat("LogSliderDeadzone", &style.LogSliderDeadzone, 0.0f, 12.0f, "%.0f");
            SliderFloat("TabRounding", &style.TabRounding, 0.0f, 12.0f, "%.0f");
            Text("Alignment");
            SliderFloat2("WindowTitleAlign", (float*)&style.WindowTitleAlign, 0.0f, 1.0f, "%.2f");
            int window_menu_button_position = style.WindowMenuButtonPosition + 1;
            if (Combo("WindowMenuButtonPosition", (int*)&window_menu_button_position, "None\0Left\0Right\0"))
                style.WindowMenuButtonPosition = window_menu_button_position - 1;
            Combo("ColorButtonPosition", (int*)&style.ColorButtonPosition, "Left\0Right\0");
            SliderFloat2("ButtonTextAlign", (float*)&style.ButtonTextAlign, 0.0f, 1.0f, "%.2f");
            SameLine(); HelpMarker(*this, "Alignment applies when a button is larger than its text content.");
            SliderFloat2("SelectableTextAlign", (float*)&style.SelectableTextAlign, 0.0f, 1.0f, "%.2f");
            SameLine(); HelpMarker(*this, "Alignment applies when a selectable is larger than its text content.");
            Text("Safe Area Padding");
            SameLine(); HelpMarker(*this, "Adjust if you cannot see the edges of your screen (e.g. on a TV where scaling has not been configured).");
            SliderFloat2("DisplaySafeAreaPadding", (float*)&style.DisplaySafeAreaPadding, 0.0f, 30.0f, "%.0f");
            EndTabItem();
        }

        if (BeginTabItem("Colors"))
        {
            static int output_dest = 0;
            static bool output_only_modified = true;
            if (Button("Export"))
            {
                if (output_dest == 0)
                    LogToClipboard();
                else
                    LogToTTY();
                LogText("ImVec4* colors = GetStyle().Colors;" IM_NEWLINE);
                for (int i = 0; i < ImGuiCol_COUNT; i++)
                {
                    const ImVec4& col = style.Colors[i];
                    const char* name = GetStyleColorName(i);
                    if (!output_only_modified || memcmp(&col, &ref->Colors[i], sizeof(ImVec4)) != 0)
                        LogText("colors[ImGuiCol_%s]%*s= ImVec4(%.2ff, %.2ff, %.2ff, %.2ff);" IM_NEWLINE,
                            name, 23 - (int)strlen(name), "", col.x, col.y, col.z, col.w);
                }
                LogFinish();
            }
            SameLine(); SetNextItemWidth(120); Combo("##output_type", &output_dest, "To Clipboard\0To TTY\0");
            SameLine(); Checkbox("Only Modified Colors", &output_only_modified);

            static ImGuiTextFilter filter(*this);
            filter.Draw("Filter colors", GetFontSize() * 16);

            static ImGuiColorEditFlags alpha_flags = 0;
            if (RadioButton("Opaque", alpha_flags == ImGuiColorEditFlags_None))             { alpha_flags = ImGuiColorEditFlags_None; } SameLine();
            if (RadioButton("Alpha",  alpha_flags == ImGuiColorEditFlags_AlphaPreview))     { alpha_flags = ImGuiColorEditFlags_AlphaPreview; } SameLine();
            if (RadioButton("Both",   alpha_flags == ImGuiColorEditFlags_AlphaPreviewHalf)) { alpha_flags = ImGuiColorEditFlags_AlphaPreviewHalf; } SameLine();
            HelpMarker(*this,
                "In the color list:\n"
                "Left-click on color square to open color picker,\n"
                "Right-click to open edit options menu.");

            BeginChild("##colors", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NavFlattened);
            PushItemWidth(-160);
            for (int i = 0; i < ImGuiCol_COUNT; i++)
            {
                const char* name = GetStyleColorName(i);
                if (!filter.PassFilter(name))
                    continue;
                PushID(i);
                ColorEdit4("##color", (float*)&style.Colors[i], ImGuiColorEditFlags_AlphaBar | alpha_flags);
                if (memcmp(&style.Colors[i], &ref->Colors[i], sizeof(ImVec4)) != 0)
                {
                    // Tips: in a real user application, you may want to merge and use an icon font into the main font,
                    // so instead of "Save"/"Revert" you'd use icons!
                    // Read the FAQ and docs/FONTS.md about using icon fonts. It's really easy and super convenient!
                    SameLine(0.0f, style.ItemInnerSpacing.x); if (Button("Save")) { ref->Colors[i] = style.Colors[i]; }
                    SameLine(0.0f, style.ItemInnerSpacing.x); if (Button("Revert")) { style.Colors[i] = ref->Colors[i]; }
                }
                SameLine(0.0f, style.ItemInnerSpacing.x);
                TextUnformatted(name);
                PopID();
            }
            PopItemWidth();
            EndChild();

            EndTabItem();
        }

        if (BeginTabItem("Fonts"))
        {
            ImGuiIO& io = GetIO();
            ImFontAtlas* atlas = io.Fonts;
            HelpMarker(*this, "Read FAQ and docs/FONTS.md for details on font loading.");
            PushItemWidth(120);
            for (int i = 0; i < atlas->Fonts.Size; i++)
            {
                ImFont* font = atlas->Fonts[i];
                PushID(font);
                NodeFont(*this, font);
                PopID();
            }
            if (TreeNode("Atlas texture", "Atlas texture (%dx%d pixels)", atlas->TexWidth, atlas->TexHeight))
            {
                ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
                Image(atlas->TexID, ImVec2((float)atlas->TexWidth, (float)atlas->TexHeight), ImVec2(0, 0), ImVec2(1, 1), tint_col, border_col);
                TreePop();
            }

            // Post-baking font scaling. Note that this is NOT the nice way of scaling fonts, read below.
            // (we enforce hard clamping manually as by default DragFloat/SliderFloat allows CTRL+Click text to get out of bounds).
            const float MIN_SCALE = 0.3f;
            const float MAX_SCALE = 2.0f;
            HelpMarker(*this,
                "Those are old settings provided for convenience.\n"
                "However, the _correct_ way of scaling your UI is currently to reload your font at the designed size, "
                "rebuild the font atlas, and call style.ScaleAllSizes() on a reference ImGuiStyle structure.\n"
                "Using those settings here will give you poor quality results.");
            static float window_scale = 1.0f;
            if (DragFloat("window scale", &window_scale, 0.005f, MIN_SCALE, MAX_SCALE, "%.2f", ImGuiSliderFlags_AlwaysClamp)) // Scale only this window
                SetWindowFontScale(window_scale);
            DragFloat("global scale", &io.FontGlobalScale, 0.005f, MIN_SCALE, MAX_SCALE, "%.2f", ImGuiSliderFlags_AlwaysClamp); // Scale everything
            PopItemWidth();

            EndTabItem();
        }

        if (BeginTabItem("Rendering"))
        {
            Checkbox("Anti-aliased lines", &style.AntiAliasedLines);
            SameLine();
            HelpMarker(*this, "When disabling anti-aliasing lines, you'll probably want to disable borders in your style as well.");

            Checkbox("Anti-aliased lines use texture", &style.AntiAliasedLinesUseTex);
            SameLine();
            HelpMarker(*this, "Faster lines using texture data. Require backend to render with bilinear filtering (not point/nearest filtering).");

            Checkbox("Anti-aliased fill", &style.AntiAliasedFill);
            PushItemWidth(100);
            DragFloat("Curve Tessellation Tolerance", &style.CurveTessellationTol, 0.02f, 0.10f, 10.0f, "%.2f");
            if (style.CurveTessellationTol < 0.10f) style.CurveTessellationTol = 0.10f;

            // When editing the "Circle Segment Max Error" value, draw a preview of its effect on auto-tessellated circles.
            DragFloat("Circle Tessellation Max Error", &style.CircleTessellationMaxError , 0.005f, 0.10f, 5.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            if (IsItemActive())
            {
                SetNextWindowPos(GetCursorScreenPos());
                BeginTooltip();
                TextUnformatted("(R = radius, N = number of segments)");
                Spacing();
                ImDrawList* draw_list = GetWindowDrawList();
                const float min_widget_width = CalcTextSize("N: MMM\nR: MMM").x;
                for (int n = 0; n < 8; n++)
                {
                    const float RAD_MIN = 5.0f;
                    const float RAD_MAX = 70.0f;
                    const float rad = RAD_MIN + (RAD_MAX - RAD_MIN) * (float)n / (8.0f - 1.0f);

                    BeginGroup();

                    Text("R: %.f\nN: %d", rad, draw_list->_CalcCircleAutoSegmentCount(rad));

                    const float canvas_width = IM_MAX(min_widget_width, rad * 2.0f);
                    const float offset_x     = floorf(canvas_width * 0.5f);
                    const float offset_y     = floorf(RAD_MAX);

                    const ImVec2 p1 = GetCursorScreenPos();
                    draw_list->AddCircle(ImVec2(p1.x + offset_x, p1.y + offset_y), rad, GetColorU32(ImGuiCol_Text));
                    Dummy(ImVec2(canvas_width, RAD_MAX * 2));

                    /*
                    const ImVec2 p2 = GetCursorScreenPos();
                    draw_list->AddCircleFilled(ImVec2(p2.x + offset_x, p2.y + offset_y), rad, GetColorU32(ImGuiCol_Text));
                    Dummy(ImVec2(canvas_width, RAD_MAX * 2));
                    */

                    EndGroup();
                    SameLine();
                }
                EndTooltip();
            }
            SameLine();
            HelpMarker(*this, "When drawing circle primitives with \"num_segments == 0\" tesselation will be calculated automatically.");

            DragFloat("Global Alpha", &style.Alpha, 0.005f, 0.20f, 1.0f, "%.2f"); // Not exposing zero here so user doesn't "lose" the UI (zero alpha clips all widgets). But application code could have a toggle to switch between zero and non-zero.
            PopItemWidth();

            EndTabItem();
        }

        EndTabBar();
    }

    PopItemWidth();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Main Menu Bar / ShowExampleAppMainMenuBar()
//-----------------------------------------------------------------------------
// - ShowExampleAppMainMenuBar()
// - ShowExampleMenuFile()
//-----------------------------------------------------------------------------

// Demonstrate creating a "main" fullscreen menu bar and populating it.
// Note the difference between BeginMainMenuBar() and BeginMenuBar():
// - BeginMenuBar() = menu-bar inside current window (which needs the ImGuiWindowFlags_MenuBar flag!)
// - BeginMainMenuBar() = helper to create menu-bar-sized window at the top of the main viewport + call BeginMenuBar() into it.
static void ShowExampleAppMainMenuBar(ImGui& imgui_)
{
    if (imgui_.BeginMainMenuBar())
    {
        if (imgui_.BeginMenu("File"))
        {
            ShowExampleMenuFile(imgui_);
            imgui_.EndMenu();
        }
        if (imgui_.BeginMenu("Edit"))
        {
            if (imgui_.MenuItem("Undo", "CTRL+Z")) {}
            if (imgui_.MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
            imgui_.Separator();
            if (imgui_.MenuItem("Cut", "CTRL+X")) {}
            if (imgui_.MenuItem("Copy", "CTRL+C")) {}
            if (imgui_.MenuItem("Paste", "CTRL+V")) {}
            imgui_.EndMenu();
        }
        imgui_.EndMainMenuBar();
    }
}

// Note that shortcuts are currently provided for display only
// (future version will add explicit flags to BeginMenu() to request processing shortcuts)
static void ShowExampleMenuFile(ImGui& imgui_)
{
    imgui_.MenuItem("(demo menu)", NULL, false, false);
    if (imgui_.MenuItem("New")) {}
    if (imgui_.MenuItem("Open", "Ctrl+O")) {}
    if (imgui_.BeginMenu("Open Recent"))
    {
        imgui_.MenuItem("fish_hat.c");
        imgui_.MenuItem("fish_hat.inl");
        imgui_.MenuItem("fish_hat.h");
        if (imgui_.BeginMenu("More.."))
        {
            imgui_.MenuItem("Hello");
            imgui_.MenuItem("Sailor");
            if (imgui_.BeginMenu("Recurse.."))
            {
                ShowExampleMenuFile(imgui_);
                imgui_.EndMenu();
            }
            imgui_.EndMenu();
        }
        imgui_.EndMenu();
    }
    if (imgui_.MenuItem("Save", "Ctrl+S")) {}
    if (imgui_.MenuItem("Save As..")) {}

    imgui_.Separator();
    if (imgui_.BeginMenu("Options"))
    {
        static bool enabled = true;
        imgui_.MenuItem("Enabled", "", &enabled);
        imgui_.BeginChild("child", ImVec2(0, 60), true);
        for (int i = 0; i < 10; i++)
            imgui_.Text("Scrolling Text %d", i);
        imgui_.EndChild();
        static float f = 0.5f;
        static int n = 0;
        imgui_.SliderFloat("Value", &f, 0.0f, 1.0f);
        imgui_.InputFloat("Input", &f, 0.1f);
        imgui_.Combo("Combo", &n, "Yes\0No\0Maybe\0\0");
        imgui_.EndMenu();
    }

    if (imgui_.BeginMenu("Colors"))
    {
        float sz = imgui_.GetTextLineHeight();
        for (int i = 0; i < ImGuiCol_COUNT; i++)
        {
            const char* name = imgui_.GetStyleColorName((ImGuiCol)i);
            ImVec2 p = imgui_.GetCursorScreenPos();
            imgui_.GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + sz, p.y + sz), imgui_.GetColorU32((ImGuiCol)i));
            imgui_.Dummy(ImVec2(sz, sz));
            imgui_.SameLine();
            imgui_.MenuItem(name);
        }
        imgui_.EndMenu();
    }

    // Here we demonstrate appending again to the "Options" menu (which we already created above)
    // Of course in this demo it is a little bit silly that this function calls BeginMenu("Options") twice.
    // In a real code-base using it would make senses to use this feature from very different code locations.
    if (imgui_.BeginMenu("Options")) // <-- Append!
    {
        static bool b = true;
        imgui_.Checkbox("SomeOption", &b);
        imgui_.EndMenu();
    }

    if (imgui_.BeginMenu("Disabled", false)) // Disabled
    {
        IM_ASSERT(0);
    }
    if (imgui_.MenuItem("Checked", NULL, true)) {}
    if (imgui_.MenuItem("Quit", "Alt+F4")) {}
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Debug Console / ShowExampleAppConsole()
//-----------------------------------------------------------------------------

// Demonstrate creating a simple console window, with scrolling, filtering, completion and history.
// For the console example, we are using a more C++ like approach of declaring a class to hold both data and functions.
struct ExampleAppConsole
{
    ImGui& imgui;

    char                  InputBuf[256];
    ImVector<char*>       Items;
    ImVector<const char*> Commands;
    ImVector<char*>       History;
    int                   HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
    ImGuiTextFilter       Filter;
    bool                  AutoScroll;
    bool                  ScrollToBottom;

    ExampleAppConsole(ImGui& imgui_)
        : imgui(imgui_)
        , Items(imgui_)
        , Commands(imgui_)
        , History(imgui_)
        , Filter(imgui_)
    {
        ClearLog();
        memset(InputBuf, 0, sizeof(InputBuf));
        HistoryPos = -1;

        // "CLASSIFY" is here to provide the test case where "C"+[tab] completes to "CL" and display multiple matches.
        Commands.push_back("HELP");
        Commands.push_back("HISTORY");
        Commands.push_back("CLEAR");
        Commands.push_back("CLASSIFY");
        AutoScroll = true;
        ScrollToBottom = false;
        AddLog("Welcome to Dear ImGui!");
    }
    ~ExampleAppConsole()
    {
        ClearLog();
        for (int i = 0; i < History.Size; i++)
            free(History[i]);
    }

    // Portable helpers
    static int   Stricmp(const char* s1, const char* s2)         { int d; while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; } return d; }
    static int   Strnicmp(const char* s1, const char* s2, int n) { int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; n--; } return d; }
    static char* Strdup(const char* s)                           { IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = malloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }
    static void  Strtrim(char* s)                                { char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0; }

    void    ClearLog()
    {
        for (int i = 0; i < Items.Size; i++)
            free(Items[i]);
        Items.clear();
    }

    void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
    {
        // FIXME-OPT
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
        buf[IM_ARRAYSIZE(buf)-1] = 0;
        va_end(args);
        Items.push_back(Strdup(buf));
    }

    void    Draw(const char* title, bool* p_open)
    {
        imgui.SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
        if (!imgui.Begin(title, p_open))
        {
            imgui.End();
            return;
        }

        // As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar.
        // So e.g. IsItemHovered() will return true when hovering the title bar.
        // Here we create a context menu only available from the title bar.
        if (imgui.BeginPopupContextItem())
        {
            if (imgui.MenuItem("Close Console"))
                *p_open = false;
            imgui.EndPopup();
        }

        imgui.TextWrapped(
            "This example implements a console with basic coloring, completion (TAB key) and history (Up/Down keys). A more elaborate "
            "implementation may want to store entries along with extra data such as timestamp, emitter, etc.");
        imgui.TextWrapped("Enter 'HELP' for help.");

        // TODO: display items starting from the bottom

        if (imgui.SmallButton("Add Debug Text"))  { AddLog("%d some text", Items.Size); AddLog("some more text"); AddLog("display very important message here!"); }
        imgui.SameLine();
        if (imgui.SmallButton("Add Debug Error")) { AddLog("[error] something went wrong"); }
        imgui.SameLine();
        if (imgui.SmallButton("Clear"))           { ClearLog(); }
        imgui.SameLine();
        bool copy_to_clipboard = imgui.SmallButton("Copy");
        //static float t = 0.0f; if (imgui.GetTime() - t > 0.02f) { t = imgui.GetTime(); AddLog("Spam %f", t); }

        imgui.Separator();

        // Options menu
        if (imgui.BeginPopup("Options"))
        {
            imgui.Checkbox("Auto-scroll", &AutoScroll);
            imgui.EndPopup();
        }

        // Options, Filter
        if (imgui.Button("Options"))
            imgui.OpenPopup("Options");
        imgui.SameLine();
        Filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
        imgui.Separator();

        // Reserve enough left-over height for 1 separator + 1 input text
        const float footer_height_to_reserve = imgui.GetStyle().ItemSpacing.y + imgui.GetFrameHeightWithSpacing();
        imgui.BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
        if (imgui.BeginPopupContextWindow())
        {
            if (imgui.Selectable("Clear")) ClearLog();
            imgui.EndPopup();
        }

        // Display every line as a separate entry so we can change their color or add custom widgets.
        // If you only want raw text you can use imgui.TextUnformatted(log.begin(), log.end());
        // NB- if you have thousands of entries this approach may be too inefficient and may require user-side clipping
        // to only process visible items. The clipper will automatically measure the height of your first item and then
        // "seek" to display only items in the visible area.
        // To use the clipper we can replace your standard loop:
        //      for (int i = 0; i < Items.Size; i++)
        //   With:
        //      ImGuiListClipper clipper;
        //      clipper.Begin(Items.Size);
        //      while (clipper.Step())
        //         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        // - That your items are evenly spaced (same height)
        // - That you have cheap random access to your elements (you can access them given their index,
        //   without processing all the ones before)
        // You cannot this code as-is if a filter is active because it breaks the 'cheap random-access' property.
        // We would need random-access on the post-filtered list.
        // A typical application wanting coarse clipping and filtering may want to pre-compute an array of indices
        // or offsets of items that passed the filtering test, recomputing this array when user changes the filter,
        // and appending newly elements as they are inserted. This is left as a task to the user until we can manage
        // to improve this example code!
        // If your items are of variable height:
        // - Split them into same height items would be simpler and facilitate random-seeking into your list.
        // - Consider using manual call to IsRectVisible() and skipping extraneous decoration from your items.
        imgui.PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
        if (copy_to_clipboard)
            imgui.LogToClipboard();
        for (int i = 0; i < Items.Size; i++)
        {
            const char* item = Items[i];
            if (!Filter.PassFilter(item))
                continue;

            // Normally you would store more information in your item than just a string.
            // (e.g. make Items[] an array of structure, store color/type etc.)
            ImVec4 color;
            bool has_color = false;
            if (strstr(item, "[error]"))          { color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); has_color = true; }
            else if (strncmp(item, "# ", 2) == 0) { color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true; }
            if (has_color)
                imgui.PushStyleColor(ImGuiCol_Text, color);
            imgui.TextUnformatted(item);
            if (has_color)
                imgui.PopStyleColor();
        }
        if (copy_to_clipboard)
            imgui.LogFinish();

        if (ScrollToBottom || (AutoScroll && imgui.GetScrollY() >= imgui.GetScrollMaxY()))
            imgui.SetScrollHereY(1.0f);
        ScrollToBottom = false;

        imgui.PopStyleVar();
        imgui.EndChild();
        imgui.Separator();

        // Command-line
        bool reclaim_focus = false;
        ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
        if (imgui.InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags, &TextEditCallbackStub, (void*)this))
        {
            char* s = InputBuf;
            Strtrim(s);
            if (s[0])
                ExecCommand(s);
            strcpy(s, "");
            reclaim_focus = true;
        }

        // Auto-focus on window apparition
        imgui.SetItemDefaultFocus();
        if (reclaim_focus)
            imgui.SetKeyboardFocusHere(-1); // Auto focus previous widget

        imgui.End();
    }

    void    ExecCommand(const char* command_line)
    {
        AddLog("# %s\n", command_line);

        // Insert into history. First find match and delete it so it can be pushed to the back.
        // This isn't trying to be smart or optimal.
        HistoryPos = -1;
        for (int i = History.Size - 1; i >= 0; i--)
            if (Stricmp(History[i], command_line) == 0)
            {
                free(History[i]);
                History.erase(History.begin() + i);
                break;
            }
        History.push_back(Strdup(command_line));

        // Process command
        if (Stricmp(command_line, "CLEAR") == 0)
        {
            ClearLog();
        }
        else if (Stricmp(command_line, "HELP") == 0)
        {
            AddLog("Commands:");
            for (int i = 0; i < Commands.Size; i++)
                AddLog("- %s", Commands[i]);
        }
        else if (Stricmp(command_line, "HISTORY") == 0)
        {
            int first = History.Size - 10;
            for (int i = first > 0 ? first : 0; i < History.Size; i++)
                AddLog("%3d: %s\n", i, History[i]);
        }
        else
        {
            AddLog("Unknown command: '%s'\n", command_line);
        }

        // On command input, we scroll to bottom even if AutoScroll==false
        ScrollToBottom = true;
    }

    // In C++11 you'd be better off using lambdas for this sort of forwarding callbacks
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data)
    {
        ExampleAppConsole* console = (ExampleAppConsole*)data->UserData;
        return console->TextEditCallback(data);
    }

    int     TextEditCallback(ImGuiInputTextCallbackData* data)
    {
        //AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
        switch (data->EventFlag)
        {
        case ImGuiInputTextFlags_CallbackCompletion:
            {
                // Example of TEXT COMPLETION

                // Locate beginning of current word
                const char* word_end = data->Buf + data->CursorPos;
                const char* word_start = word_end;
                while (word_start > data->Buf)
                {
                    const char c = word_start[-1];
                    if (c == ' ' || c == '\t' || c == ',' || c == ';')
                        break;
                    word_start--;
                }

                // Build a list of candidates
                ImVector<const char*> candidates(imgui);
                for (int i = 0; i < Commands.Size; i++)
                    if (Strnicmp(Commands[i], word_start, (int)(word_end - word_start)) == 0)
                        candidates.push_back(Commands[i]);

                if (candidates.Size == 0)
                {
                    // No match
                    AddLog("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
                }
                else if (candidates.Size == 1)
                {
                    // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
                    data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
                    data->InsertChars(data->CursorPos, candidates[0]);
                    data->InsertChars(data->CursorPos, " ");
                }
                else
                {
                    // Multiple matches. Complete as much as we can..
                    // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
                    int match_len = (int)(word_end - word_start);
                    for (;;)
                    {
                        int c = 0;
                        bool all_candidates_matches = true;
                        for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
                            if (i == 0)
                                c = toupper(candidates[i][match_len]);
                            else if (c == 0 || c != toupper(candidates[i][match_len]))
                                all_candidates_matches = false;
                        if (!all_candidates_matches)
                            break;
                        match_len++;
                    }

                    if (match_len > 0)
                    {
                        data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
                        data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
                    }

                    // List matches
                    AddLog("Possible matches:\n");
                    for (int i = 0; i < candidates.Size; i++)
                        AddLog("- %s\n", candidates[i]);
                }

                break;
            }
        case ImGuiInputTextFlags_CallbackHistory:
            {
                // Example of HISTORY
                const int prev_history_pos = HistoryPos;
                if (data->EventKey == ImGuiKey_UpArrow)
                {
                    if (HistoryPos == -1)
                        HistoryPos = History.Size - 1;
                    else if (HistoryPos > 0)
                        HistoryPos--;
                }
                else if (data->EventKey == ImGuiKey_DownArrow)
                {
                    if (HistoryPos != -1)
                        if (++HistoryPos >= History.Size)
                            HistoryPos = -1;
                }

                // A better implementation would preserve the data on the current input line along with cursor position.
                if (prev_history_pos != HistoryPos)
                {
                    const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
                    data->DeleteChars(0, data->BufTextLen);
                    data->InsertChars(0, history_str);
                }
            }
        }
        return 0;
    }
};

static void ShowExampleAppConsole(ImGui& imgui_, bool* p_open)
{
    static ExampleAppConsole console(imgui_);
    console.Draw("Example: Console", p_open);
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Debug Log / ShowExampleAppLog()
//-----------------------------------------------------------------------------

// Usage:
//  static ExampleAppLog my_log;
//  my_log.AddLog("Hello %d world\n", 123);
//  my_log.Draw("title");
struct ExampleAppLog
{
    ImGui& imgui;

    ImGuiTextBuffer     Buf;
    ImGuiTextFilter     Filter;
    ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
    bool                AutoScroll;  // Keep scrolling if already at the bottom.

    ExampleAppLog(ImGui& imgui_)
        : imgui(imgui_)
        , Buf(imgui_)
        , Filter(imgui_)
        , LineOffsets(imgui_)
    {
        AutoScroll = true;
        Clear();
    }

    void    Clear()
    {
        Buf.clear();
        LineOffsets.clear();
        LineOffsets.push_back(0);
    }

    void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
    {
        int old_size = Buf.size();
        va_list args;
        va_start(args, fmt);
        Buf.appendfv(fmt, args);
        va_end(args);
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size + 1);
    }

    void    Draw(const char* title, bool* p_open = NULL)
    {
        if (!imgui.Begin(title, p_open))
        {
            imgui.End();
            return;
        }

        // Options menu
        if (imgui.BeginPopup("Options"))
        {
            imgui.Checkbox("Auto-scroll", &AutoScroll);
            imgui.EndPopup();
        }

        // Main window
        if (imgui.Button("Options"))
            imgui.OpenPopup("Options");
        imgui.SameLine();
        bool clear = imgui.Button("Clear");
        imgui.SameLine();
        bool copy = imgui.Button("Copy");
        imgui.SameLine();
        Filter.Draw("Filter", -100.0f);

        imgui.Separator();
        imgui.BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        if (clear)
            Clear();
        if (copy)
            imgui.LogToClipboard();

        imgui.PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const char* buf = Buf.begin();
        const char* buf_end = Buf.end();
        if (Filter.IsActive())
        {
            // In this example we don't use the clipper when Filter is enabled.
            // This is because we don't have a random access on the result on our filter.
            // A real application processing logs with ten of thousands of entries may want to store the result of
            // search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
            for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
            {
                const char* line_start = buf + LineOffsets[line_no];
                const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                if (Filter.PassFilter(line_start, line_end))
                    imgui.TextUnformatted(line_start, line_end);
            }
        }
        else
        {
            // The simplest and easy way to display the entire buffer:
            //   imgui.TextUnformatted(buf_begin, buf_end);
            // And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward
            // to skip non-visible lines. Here we instead demonstrate using the clipper to only process lines that are
            // within the visible area.
            // If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them
            // on your side is recommended. Using ImGuiListClipper requires
            // - A) random access into your data
            // - B) items all being the  same height,
            // both of which we can handle since we an array pointing to the beginning of each line of text.
            // When using the filter (in the block of code above) we don't have random access into the data to display
            // anymore, which is why we don't use the clipper. Storing or skimming through the search result would make
            // it possible (and would be recommended if you want to search through tens of thousands of entries).
            ImGuiListClipper clipper(imgui);
            clipper.Begin(LineOffsets.Size);
            while (clipper.Step())
            {
                for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                {
                    const char* line_start = buf + LineOffsets[line_no];
                    const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                    imgui.TextUnformatted(line_start, line_end);
                }
            }
            clipper.End();
        }
        imgui.PopStyleVar();

        if (AutoScroll && imgui.GetScrollY() >= imgui.GetScrollMaxY())
            imgui.SetScrollHereY(1.0f);

        imgui.EndChild();
        imgui.End();
    }
};

// Demonstrate creating a simple log window with basic filtering.
static void ShowExampleAppLog(ImGui& imgui_, bool* p_open)
{
    static ExampleAppLog log(imgui_);

    // For the demo: add a debug button _BEFORE_ the normal log window contents
    // We take advantage of a rarely used feature: multiple calls to Begin()/End() are appending to the _same_ window.
    // Most of the contents of the window will be added by the log.Draw() call.
    imgui_.SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    imgui_.Begin("Example: Log", p_open);
    if (imgui_.SmallButton("[Debug] Add 5 entries"))
    {
        static int counter = 0;
        const char* categories[3] = { "info", "warn", "error" };
        const char* words[] = { "Bumfuzzled", "Cattywampus", "Snickersnee", "Abibliophobia", "Absquatulate", "Nincompoop", "Pauciloquent" };
        for (int n = 0; n < 5; n++)
        {
            const char* category = categories[counter % IM_ARRAYSIZE(categories)];
            const char* word = words[counter % IM_ARRAYSIZE(words)];
            log.AddLog("[%05d] [%s] Hello, current time is %.1f, here's a word: '%s'\n",
                imgui_.GetFrameCount(), category, imgui_.GetTime(), word);
            counter++;
        }
    }
    imgui_.End();

    // Actually call in the regular Log helper (which will Begin() into the same window as we just did)
    log.Draw("Example: Log", p_open);
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Simple Layout / ShowExampleAppLayout()
//-----------------------------------------------------------------------------

// Demonstrate create a window with multiple child windows.
static void ShowExampleAppLayout(ImGui& imgui_, bool* p_open)
{
    imgui_.SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
    if (imgui_.Begin("Example: Simple layout", p_open, ImGuiWindowFlags_MenuBar))
    {
        if (imgui_.BeginMenuBar())
        {
            if (imgui_.BeginMenu("File"))
            {
                if (imgui_.MenuItem("Close")) *p_open = false;
                imgui_.EndMenu();
            }
            imgui_.EndMenuBar();
        }

        // Left
        static int selected = 0;
        {
            imgui_.BeginChild("left pane", ImVec2(150, 0), true);
            for (int i = 0; i < 100; i++)
            {
                char label[128];
                sprintf(label, "MyObject %d", i);
                if (imgui_.Selectable(label, selected == i))
                    selected = i;
            }
            imgui_.EndChild();
        }
        imgui_.SameLine();

        // Right
        {
            imgui_.BeginGroup();
            imgui_.BeginChild("item view", ImVec2(0, -imgui_.GetFrameHeightWithSpacing())); // Leave room for 1 line below us
            imgui_.Text("MyObject: %d", selected);
            imgui_.Separator();
            if (imgui_.BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
            {
                if (imgui_.BeginTabItem("Description"))
                {
                    imgui_.TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. ");
                    imgui_.EndTabItem();
                }
                if (imgui_.BeginTabItem("Details"))
                {
                    imgui_.Text("ID: 0123456789");
                    imgui_.EndTabItem();
                }
                imgui_.EndTabBar();
            }
            imgui_.EndChild();
            if (imgui_.Button("Revert")) {}
            imgui_.SameLine();
            if (imgui_.Button("Save")) {}
            imgui_.EndGroup();
        }
    }
    imgui_.End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Property Editor / ShowExampleAppPropertyEditor()
//-----------------------------------------------------------------------------

static void ShowPlaceholderObject(ImGui& imgui_, const char* prefix, int uid)
{
    // Use object uid as identifier. Most commonly you could also use the object pointer as a base ID.
    imgui_.PushID(uid);

    // Text and Tree nodes are less high than framed widgets, using AlignTextToFramePadding() we add vertical spacing to make the tree lines equal high.
    imgui_.TableNextRow();
    imgui_.TableSetColumnIndex(0);
    imgui_.AlignTextToFramePadding();
    bool node_open = imgui_.TreeNode("Object", "%s_%u", prefix, uid);
    imgui_.TableSetColumnIndex(1);
    imgui_.Text("my sailor is rich");

    if (node_open)
    {
        static float placeholder_members[8] = { 0.0f, 0.0f, 1.0f, 3.1416f, 100.0f, 999.0f };
        for (int i = 0; i < 8; i++)
        {
            imgui_.PushID(i); // Use field index as identifier.
            if (i < 2)
            {
                ShowPlaceholderObject(imgui_, "Child", 424242);
            }
            else
            {
                // Here we use a TreeNode to highlight on hover (we could use e.g. Selectable as well)
                imgui_.TableNextRow();
                imgui_.TableSetColumnIndex(0);
                imgui_.AlignTextToFramePadding();
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet;
                imgui_.TreeNodeEx("Field", flags, "Field_%d", i);

                imgui_.TableSetColumnIndex(1);
                imgui_.SetNextItemWidth(-FLT_MIN);
                if (i >= 5)
                    imgui_.InputFloat("##value", &placeholder_members[i], 1.0f);
                else
                    imgui_.DragFloat("##value", &placeholder_members[i], 0.01f);
                imgui_.NextColumn();
            }
            imgui_.PopID();
        }
        imgui_.TreePop();
    }
    imgui_.PopID();
}

// Demonstrate create a simple property editor.
static void ShowExampleAppPropertyEditor(ImGui& imgui_, bool* p_open)
{
    imgui_.SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
    if (!imgui_.Begin("Example: Property editor", p_open))
    {
        imgui_.End();
        return;
    }

    HelpMarker(imgui_,
        "This example shows how you may implement a property editor using two columns.\n"
        "All objects/fields data are dummies here.\n"
        "Remember that in many simple cases, you can use imgui_.SameLine(xxx) to position\n"
        "your cursor horizontally instead of using the Columns() API.");

    imgui_.PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
    if (imgui_.BeginTable("split", 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable))
    {
        // Iterate placeholder objects (all the same data)
        for (int obj_i = 0; obj_i < 4; obj_i++)
        {
            ShowPlaceholderObject(imgui_, "Object", obj_i);
            //imgui_.Separator();
        }
        imgui_.EndTable();
    }
    imgui_.PopStyleVar();
    imgui_.End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Long Text / ShowExampleAppLongText()
//-----------------------------------------------------------------------------

// Demonstrate/test rendering huge amount of text, and the incidence of clipping.
static void ShowExampleAppLongText(ImGui& imgui_, bool* p_open)
{
    imgui_.SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    if (!imgui_.Begin("Example: Long text display", p_open))
    {
        imgui_.End();
        return;
    }

    static int test_type = 0;
    static ImGuiTextBuffer log(imgui_);
    static int lines = 0;
    imgui_.Text("Printing unusually long amount of text.");
    imgui_.Combo("Test type", &test_type,
        "Single call to TextUnformatted()\0"
        "Multiple calls to Text(), clipped\0"
        "Multiple calls to Text(), not clipped (slow)\0");
    imgui_.Text("Buffer contents: %d lines, %d bytes", lines, log.size());
    if (imgui_.Button("Clear")) { log.clear(); lines = 0; }
    imgui_.SameLine();
    if (imgui_.Button("Add 1000 lines"))
    {
        for (int i = 0; i < 1000; i++)
            log.appendf("%i The quick brown fox jumps over the lazy dog\n", lines + i);
        lines += 1000;
    }
    imgui_.BeginChild("Log");
    switch (test_type)
    {
    case 0:
        // Single call to TextUnformatted() with a big buffer
        imgui_.TextUnformatted(log.begin(), log.end());
        break;
    case 1:
        {
            // Multiple calls to Text(), manually coarsely clipped - demonstrate how to use the ImGuiListClipper helper.
            imgui_.PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            ImGuiListClipper clipper(imgui_);
            clipper.Begin(lines);
            while (clipper.Step())
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                    imgui_.Text("%i The quick brown fox jumps over the lazy dog", i);
            imgui_.PopStyleVar();
            break;
        }
    case 2:
        // Multiple calls to Text(), not clipped (slow)
        imgui_.PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        for (int i = 0; i < lines; i++)
            imgui_.Text("%i The quick brown fox jumps over the lazy dog", i);
        imgui_.PopStyleVar();
        break;
    }
    imgui_.EndChild();
    imgui_.End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Auto Resize / ShowExampleAppAutoResize()
//-----------------------------------------------------------------------------

// Demonstrate creating a window which gets auto-resized according to its content.
static void ShowExampleAppAutoResize(ImGui& imgui_, bool* p_open)
{
    if (!imgui_.Begin("Example: Auto-resizing window", p_open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        imgui_.End();
        return;
    }

    static int lines = 10;
    imgui_.TextUnformatted(
        "Window will resize every-frame to the size of its content.\n"
        "Note that you probably don't want to query the window size to\n"
        "output your content because that would create a feedback loop.");
    imgui_.SliderInt("Number of lines", &lines, 1, 20);
    for (int i = 0; i < lines; i++)
        imgui_.Text("%*sThis is line %d", i * 4, "", i); // Pad with space to extend size horizontally
    imgui_.End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Constrained Resize / ShowExampleAppConstrainedResize()
//-----------------------------------------------------------------------------

// Demonstrate creating a window with custom resize constraints.
static void ShowExampleAppConstrainedResize(ImGui& imgui_, bool* p_open)
{
    struct CustomConstraints
    {
        // Helper functions to demonstrate programmatic constraints
        static void Square(ImGuiSizeCallbackData* data) { data->DesiredSize.x = data->DesiredSize.y = IM_MAX(data->DesiredSize.x, data->DesiredSize.y); }
        static void Step(ImGuiSizeCallbackData* data)   { float step = (float)(int)(intptr_t)data->UserData; data->DesiredSize = ImVec2((int)(data->DesiredSize.x / step + 0.5f) * step, (int)(data->DesiredSize.y / step + 0.5f) * step); }
    };

    const char* test_desc[] =
    {
        "Resize vertical only",
        "Resize horizontal only",
        "Width > 100, Height > 100",
        "Width 400-500",
        "Height 400-500",
        "Custom: Always Square",
        "Custom: Fixed Steps (100)",
    };

    static bool auto_resize = false;
    static int type = 0;
    static int display_lines = 10;
    if (type == 0) imgui_.SetNextWindowSizeConstraints(ImVec2(-1, 0),    ImVec2(-1, FLT_MAX));      // Vertical only
    if (type == 1) imgui_.SetNextWindowSizeConstraints(ImVec2(0, -1),    ImVec2(FLT_MAX, -1));      // Horizontal only
    if (type == 2) imgui_.SetNextWindowSizeConstraints(ImVec2(100, 100), ImVec2(FLT_MAX, FLT_MAX)); // Width > 100, Height > 100
    if (type == 3) imgui_.SetNextWindowSizeConstraints(ImVec2(400, -1),  ImVec2(500, -1));          // Width 400-500
    if (type == 4) imgui_.SetNextWindowSizeConstraints(ImVec2(-1, 400),  ImVec2(-1, 500));          // Height 400-500
    if (type == 5) imgui_.SetNextWindowSizeConstraints(ImVec2(0, 0),     ImVec2(FLT_MAX, FLT_MAX), CustomConstraints::Square);                     // Always Square
    if (type == 6) imgui_.SetNextWindowSizeConstraints(ImVec2(0, 0),     ImVec2(FLT_MAX, FLT_MAX), CustomConstraints::Step, (void*)(intptr_t)100); // Fixed Step

    ImGuiWindowFlags flags = auto_resize ? ImGuiWindowFlags_AlwaysAutoResize : 0;
    if (imgui_.Begin("Example: Constrained Resize", p_open, flags))
    {
        if (imgui_.Button("200x200")) { imgui_.SetWindowSize(ImVec2(200, 200)); } imgui_.SameLine();
        if (imgui_.Button("500x500")) { imgui_.SetWindowSize(ImVec2(500, 500)); } imgui_.SameLine();
        if (imgui_.Button("800x200")) { imgui_.SetWindowSize(ImVec2(800, 200)); }
        imgui_.SetNextItemWidth(200);
        imgui_.Combo("Constraint", &type, test_desc, IM_ARRAYSIZE(test_desc));
        imgui_.SetNextItemWidth(200);
        imgui_.DragInt("Lines", &display_lines, 0.2f, 1, 100);
        imgui_.Checkbox("Auto-resize", &auto_resize);
        for (int i = 0; i < display_lines; i++)
            imgui_.Text("%*sHello, sailor! Making this line long enough for the example.", i * 4, "");
    }
    imgui_.End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Simple overlay / ShowExampleAppSimpleOverlay()
//-----------------------------------------------------------------------------

// Demonstrate creating a simple static window with no decoration
// + a context-menu to choose which corner of the screen to use.
static void ShowExampleAppSimpleOverlay(ImGui& imgui_, bool* p_open)
{
    const float PAD = 10.0f;
    static int corner = 0;
    ImGuiIO& io = imgui_.GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    if (corner != -1)
    {
        const ImGuiViewport* viewport = imgui_.GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
        window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
        window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
        imgui_.SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        window_flags |= ImGuiWindowFlags_NoMove;
    }
    imgui_.SetNextWindowBgAlpha(0.35f); // Transparent background
    if (imgui_.Begin("Example: Simple overlay", p_open, window_flags))
    {
        imgui_.Text("Simple overlay\n" "in the corner of the screen.\n" "(right-click to change position)");
        imgui_.Separator();
        if (imgui_.IsMousePosValid())
            imgui_.Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
        else
            imgui_.Text("Mouse Position: <invalid>");
        if (imgui_.BeginPopupContextWindow())
        {
            if (imgui_.MenuItem("Custom",       NULL, corner == -1)) corner = -1;
            if (imgui_.MenuItem("Top-left",     NULL, corner == 0)) corner = 0;
            if (imgui_.MenuItem("Top-right",    NULL, corner == 1)) corner = 1;
            if (imgui_.MenuItem("Bottom-left",  NULL, corner == 2)) corner = 2;
            if (imgui_.MenuItem("Bottom-right", NULL, corner == 3)) corner = 3;
            if (p_open && imgui_.MenuItem("Close")) *p_open = false;
            imgui_.EndPopup();
        }
    }
    imgui_.End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Fullscreen window / ShowExampleAppFullscreen()
//-----------------------------------------------------------------------------

// Demonstrate creating a window covering the entire screen/viewport
static void ShowExampleAppFullscreen(ImGui& imgui_, bool* p_open)
{
    static bool use_work_area = true;
    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;

    // We demonstrate using the full viewport area or the work area (without menu-bars, task-bars etc.)
    // Based on your use case you may want one of the other.
    const ImGuiViewport* viewport = imgui_.GetMainViewport();
    imgui_.SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
    imgui_.SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);

    if (imgui_.Begin("Example: Fullscreen window", p_open, flags))
    {
        imgui_.Checkbox("Use work area instead of main area", &use_work_area);
        imgui_.SameLine();
        HelpMarker(imgui_, "Main Area = entire viewport,\nWork Area = entire viewport minus sections used by the main menu bars, task bars etc.\n\nEnable the main-menu bar in Examples menu to see the difference.");

        imgui_.CheckboxFlags("ImGuiWindowFlags_NoBackground", &flags, ImGuiWindowFlags_NoBackground);
        imgui_.CheckboxFlags("ImGuiWindowFlags_NoDecoration", &flags, ImGuiWindowFlags_NoDecoration);
        imgui_.Indent();
        imgui_.CheckboxFlags("ImGuiWindowFlags_NoTitleBar", &flags, ImGuiWindowFlags_NoTitleBar);
        imgui_.CheckboxFlags("ImGuiWindowFlags_NoCollapse", &flags, ImGuiWindowFlags_NoCollapse);
        imgui_.CheckboxFlags("ImGuiWindowFlags_NoScrollbar", &flags, ImGuiWindowFlags_NoScrollbar);
        imgui_.Unindent();

        if (p_open && imgui_.Button("Close this window"))
            *p_open = false;
    }
    imgui_.End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Manipulating Window Titles / ShowExampleAppWindowTitles()
//-----------------------------------------------------------------------------

// Demonstrate using "##" and "###" in identifiers to manipulate ID generation.
// This apply to all regular items as well.
// Read FAQ section "How can I have multiple widgets with the same label?" for details.
static void ShowExampleAppWindowTitles(ImGui& imgui_, bool*)
{
    const ImGuiViewport* viewport = imgui_.GetMainViewport();
    const ImVec2 base_pos = viewport->Pos;

    // By default, Windows are uniquely identified by their title.
    // You can use the "##" and "###" markers to manipulate the display/ID.

    // Using "##" to display same title but have unique identifier.
    imgui_.SetNextWindowPos(ImVec2(base_pos.x + 100, base_pos.y + 100), ImGuiCond_FirstUseEver);
    imgui_.Begin("Same title as another window##1");
    imgui_.Text("This is window 1.\nMy title is the same as window 2, but my identifier is unique.");
    imgui_.End();

    imgui_.SetNextWindowPos(ImVec2(base_pos.x + 100, base_pos.y + 200), ImGuiCond_FirstUseEver);
    imgui_.Begin("Same title as another window##2");
    imgui_.Text("This is window 2.\nMy title is the same as window 1, but my identifier is unique.");
    imgui_.End();

    // Using "###" to display a changing title but keep a static identifier "AnimatedTitle"
    char buf[128];
    sprintf(buf, "Animated title %c %d###AnimatedTitle", "|/-\\"[(int)(imgui_.GetTime() / 0.25f) & 3], imgui_.GetFrameCount());
    imgui_.SetNextWindowPos(ImVec2(base_pos.x + 100, base_pos.y + 300), ImGuiCond_FirstUseEver);
    imgui_.Begin(buf);
    imgui_.Text("This window has a changing title.");
    imgui_.End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Custom Rendering using ImDrawList API / ShowExampleAppCustomRendering()
//-----------------------------------------------------------------------------

// Demonstrate using the low-level ImDrawList to draw custom shapes.
static void ShowExampleAppCustomRendering(ImGui& imgui_, bool* p_open)
{
    if (!imgui_.Begin("Example: Custom rendering", p_open))
    {
        imgui_.End();
        return;
    }

    // Tip: If you do a lot of custom rendering, you probably want to use your own geometrical types and benefit of
    // overloaded operators, etc. Define IM_VEC2_CLASS_EXTRA in imconfig.h to create implicit conversions between your
    // types and ImVec2/ImVec4. Dear ImGui defines overloaded operators but they are internal to imgui.cpp and not
    // exposed outside (to avoid messing with your types) In this example we are not using the maths operators!

    if (imgui_.BeginTabBar("##TabBar"))
    {
        if (imgui_.BeginTabItem("Primitives"))
        {
            imgui_.PushItemWidth(-imgui_.GetFontSize() * 15);
            ImDrawList* draw_list = imgui_.GetWindowDrawList();

            // Draw gradients
            // (note that those are currently exacerbating our sRGB/Linear issues)
            // Calling imgui_.GetColorU32() multiplies the given colors by the current Style Alpha, but you may pass the IM_COL32() directly as well..
            imgui_.Text("Gradients");
            ImVec2 gradient_size = ImVec2(imgui_.CalcItemWidth(), imgui_.GetFrameHeight());
            {
                ImVec2 p0 = imgui_.GetCursorScreenPos();
                ImVec2 p1 = ImVec2(p0.x + gradient_size.x, p0.y + gradient_size.y);
                ImU32 col_a = imgui_.GetColorU32(IM_COL32(0, 0, 0, 255));
                ImU32 col_b = imgui_.GetColorU32(IM_COL32(255, 255, 255, 255));
                draw_list->AddRectFilledMultiColor(p0, p1, col_a, col_b, col_b, col_a);
                imgui_.InvisibleButton("##gradient1", gradient_size);
            }
            {
                ImVec2 p0 = imgui_.GetCursorScreenPos();
                ImVec2 p1 = ImVec2(p0.x + gradient_size.x, p0.y + gradient_size.y);
                ImU32 col_a = imgui_.GetColorU32(IM_COL32(0, 255, 0, 255));
                ImU32 col_b = imgui_.GetColorU32(IM_COL32(255, 0, 0, 255));
                draw_list->AddRectFilledMultiColor(p0, p1, col_a, col_b, col_b, col_a);
                imgui_.InvisibleButton("##gradient2", gradient_size);
            }

            // Draw a bunch of primitives
            imgui_.Text("All primitives");
            static float sz = 36.0f;
            static float thickness = 3.0f;
            static int ngon_sides = 6;
            static bool circle_segments_override = false;
            static int circle_segments_override_v = 12;
            static bool curve_segments_override = false;
            static int curve_segments_override_v = 8;
            static ImVec4 colf = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
            imgui_.DragFloat("Size", &sz, 0.2f, 2.0f, 100.0f, "%.0f");
            imgui_.DragFloat("Thickness", &thickness, 0.05f, 1.0f, 8.0f, "%.02f");
            imgui_.SliderInt("N-gon sides", &ngon_sides, 3, 12);
            imgui_.Checkbox("##circlesegmentoverride", &circle_segments_override);
            imgui_.SameLine(0.0f, imgui_.GetStyle().ItemInnerSpacing.x);
            circle_segments_override |= imgui_.SliderInt("Circle segments override", &circle_segments_override_v, 3, 40);
            imgui_.Checkbox("##curvessegmentoverride", &curve_segments_override);
            imgui_.SameLine(0.0f, imgui_.GetStyle().ItemInnerSpacing.x);
            curve_segments_override |= imgui_.SliderInt("Curves segments override", &curve_segments_override_v, 3, 40);
            imgui_.ColorEdit4("Color", &colf.x);

            const ImVec2 p = imgui_.GetCursorScreenPos();
            const ImU32 col = ImColor(colf);
            const float spacing = 10.0f;
            const ImDrawFlags corners_tl_br = ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomRight;
            const float rounding = sz / 5.0f;
            const int circle_segments = circle_segments_override ? circle_segments_override_v : 0;
            const int curve_segments = curve_segments_override ? curve_segments_override_v : 0;
            float x = p.x + 4.0f;
            float y = p.y + 4.0f;
            for (int n = 0; n < 2; n++)
            {
                // First line uses a thickness of 1.0f, second line uses the configurable thickness
                float th = (n == 0) ? 1.0f : thickness;
                draw_list->AddNgon(ImVec2(x + sz*0.5f, y + sz*0.5f), sz*0.5f, col, ngon_sides, th);                 x += sz + spacing;  // N-gon
                draw_list->AddCircle(ImVec2(x + sz*0.5f, y + sz*0.5f), sz*0.5f, col, circle_segments, th);          x += sz + spacing;  // Circle
                draw_list->AddRect(ImVec2(x, y), ImVec2(x + sz, y + sz), col, 0.0f, ImDrawFlags_None, th);          x += sz + spacing;  // Square
                draw_list->AddRect(ImVec2(x, y), ImVec2(x + sz, y + sz), col, rounding, ImDrawFlags_None, th);      x += sz + spacing;  // Square with all rounded corners
                draw_list->AddRect(ImVec2(x, y), ImVec2(x + sz, y + sz), col, rounding, corners_tl_br, th);         x += sz + spacing;  // Square with two rounded corners
                draw_list->AddTriangle(ImVec2(x+sz*0.5f,y), ImVec2(x+sz, y+sz-0.5f), ImVec2(x, y+sz-0.5f), col, th);x += sz + spacing;  // Triangle
                //draw_list->AddTriangle(ImVec2(x+sz*0.2f,y), ImVec2(x, y+sz-0.5f), ImVec2(x+sz*0.4f, y+sz-0.5f), col, th);x+= sz*0.4f + spacing; // Thin triangle
                draw_list->AddLine(ImVec2(x, y), ImVec2(x + sz, y), col, th);                                       x += sz + spacing;  // Horizontal line (note: drawing a filled rectangle will be faster!)
                draw_list->AddLine(ImVec2(x, y), ImVec2(x, y + sz), col, th);                                       x += spacing;       // Vertical line (note: drawing a filled rectangle will be faster!)
                draw_list->AddLine(ImVec2(x, y), ImVec2(x + sz, y + sz), col, th);                                  x += sz + spacing;  // Diagonal line

                // Quadratic Bezier Curve (3 control points)
                ImVec2 cp3[3] = { ImVec2(x, y + sz * 0.6f), ImVec2(x + sz * 0.5f, y - sz * 0.4f), ImVec2(x + sz, y + sz) };
                draw_list->AddBezierQuadratic(cp3[0], cp3[1], cp3[2], col, th, curve_segments); x += sz + spacing;

                // Cubic Bezier Curve (4 control points)
                ImVec2 cp4[4] = { ImVec2(x, y), ImVec2(x + sz * 1.3f, y + sz * 0.3f), ImVec2(x + sz - sz * 1.3f, y + sz - sz * 0.3f), ImVec2(x + sz, y + sz) };
                draw_list->AddBezierCubic(cp4[0], cp4[1], cp4[2], cp4[3], col, th, curve_segments);

                x = p.x + 4;
                y += sz + spacing;
            }
            draw_list->AddNgonFilled(ImVec2(x + sz * 0.5f, y + sz * 0.5f), sz*0.5f, col, ngon_sides);               x += sz + spacing;  // N-gon
            draw_list->AddCircleFilled(ImVec2(x + sz*0.5f, y + sz*0.5f), sz*0.5f, col, circle_segments);            x += sz + spacing;  // Circle
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + sz, y + sz), col);                                    x += sz + spacing;  // Square
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + sz, y + sz), col, 10.0f);                             x += sz + spacing;  // Square with all rounded corners
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + sz, y + sz), col, 10.0f, corners_tl_br);              x += sz + spacing;  // Square with two rounded corners
            draw_list->AddTriangleFilled(ImVec2(x+sz*0.5f,y), ImVec2(x+sz, y+sz-0.5f), ImVec2(x, y+sz-0.5f), col);  x += sz + spacing;  // Triangle
            //draw_list->AddTriangleFilled(ImVec2(x+sz*0.2f,y), ImVec2(x, y+sz-0.5f), ImVec2(x+sz*0.4f, y+sz-0.5f), col); x += sz*0.4f + spacing; // Thin triangle
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + sz, y + thickness), col);                             x += sz + spacing;  // Horizontal line (faster than AddLine, but only handle integer thickness)
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + thickness, y + sz), col);                             x += spacing * 2.0f;// Vertical line (faster than AddLine, but only handle integer thickness)
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + 1, y + 1), col);                                      x += sz;            // Pixel (faster than AddLine)
            draw_list->AddRectFilledMultiColor(ImVec2(x, y), ImVec2(x + sz, y + sz), IM_COL32(0, 0, 0, 255), IM_COL32(255, 0, 0, 255), IM_COL32(255, 255, 0, 255), IM_COL32(0, 255, 0, 255));

            imgui_.Dummy(ImVec2((sz + spacing) * 10.2f, (sz + spacing) * 3.0f));
            imgui_.PopItemWidth();
            imgui_.EndTabItem();
        }

        if (imgui_.BeginTabItem("Canvas"))
        {
            static ImVector<ImVec2> points(imgui_);
            static ImVec2 scrolling(0.0f, 0.0f);
            static bool opt_enable_grid = true;
            static bool opt_enable_context_menu = true;
            static bool adding_line = false;

            imgui_.Checkbox("Enable grid", &opt_enable_grid);
            imgui_.Checkbox("Enable context menu", &opt_enable_context_menu);
            imgui_.Text("Mouse Left: drag to add lines,\nMouse Right: drag to scroll, click for context menu.");

            // Typically you would use a BeginChild()/EndChild() pair to benefit from a clipping region + own scrolling.
            // Here we demonstrate that this can be replaced by simple offsetting + custom drawing + PushClipRect/PopClipRect() calls.
            // To use a child window instead we could use, e.g:
            //      imgui_.PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));      // Disable padding
            //      imgui_.PushStyleColor(ImGuiCol_ChildBg, IM_COL32(50, 50, 50, 255));  // Set a background color
            //      imgui_.BeginChild("canvas", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_NoMove);
            //      imgui_.PopStyleColor();
            //      imgui_.PopStyleVar();
            //      [...]
            //      imgui_.EndChild();

            // Using InvisibleButton() as a convenience 1) it will advance the layout cursor and 2) allows us to use IsItemHovered()/IsItemActive()
            ImVec2 canvas_p0 = imgui_.GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
            ImVec2 canvas_sz = imgui_.GetContentRegionAvail();   // Resize canvas to what's available
            if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
            if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
            ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

            // Draw border and background color
            ImGuiIO& io = imgui_.GetIO();
            ImDrawList* draw_list = imgui_.GetWindowDrawList();
            draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
            draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));

            // This will catch our interactions
            imgui_.InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
            const bool is_hovered = imgui_.IsItemHovered(); // Hovered
            const bool is_active = imgui_.IsItemActive();   // Held
            const ImVec2 origin(canvas_p0.x + scrolling.x, canvas_p0.y + scrolling.y); // Lock scrolled origin
            const ImVec2 mouse_pos_in_canvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);

            // Add first and second point
            if (is_hovered && !adding_line && imgui_.IsMouseClicked(ImGuiMouseButton_Left))
            {
                points.push_back(mouse_pos_in_canvas);
                points.push_back(mouse_pos_in_canvas);
                adding_line = true;
            }
            if (adding_line)
            {
                points.back() = mouse_pos_in_canvas;
                if (!imgui_.IsMouseDown(ImGuiMouseButton_Left))
                    adding_line = false;
            }

            // Pan (we use a zero mouse threshold when there's no context menu)
            // You may decide to make that threshold dynamic based on whether the mouse is hovering something etc.
            const float mouse_threshold_for_pan = opt_enable_context_menu ? -1.0f : 0.0f;
            if (is_active && imgui_.IsMouseDragging(ImGuiMouseButton_Right, mouse_threshold_for_pan))
            {
                scrolling.x += io.MouseDelta.x;
                scrolling.y += io.MouseDelta.y;
            }

            // Context menu (under default mouse threshold)
            ImVec2 drag_delta = imgui_.GetMouseDragDelta(ImGuiMouseButton_Right);
            if (opt_enable_context_menu && imgui_.IsMouseReleased(ImGuiMouseButton_Right) && drag_delta.x == 0.0f && drag_delta.y == 0.0f)
                imgui_.OpenPopupOnItemClick("context");
            if (imgui_.BeginPopup("context"))
            {
                if (adding_line)
                    points.resize(points.size() - 2);
                adding_line = false;
                if (imgui_.MenuItem("Remove one", NULL, false, points.Size > 0)) { points.resize(points.size() - 2); }
                if (imgui_.MenuItem("Remove all", NULL, false, points.Size > 0)) { points.clear(); }
                imgui_.EndPopup();
            }

            // Draw grid + all lines in the canvas
            draw_list->PushClipRect(canvas_p0, canvas_p1, true);
            if (opt_enable_grid)
            {
                const float GRID_STEP = 64.0f;
                for (float x = fmodf(scrolling.x, GRID_STEP); x < canvas_sz.x; x += GRID_STEP)
                    draw_list->AddLine(ImVec2(canvas_p0.x + x, canvas_p0.y), ImVec2(canvas_p0.x + x, canvas_p1.y), IM_COL32(200, 200, 200, 40));
                for (float y = fmodf(scrolling.y, GRID_STEP); y < canvas_sz.y; y += GRID_STEP)
                    draw_list->AddLine(ImVec2(canvas_p0.x, canvas_p0.y + y), ImVec2(canvas_p1.x, canvas_p0.y + y), IM_COL32(200, 200, 200, 40));
            }
            for (int n = 0; n < points.Size; n += 2)
                draw_list->AddLine(ImVec2(origin.x + points[n].x, origin.y + points[n].y), ImVec2(origin.x + points[n + 1].x, origin.y + points[n + 1].y), IM_COL32(255, 255, 0, 255), 2.0f);
            draw_list->PopClipRect();

            imgui_.EndTabItem();
        }

        if (imgui_.BeginTabItem("BG/FG draw lists"))
        {
            static bool draw_bg = true;
            static bool draw_fg = true;
            imgui_.Checkbox("Draw in Background draw list", &draw_bg);
            imgui_.SameLine(); HelpMarker(imgui_, "The Background draw list will be rendered below every Dear ImGui windows.");
            imgui_.Checkbox("Draw in Foreground draw list", &draw_fg);
            imgui_.SameLine(); HelpMarker(imgui_, "The Foreground draw list will be rendered over every Dear ImGui windows.");
            ImVec2 window_pos = imgui_.GetWindowPos();
            ImVec2 window_size = imgui_.GetWindowSize();
            ImVec2 window_center = ImVec2(window_pos.x + window_size.x * 0.5f, window_pos.y + window_size.y * 0.5f);
            if (draw_bg)
                imgui_.GetBackgroundDrawList()->AddCircle(window_center, window_size.x * 0.6f, IM_COL32(255, 0, 0, 200), 0, 10 + 4);
            if (draw_fg)
                imgui_.GetForegroundDrawList()->AddCircle(window_center, window_size.y * 0.6f, IM_COL32(0, 255, 0, 200), 0, 10);
            imgui_.EndTabItem();
        }

        imgui_.EndTabBar();
    }

    imgui_.End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Documents Handling / ShowExampleAppDocuments()
//-----------------------------------------------------------------------------

// Simplified structure to mimic a Document model
struct MyDocument
{
    ImGui& imgui;

    const char* Name;       // Document title
    bool        Open;       // Set when open (we keep an array of all available documents to simplify demo code!)
    bool        OpenPrev;   // Copy of Open from last update.
    bool        Dirty;      // Set when the document has been modified
    bool        WantClose;  // Set when the document
    ImVec4      Color;      // An arbitrary variable associated to the document

    MyDocument(ImGui& imgui_, const char* name, bool open = true, const ImVec4& color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f))
        : imgui(imgui_)
    {
        Name = name;
        Open = OpenPrev = open;
        Dirty = false;
        WantClose = false;
        Color = color;
    }
    void DoOpen()       { Open = true; }
    void DoQueueClose() { WantClose = true; }
    void DoForceClose() { Open = false; Dirty = false; }
    void DoSave()       { Dirty = false; }

    // Display placeholder contents for the Document
    static void DisplayContents(MyDocument* doc)
    {
        doc->imgui.PushID(doc);
        doc->imgui.Text("Document \"%s\"", doc->Name);
        doc->imgui.PushStyleColor(ImGuiCol_Text, doc->Color);
        doc->imgui.TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.");
        doc->imgui.PopStyleColor();
        if (doc->imgui.Button("Modify", ImVec2(100, 0)))
            doc->Dirty = true;
        doc->imgui.SameLine();
        if (doc->imgui.Button("Save", ImVec2(100, 0)))
            doc->DoSave();
        doc->imgui.ColorEdit3("color", &doc->Color.x);  // Useful to test drag and drop and hold-dragged-to-open-tab behavior.
        doc->imgui.PopID();
    }

    // Display context menu for the Document
    static void DisplayContextMenu(MyDocument* doc)
    {
        if (!doc->imgui.BeginPopupContextItem())
            return;

        char buf[256];
        sprintf(buf, "Save %s", doc->Name);
        if (doc->imgui.MenuItem(buf, "CTRL+S", false, doc->Open))
            doc->DoSave();
        if (doc->imgui.MenuItem("Close", "CTRL+W", false, doc->Open))
            doc->DoQueueClose();
        doc->imgui.EndPopup();
    }
};

struct ExampleAppDocuments
{
    ImVector<MyDocument> Documents;

    ExampleAppDocuments(ImGui& imgui_)
        : Documents(imgui_)
    {
        Documents.push_back(MyDocument(imgui_, "Lettuce",             true,  ImVec4(0.4f, 0.8f, 0.4f, 1.0f)));
        Documents.push_back(MyDocument(imgui_, "Eggplant",            true,  ImVec4(0.8f, 0.5f, 1.0f, 1.0f)));
        Documents.push_back(MyDocument(imgui_, "Carrot",              true,  ImVec4(1.0f, 0.8f, 0.5f, 1.0f)));
        Documents.push_back(MyDocument(imgui_, "Tomato",              false, ImVec4(1.0f, 0.3f, 0.4f, 1.0f)));
        Documents.push_back(MyDocument(imgui_, "A Rather Long Title", false));
        Documents.push_back(MyDocument(imgui_, "Some Document",       false));
    }
};

// [Optional] Notify the system of Tabs/Windows closure that happened outside the regular tab interface.
// If a tab has been closed programmatically (aka closed from another source such as the Checkbox() in the demo,
// as opposed to clicking on the regular tab closing button) and stops being submitted, it will take a frame for
// the tab bar to notice its absence. During this frame there will be a gap in the tab bar, and if the tab that has
// disappeared was the selected one, the tab bar will report no selected tab during the frame. This will effectively
// give the impression of a flicker for one frame.
// We call SetTabItemClosed() to manually notify the Tab Bar or Docking system of removed tabs to avoid this glitch.
// Note that this completely optional, and only affect tab bars with the ImGuiTabBarFlags_Reorderable flag.
static void NotifyOfDocumentsClosedElsewhere(ImGui& imgui_, ExampleAppDocuments& app)
{
    for (int doc_n = 0; doc_n < app.Documents.Size; doc_n++)
    {
        MyDocument* doc = &app.Documents[doc_n];
        if (!doc->Open && doc->OpenPrev)
            imgui_.SetTabItemClosed(doc->Name);
        doc->OpenPrev = doc->Open;
    }
}

void ShowExampleAppDocuments(ImGui& imgui_, bool* p_open)
{
    static ExampleAppDocuments app(imgui_);

    // Options
    static bool opt_reorderable = true;
    static ImGuiTabBarFlags opt_fitting_flags = ImGuiTabBarFlags_FittingPolicyDefault_;

    bool window_contents_visible = imgui_.Begin("Example: Documents", p_open, ImGuiWindowFlags_MenuBar);
    if (!window_contents_visible)
    {
        imgui_.End();
        return;
    }

    // Menu
    if (imgui_.BeginMenuBar())
    {
        if (imgui_.BeginMenu("File"))
        {
            int open_count = 0;
            for (int doc_n = 0; doc_n < app.Documents.Size; doc_n++)
                open_count += app.Documents[doc_n].Open ? 1 : 0;

            if (imgui_.BeginMenu("Open", open_count < app.Documents.Size))
            {
                for (int doc_n = 0; doc_n < app.Documents.Size; doc_n++)
                {
                    MyDocument* doc = &app.Documents[doc_n];
                    if (!doc->Open)
                        if (imgui_.MenuItem(doc->Name))
                            doc->DoOpen();
                }
                imgui_.EndMenu();
            }
            if (imgui_.MenuItem("Close All Documents", NULL, false, open_count > 0))
                for (int doc_n = 0; doc_n < app.Documents.Size; doc_n++)
                    app.Documents[doc_n].DoQueueClose();
            if (imgui_.MenuItem("Exit", "Alt+F4")) {}
            imgui_.EndMenu();
        }
        imgui_.EndMenuBar();
    }

    // [Debug] List documents with one checkbox for each
    for (int doc_n = 0; doc_n < app.Documents.Size; doc_n++)
    {
        MyDocument* doc = &app.Documents[doc_n];
        if (doc_n > 0)
            imgui_.SameLine();
        imgui_.PushID(doc);
        if (imgui_.Checkbox(doc->Name, &doc->Open))
            if (!doc->Open)
                doc->DoForceClose();
        imgui_.PopID();
    }

    imgui_.Separator();

    // Submit Tab Bar and Tabs
    {
        ImGuiTabBarFlags tab_bar_flags = (opt_fitting_flags) | (opt_reorderable ? ImGuiTabBarFlags_Reorderable : 0);
        if (imgui_.BeginTabBar("##tabs", tab_bar_flags))
        {
            if (opt_reorderable)
                NotifyOfDocumentsClosedElsewhere(imgui_, app);

            // [DEBUG] Stress tests
            //if ((imgui_.GetFrameCount() % 30) == 0) docs[1].Open ^= 1;            // [DEBUG] Automatically show/hide a tab. Test various interactions e.g. dragging with this on.
            //if (imgui_.GetIO().KeyCtrl) imgui_.SetTabItemSelected(docs[1].Name);  // [DEBUG] Test SetTabItemSelected(), probably not very useful as-is anyway..

            // Submit Tabs
            for (int doc_n = 0; doc_n < app.Documents.Size; doc_n++)
            {
                MyDocument* doc = &app.Documents[doc_n];
                if (!doc->Open)
                    continue;

                ImGuiTabItemFlags tab_flags = (doc->Dirty ? ImGuiTabItemFlags_UnsavedDocument : 0);
                bool visible = imgui_.BeginTabItem(doc->Name, &doc->Open, tab_flags);

                // Cancel attempt to close when unsaved add to save queue so we can display a popup.
                if (!doc->Open && doc->Dirty)
                {
                    doc->Open = true;
                    doc->DoQueueClose();
                }

                MyDocument::DisplayContextMenu(doc);
                if (visible)
                {
                    MyDocument::DisplayContents(doc);
                    imgui_.EndTabItem();
                }
            }

            imgui_.EndTabBar();
        }
    }

    // Update closing queue
    static ImVector<MyDocument*> close_queue(imgui_);
    if (close_queue.empty())
    {
        // Close queue is locked once we started a popup
        for (int doc_n = 0; doc_n < app.Documents.Size; doc_n++)
        {
            MyDocument* doc = &app.Documents[doc_n];
            if (doc->WantClose)
            {
                doc->WantClose = false;
                close_queue.push_back(doc);
            }
        }
    }

    // Display closing confirmation UI
    if (!close_queue.empty())
    {
        int close_queue_unsaved_documents = 0;
        for (int n = 0; n < close_queue.Size; n++)
            if (close_queue[n]->Dirty)
                close_queue_unsaved_documents++;

        if (close_queue_unsaved_documents == 0)
        {
            // Close documents when all are unsaved
            for (int n = 0; n < close_queue.Size; n++)
                close_queue[n]->DoForceClose();
            close_queue.clear();
        }
        else
        {
            if (!imgui_.IsPopupOpen("Save?"))
                imgui_.OpenPopup("Save?");
            if (imgui_.BeginPopupModal("Save?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                imgui_.Text("Save change to the following items?");
                float item_height = imgui_.GetTextLineHeightWithSpacing();
                if (imgui_.BeginChildFrame(imgui_.GetID("frame"), ImVec2(-FLT_MIN, 6.25f * item_height)))
                {
                    for (int n = 0; n < close_queue.Size; n++)
                        if (close_queue[n]->Dirty)
                            imgui_.Text("%s", close_queue[n]->Name);
                    imgui_.EndChildFrame();
                }

                ImVec2 button_size(imgui_.GetFontSize() * 7.0f, 0.0f);
                if (imgui_.Button("Yes", button_size))
                {
                    for (int n = 0; n < close_queue.Size; n++)
                    {
                        if (close_queue[n]->Dirty)
                            close_queue[n]->DoSave();
                        close_queue[n]->DoForceClose();
                    }
                    close_queue.clear();
                    imgui_.CloseCurrentPopup();
                }
                imgui_.SameLine();
                if (imgui_.Button("No", button_size))
                {
                    for (int n = 0; n < close_queue.Size; n++)
                        close_queue[n]->DoForceClose();
                    close_queue.clear();
                    imgui_.CloseCurrentPopup();
                }
                imgui_.SameLine();
                if (imgui_.Button("Cancel", button_size))
                {
                    close_queue.clear();
                    imgui_.CloseCurrentPopup();
                }
                imgui_.EndPopup();
            }
        }
    }

    imgui_.End();
}

// End of Demo code
#else

void ImGui::ShowAboutWindow(bool*) {}
void ImGui::ShowDemoWindow(bool*) {}
void ImGui::ShowUserGuide() {}
void ImGui::ShowStyleEditor(ImGuiStyle*) {}

#endif

#endif // #ifndef IMGUI_DISABLE
