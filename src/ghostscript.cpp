#include "ghostscript.h"
#include "pdfparse.h"

static std::wstring g_pluginDir;
static HMODULE g_gs = NULL;
static bool g_tried = false;

void GhostscriptInterface::SetPluginDir(const std::wstring& dir) { g_pluginDir = dir; }

static HMODULE TryLoadGs() {
    if (g_tried) return g_gs;
    g_tried = true;
#ifdef _WIN64
    const wchar_t* name = L"gsdll64.dll";
#else
    const wchar_t* name = L"gsdll32.dll";
#endif
    // 1) plugin dir, 2) PATH
    if (!g_pluginDir.empty()) {
        std::wstring p = g_pluginDir + L"\\" + name;
        g_gs = LoadLibraryW(p.c_str());
        if (g_gs) return g_gs;
    }
    g_gs = LoadLibraryW(name);
    return g_gs; // may be NULL -> fallback mode
}

bool GhostscriptInterface::Initialize() { TryLoadGs(); return true; }
void GhostscriptInterface::Shutdown() { /* keep loaded; TC unloads DLL */ }
bool GhostscriptInterface::Available() { return TryLoadGs() != NULL; }

// Fallback thumbnail: red "PDF" banner + filename + page count.
// If Ghostscript DLL is present we still return this placeholder for v0.1
// (full GS rasterization lands in v0.2); presence is reported via Available().
HBITMAP GhostscriptInterface::RenderFirstPage(const std::wstring& pdfPath, int maxW, int maxH) {
    if (maxW < 16) maxW = 16; if (maxW > 1024) maxW = 1024;
    if (maxH < 16) maxH = 16; if (maxH > 1024) maxH = 1024;
    PdfInfo info = ParsePdf(pdfPath);
    int pages = info.valid ? info.pageCount : 0;

    HDC screen = GetDC(NULL);
    HDC dc = CreateCompatibleDC(screen);
    HBITMAP bmp = CreateCompatibleBitmap(screen, maxW, maxH);
    HBITMAP old = (HBITMAP)SelectObject(dc, bmp);

    RECT rc{0, 0, maxW, maxH};
    HBRUSH bg = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(dc, &rc, bg);
    DeleteObject(bg);

    int bannerH = maxH / 3;
    if (bannerH < 16) bannerH = 16;
    RECT rb{0, 0, maxW, bannerH};
    HBRUSH red = CreateSolidBrush(RGB(200, 30, 30));
    FillRect(dc, &rb, red);
    DeleteObject(red);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    HFONT f = CreateFontW(bannerH * 2 / 3, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH, L"Arial");
    HFONT of = (HFONT)SelectObject(dc, f);
    DrawTextW(dc, L"PDF", -1, &rb, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, of);
    DeleteObject(f);

    SetTextColor(dc, RGB(40, 40, 40));
    RECT rt{4, bannerH + 2, maxW - 4, maxH - 2};
    wchar_t buf[256];
    const wchar_t* base = pdfPath.c_str();
    const wchar_t* bs = wcsrchr(base, L'\\');
    if (bs) base = bs + 1;
    std::wstring fname(base);
    if (fname.size() > 28) fname = fname.substr(0, 25) + L"...";
    swprintf_s(buf, L"%s\n%d str.", fname.c_str(), pages);
    HFONT f2 = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH, L"Arial");
    of = (HFONT)SelectObject(dc, f2);
    DrawTextW(dc, buf, -1, &rt, DT_CENTER | DT_TOP | DT_WORDBREAK);
    SelectObject(dc, of);
    DeleteObject(f2);

    // border
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(160, 160, 160));
    HPEN op = (HPEN)SelectObject(dc, pen);
    HBRUSH nob = (HBRUSH)GetStockObject(NULL_BRUSH);
    Rectangle(dc, 0, 0, maxW, maxH);
    SelectObject(dc, op);
    DeleteObject(pen);

    SelectObject(dc, old);
    DeleteDC(dc);
    ReleaseDC(NULL, screen);
    return bmp;
}
