// JSON String Converter - Notepad++ plugin
// Converts an escaped JSON string ("{\"a\":1}") into real, formatted JSON.
#include <windows.h>
#include <string>
#include "JsonCore.h"

// ---------------------------------------------------------------------------
// Minimal subset of the Notepad++ plugin interface (PluginInterface.h,
// Notepad_plus_msgs.h, Scintilla.h). Layouts/values match the official SDK.
// ---------------------------------------------------------------------------
struct NppData {
    HWND _nppHandle;
    HWND _scintillaMainHandle;
    HWND _scintillaSecondHandle;
};
typedef void(__cdecl* PFUNCPLUGINCMD)();
struct ShortcutKey { bool _isCtrl; bool _isAlt; bool _isShift; UCHAR _key; };
struct FuncItem {
    wchar_t        _itemName[64];
    PFUNCPLUGINCMD _pFunc;
    int            _cmdID;
    bool           _init2Check;
    ShortcutKey*   _pShKey;
};

#define NPPMSG                   (WM_USER + 1000)
#define NPPM_GETCURRENTSCINTILLA (NPPMSG + 4)
#define NPPM_SETCURRENTLANGTYPE  (NPPMSG + 6)
#define L_JSON                   57

#define SCI_GETLENGTH            2006
#define SCI_GETEOLMODE           2030
#define SCI_BEGINUNDOACTION      2078
#define SCI_ENDUNDOACTION        2079
#define SCI_GETCODEPAGE          2137
#define SCI_GETSELECTIONSTART    2143
#define SCI_GETSELECTIONEND      2145
#define SCI_SETSEL               2160
#define SCI_SETTARGETSTART       2190
#define SCI_SETTARGETEND         2192
#define SCI_REPLACETARGET        2194
#define SCI_GETCHARACTERPOINTER  2520
#define SC_CP_UTF8               65001
#define SC_EOL_CRLF 0
#define SC_EOL_CR   1

// ---------------------------------------------------------------------------
static const wchar_t PLUGIN_NAME[] = L"JSON String Converter";
static NppData g_npp;

static void cmdUnescapeAndFormat();
static void cmdUnescapeOnly();
static void cmdPrettyPrint();
static void cmdMinify();
static void cmdEscape();
static void cmdAbout();

// Ctrl+Alt+Shift+J for the main command.
static ShortcutKey g_keyMain = { true, true, true, 'J' };

static FuncItem g_funcs[] = {
    { L"JSON string -> JSON (formatted)", cmdUnescapeAndFormat, 0, false, &g_keyMain },
    { L"JSON string -> JSON (unescape only)", cmdUnescapeOnly, 0, false, nullptr },
    { L"---", nullptr, 0, false, nullptr },
    { L"Pretty print JSON", cmdPrettyPrint, 0, false, nullptr },
    { L"Minify JSON", cmdMinify, 0, false, nullptr },
    { L"JSON -> JSON string (escape)", cmdEscape, 0, false, nullptr },
    { L"---", nullptr, 0, false, nullptr },
    { L"About", cmdAbout, 0, false, nullptr },
};
static const int FUNC_COUNT = sizeof(g_funcs) / sizeof(g_funcs[0]);

