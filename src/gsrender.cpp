#include "gsrender.h"
#include <vector>

// ---- minimal gsapi declarations (no SDK headers needed) ----
#define GS_ARG_ENCODING_UTF8 1
typedef int (__stdcall *PFN_gsapi_revision)(void*, int);
typedef int (__stdcall *PFN_gsapi_new_instance)(void**, void*);
typedef void (__stdcall *PFN_gsapi_delete_instance)(void*);
typedef int (__stdcall *PFN_gsapi_set_arg_encoding)(void*, int);
typedef int (__stdcall *PFN_gsapi_set_stdio)(void*,
    int (__stdcall *)(void*, char*, int),
    int (__stdcall *)(void*, const char*, int),
    int (__stdcall *)(void*, const char*, int));
typedef int (__stdcall *PFN_gsapi_init_with_args)(void*, int, char**);
typedef int (__stdcall *PFN_gsapi_exit)(void*);
typedef int (__stdcall *PFN_gsapi_run_string)(void*, const char*, int, int*);
typedef int (__stdcall *PFN_gsapi_set_stdio_with_handle)(void*,
    int (__stdcall *)(void*, char*, int),
    int (__stdcall *)(void*, const char*, int),
    int (__stdcall *)(void*, const char*, int), void*);

static std::wstring g_dir;
static HMODULE g_lib = NULL;
static bool g_tried = false;
static CRITICAL_SECTION g_cs;
static bool g_csInit = false;

static int __stdcall NullStdin(void*, char*, int) { return 0; }
static int __stdcall NullStdout(void*, const char*, int len) { return len; }
static int __stdcall NullStderr(void*, const char*, int len) { return len; }

void GSRender::SetPluginDir(const std::wstring& dir) { g_dir = dir; }

static void EnsureCS() {
    if (!g_csInit) { InitializeCriticalSection(&g_cs); g_csInit = true; }
}

static HMODULE LoadGS();
static std::wstring OwnDir() {
    HMODULE h = NULL;
    // address inside this DLL -> its own path (works even if TC never
    // calls ListSetDefaultParams, e.g. thumbnail-only loads)
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)&LoadGS, &h)) {
        wchar_t buf[MAX_PATH * 2]{};
        if (GetModuleFileNameW(h, buf, MAX_PATH * 2)) {
            std::wstring m(buf);
            size_t p = m.find_last_of(L"\\/");
            if (p != std::wstring::npos) return m.substr(0, p);
        }
    }
    return L"";
}

static HMODULE LoadGS() {
    if (g_tried) return g_lib;
    g_tried = true;
#ifdef _WIN64
    const wchar_t* name = L"gsdll64.dll";
#else
    const wchar_t* name = L"gsdll32.dll";
#endif
    if (!g_dir.empty())
        g_lib = LoadLibraryW((g_dir + L"\\" + name).c_str());
    if (!g_lib) {
        std::wstring od = OwnDir();
        if (!od.empty())
            g_lib = LoadLibraryW((od + L"\\" + name).c_str());
    }
    if (!g_lib)
        g_lib = LoadLibraryW(name); // PATH / installed copy
    if (g_lib && g_dir.empty()) {
        // remember for -I resource paths
        std::wstring od = OwnDir();
        if (!od.empty()) g_dir = od;
    }
    return g_lib;
}

bool GSRender::Available() { return LoadGS() != NULL; }

static std::string ToUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, NULL, 0, NULL, NULL);
    std::string s(n ? n - 1 : 0, '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, s.data(), n, NULL, NULL);
    return s;
}

static std::wstring ShortPath(const std::wstring& p) {
    wchar_t buf[MAX_PATH * 2]{};
    DWORD n = GetShortPathNameW(p.c_str(), buf, MAX_PATH * 2);
    if (n == 0 || n >= MAX_PATH * 2) return p;
    return buf;
}

static std::wstring TempBmpPath() {
    wchar_t tmp[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tmp);
    std::wstring d = std::wstring(tmp) + L"TCPDFview\\render";
    CreateDirectoryW((std::wstring(tmp) + L"TCPDFview").c_str(), NULL);
    CreateDirectoryW(d.c_str(), NULL);
    wchar_t name[MAX_PATH]{};
    swprintf_s(name, L"\\gs_%lu_%u.bmp", GetCurrentProcessId(), GetTickCount());
    return d + name;
}

static bool RunGS(const std::wstring& pdf, int page, int dpi, const std::wstring& out) {
    HMODULE lib = LoadGS();
    if (!lib) return false;
    auto pNew = (PFN_gsapi_new_instance)GetProcAddress(lib, "gsapi_new_instance");
    auto pDel = (PFN_gsapi_delete_instance)GetProcAddress(lib, "gsapi_delete_instance");
    auto pEnc = (PFN_gsapi_set_arg_encoding)GetProcAddress(lib, "gsapi_set_arg_encoding");
    auto pStd = (PFN_gsapi_set_stdio)GetProcAddress(lib, "gsapi_set_stdio");
    auto pInit = (PFN_gsapi_init_with_args)GetProcAddress(lib, "gsapi_init_with_args");
    auto pExit = (PFN_gsapi_exit)GetProcAddress(lib, "gsapi_exit");
    if (!pNew || !pDel || !pInit || !pExit) return false;

    // short ANSI-safe paths, forward slashes for the PS parser
    std::wstring pdfS = ShortPath(pdf), outS = ShortPath(out);
    for (auto& c : pdfS) if (c == L'\\') c = L'/';
    for (auto& c : outS) if (c == L'\\') c = L'/';
    std::string pdfA = ToUtf8(pdfS), outA = ToUtf8(outS);
    std::string libA, resA;
    if (!g_dir.empty()) {
        libA = ToUtf8(g_dir + L"/lib");
        resA = ToUtf8(g_dir + L"/Resource");
        for (auto& c : libA) if (c == '\\') c = '/';
        for (auto& c : resA) if (c == '\\') c = '/';
    }

    char dpiS[16], pgS[16];
    sprintf_s(dpiS, "%d", dpi);
    sprintf_s(pgS, "%d", page);

    std::vector<std::string> owned;
    owned.push_back("gs");
    owned.push_back("-dBATCH"); owned.push_back("-dNOPAUSE"); owned.push_back("-dSAFER");
    owned.push_back("-sDEVICE=bmp16m");
    owned.push_back(std::string("-dFirstPage=") + pgS);
    owned.push_back(std::string("-dLastPage=") + pgS);
    owned.push_back(std::string("-r") + dpiS);
    owned.push_back("-dTextAlphaBits=4"); owned.push_back("-dGraphicsAlphaBits=4");
    if (!libA.empty()) { owned.push_back("-I" + libA); owned.push_back("-I" + resA); }
    owned.push_back(std::string("-sOutputFile=") + outA);
    owned.push_back(pdfA);

    std::vector<char*> argv;
    for (auto& s : owned) argv.push_back(s.data());

    EnsureCS();
    EnterCriticalSection(&g_cs);
    void* inst = NULL;
    bool ok = false;
    if (pNew(&inst, NULL) >= 0 && inst) {
        if (pEnc) pEnc(inst, GS_ARG_ENCODING_UTF8);
        if (pStd) pStd(inst, NullStdin, NullStdout, NullStderr);
        pInit(inst, (int)argv.size(), argv.data()); // ignore code; file existence is ground truth
        pExit(inst);
        pDel(inst);
        WIN32_FILE_ATTRIBUTE_DATA fad{};
        if (GetFileAttributesExW(out.c_str(), GetFileExInfoStandard, &fad) &&
            (((uint64_t)fad.nFileSizeHigh << 32) | fad.nFileSizeLow) > 1000)
            ok = true;
    }
    LeaveCriticalSection(&g_cs);
    return ok;
}