// ---------------------------------------------------------------- helpers
static HWND currentScintilla() {
    int which = -1;
    ::SendMessage(g_npp._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    if (which == -1) return nullptr;
    return which == 0 ? g_npp._scintillaMainHandle : g_npp._scintillaSecondHandle;
}

static LRESULT sci(HWND h, UINT msg, WPARAM w = 0, LPARAM l = 0) { return ::SendMessage(h, msg, w, l); }

static std::wstring toWide(const std::string& s) {
    int n = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

static void showError(const std::string& msg) {
    ::MessageBoxW(g_npp._nppHandle, toWide(msg).c_str(), PLUGIN_NAME, MB_OK | MB_ICONWARNING);
}

struct Target {
    HWND        h = nullptr;
    intptr_t    start = 0, end = 0;
    bool        wholeDoc = false;
    std::string text;
    jsc::Options opt;
};

// Uses the selection if there is one, otherwise the whole document.
static bool getTarget(Target& t) {
    t.h = currentScintilla();
    if (!t.h) return false;
    t.start = sci(t.h, SCI_GETSELECTIONSTART);
    t.end   = sci(t.h, SCI_GETSELECTIONEND);
    if (t.start == t.end) {
        t.start = 0;
        t.end = sci(t.h, SCI_GETLENGTH);
        t.wholeDoc = true;
    }
    const char* p = (const char*)sci(t.h, SCI_GETCHARACTERPOINTER);
    if (!p) return false;
    t.text.assign(p + t.start, (size_t)(t.end - t.start));

    t.opt.utf8 = sci(t.h, SCI_GETCODEPAGE) == SC_CP_UTF8;
    switch (sci(t.h, SCI_GETEOLMODE)) {
        case SC_EOL_CRLF: t.opt.eol = "\r\n"; break;
        case SC_EOL_CR:   t.opt.eol = "\r";   break;
        default:          t.opt.eol = "\n";   break;
    }
    return true;
}

static void replaceTarget(const Target& t, const std::string& result, bool setJsonLang) {
    sci(t.h, SCI_BEGINUNDOACTION);
    sci(t.h, SCI_SETTARGETSTART, (WPARAM)t.start);
    sci(t.h, SCI_SETTARGETEND, (WPARAM)t.end);
    sci(t.h, SCI_REPLACETARGET, (WPARAM)result.size(), (LPARAM)result.data());
    sci(t.h, SCI_ENDUNDOACTION);
    sci(t.h, SCI_SETSEL, (WPARAM)t.start, (LPARAM)(t.start + (intptr_t)result.size()));
    if (setJsonLang && t.wholeDoc)
        ::SendMessage(g_npp._nppHandle, NPPM_SETCURRENTLANGTYPE, 0, (LPARAM)L_JSON);
}

// ---------------------------------------------------------------- commands
static void unescape(bool format) {
    Target t;
    if (!getTarget(t)) return;
    std::string json, err;
    if (!jsc::unescapeJsonString(t.text, t.opt, json, err)) {
        showError("Could not unescape the JSON string:\n\n" + err);
        return;
    }
    if (format) {
        std::string pretty, ferr;
        if (jsc::formatJson(json, true, t.opt, pretty, ferr)) {
            json.swap(pretty);
        } else {
            replaceTarget(t, json, false);
            showError("The string was unescaped, but the result is not valid JSON so it was not formatted:\n\n" + ferr);
            return;
        }
    }
    replaceTarget(t, json, format);
}

static void reformat(bool pretty) {
    Target t;
    if (!getTarget(t)) return;
    std::string out, err;
    if (!jsc::formatJson(t.text, pretty, t.opt, out, err)) {
        showError("Invalid JSON:\n\n" + err);
        return;
    }
    replaceTarget(t, out, true);
}

static void cmdUnescapeAndFormat() { unescape(true); }
static void cmdUnescapeOnly()      { unescape(false); }
static void cmdPrettyPrint()       { reformat(true); }
static void cmdMinify()            { reformat(false); }

static void cmdEscape() {
    Target t;
    if (!getTarget(t)) return;
    std::string src = t.text, minified, err;
    if (jsc::formatJson(src, false, t.opt, minified, err)) src.swap(minified);  // compact valid JSON first
    replaceTarget(t, jsc::escapeToJsonString(src), false);
}

static void cmdAbout() {
    ::MessageBoxW(g_npp._nppHandle,
        L"JSON String Converter 1.0\n\n"
        L"Converts an escaped JSON string such as\n"
        L"    \"{\\\"name\\\":\\\"John\\\",\\\"age\\\":30}\"\n"
        L"into formatted JSON.\n\n"
        L"Works on the current selection, or the whole document if nothing is selected.\n"
        L"Surrounding quotes are optional; double-escaped strings are handled.\n\n"
        L"Shortcut: Ctrl+Alt+Shift+J",
        PLUGIN_NAME, MB_OK | MB_ICONINFORMATION);
}

// ---------------------------------------------------------------- exports
extern "C" {
__declspec(dllexport) BOOL isUnicode() { return TRUE; }
__declspec(dllexport) void setInfo(NppData data) { g_npp = data; }
__declspec(dllexport) const wchar_t* getName() { return PLUGIN_NAME; }
__declspec(dllexport) FuncItem* getFuncsArray(int* n) { *n = FUNC_COUNT; return g_funcs; }
__declspec(dllexport) void beNotified(void* /*SCNotification*/) {}
__declspec(dllexport) LRESULT messageProc(UINT, WPARAM, LPARAM) { return TRUE; }
}

BOOL APIENTRY DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