HBITMAP GSRender::RenderPage(const std::wstring& pdfPath, int page, int dpi) {
    if (page < 1) page = 1;
    if (dpi < 36) dpi = 36; if (dpi > 300) dpi = 300;
    std::wstring out = TempBmpPath();
    DeleteFileW(out.c_str());
    if (!RunGS(pdfPath, page, dpi, out)) { DeleteFileW(out.c_str()); return NULL; }
    HBITMAP bmp = (HBITMAP)LoadImageW(NULL, out.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    DeleteFileW(out.c_str());
    return bmp;
}

static int __stdcall CapStdout(void* h, const char* str, int len) {
    if (h && str && len > 0) ((std::string*)h)->append(str, len);
    return len;
}

int GSRender::GetPageCount(const std::wstring& pdfPath) {
    HMODULE lib = LoadGS();
    if (!lib) return -1;
    auto pNew = (PFN_gsapi_new_instance)GetProcAddress(lib, "gsapi_new_instance");
    auto pDel = (PFN_gsapi_delete_instance)GetProcAddress(lib, "gsapi_delete_instance");
    auto pEnc = (PFN_gsapi_set_arg_encoding)GetProcAddress(lib, "gsapi_set_arg_encoding");
    auto pStdH = (PFN_gsapi_set_stdio_with_handle)GetProcAddress(lib, "gsapi_set_stdio_with_handle");
    auto pRun = (PFN_gsapi_run_string)GetProcAddress(lib, "gsapi_run_string");
    auto pExit = (PFN_gsapi_exit)GetProcAddress(lib, "gsapi_exit");
    if (!pNew || !pDel || !pRun || !pExit) return -1;

    std::wstring ps = ShortPath(pdfPath);
    for (auto& c : ps) if (c == L'\\') c = L'/';
    std::string pa = ToUtf8(ps), esc;
    for (char c : pa) {
        if (c == '\\' || c == '(' || c == ')') esc.push_back('\\');
        esc.push_back(c);
    }
    std::string prog = "(" + esc + ") (r) file runpdfbegin pdfpagecount = quit\n";

    std::string captured;
    int pages = -1;
    EnsureCS();
    EnterCriticalSection(&g_cs);
    void* inst = NULL;
    if (pNew && pNew(&inst, NULL) >= 0 && inst) {
        if (pEnc) pEnc(inst, GS_ARG_ENCODING_UTF8);
        if (pStdH) pStdH(inst, NullStdin, CapStdout, NullStderr, &captured);
        auto pInit = (PFN_gsapi_init_with_args)GetProcAddress(lib, "gsapi_init_with_args");
        int ec = 0, irc = -999, rrc = -999;
        if (pInit) {
            const char* args[] = {"gs", "-dNOSAFER", "-dNODISPLAY", "-q"};
            std::vector<char*> argv;
            for (auto a : args) argv.push_back(const_cast<char*>(a));
            irc = pInit(inst, (int)argv.size(), argv.data());
        }
        if (irc == 0 && pRun)
            rrc = pRun(inst, prog.c_str(), 0, &ec); // ends with quit -> gs_error_Quit, fine
        pExit(inst);
        pDel(inst);
        // parse first integer from captured stdout
        size_t i = 0;
        while (i < captured.size() && !(captured[i] >= '0' && captured[i] <= '9')) i++;
        size_t j = i;
        while (j < captured.size() && captured[j] >= '0' && captured[j] <= '9') j++;
        if (j > i) {
            pages = atoi(captured.substr(i, j - i).c_str());
            if (pages < 1 || pages > 1000000) pages = -1;
        }
    }
    LeaveCriticalSection(&g_cs);
    return pages;
}

HBITMAP GSRender::RenderThumb(const std::wstring& pdfPath, int page, int w, int h) {
    if (w < 16) w = 16; if (h < 16) h = 16;
    int big = w > h ? w : h;
    int dpi = 72 * big / 500; // ~500px reference width at 72dpi
    if (dpi < 40) dpi = 40; if (dpi > 150) dpi = 150;
    HBITMAP src = RenderPage(pdfPath, page, dpi);
    if (!src) return NULL;
    BITMAP b{};
    GetObject(src, sizeof(b), &b);

    HDC screen = GetDC(NULL);
    HDC ddc = CreateCompatibleDC(screen);
    HBITMAP dst = CreateCompatibleBitmap(screen, w, h);
    HBITMAP od = (HBITMAP)SelectObject(ddc, dst);
    RECT rc{0, 0, w, h};
    FillRect(ddc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
    // aspect fit, centered
    double s = min((double)w / b.bmWidth, (double)h / b.bmHeight);
    int dw = max(1, (int)(b.bmWidth * s)), dh = max(1, (int)(b.bmHeight * s));
    int dx = (w - dw) / 2, dy = (h - dh) / 2;
    HDC sdc = CreateCompatibleDC(screen);
    HBITMAP os = (HBITMAP)SelectObject(sdc, src);
    SetStretchBltMode(ddc, HALFTONE);
    StretchBlt(ddc, dx, dy, dw, dh, sdc, 0, 0, b.bmWidth, b.bmHeight, SRCCOPY);
    SelectObject(sdc, os);
    DeleteDC(sdc);
    // frame
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(160, 160, 160));
    HPEN op = (HPEN)SelectObject(ddc, pen);
    HBRUSH nob = (HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH ob = (HBRUSH)SelectObject(ddc, nob);
    Rectangle(ddc, 0, 0, w, h);
    SelectObject(ddc, ob); SelectObject(ddc, op);
    DeleteObject(pen);
    SelectObject(ddc, od);
    DeleteDC(ddc);
    ReleaseDC(NULL, screen);
    DeleteObject(src);
    return dst;
}
