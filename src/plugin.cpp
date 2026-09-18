// TCPDFview v0.5 — Total Commander Lister (WLX) plugin for PDF.
// Implements official WLX API: ListLoad/W, ListLoadNext/W, ListCloseWindow,
// ListGetDetectString, ListSetDefaultParams, ListGetPreviewBitmap/W,
// ListSearchText/W, ListSendCommand, ListPrint/W.
// Window: custom class with scrollbars, zoom, paging, full context menu (6 langs).
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <map>
#include "listplug.h"
#include "lang.h"
#include "cache.h"
#include "ghostscript.h"
#include "gsrender.h"
#include "shellthumb.h"
#include "pdfparse.h"

#define TCPDFVIEW_VERSION L"0.5"

static HINSTANCE g_hInst = NULL;
static std::wstring g_iniPath;
static std::wstring g_pluginDir;
static const wchar_t* kClass = L"TCPDFviewViewer";

// ---- per-window state (multi-window safe) ----
struct ViewerState {
    HWND hwnd = NULL;
    HWND hwndParent = NULL;
    std::wstring file;
    PdfInfo pdf;
    HBITMAP pageBmp = NULL; // system-rendered first page (IShellItemImageFactory)
    int bmpW = 0, bmpH = 0;
    int curPage = 1;
    double zoom = 1.0;
    bool fit = true;
    int scrollX = 0;
    int scrollY = 0;
};

static void FreePageBmp(ViewerState* st) {
    if (st && st->pageBmp) { DeleteObject(st->pageBmp); st->pageBmp = NULL; }
}

static void RenderCurrentPage(ViewerState* st) {
    FreePageBmp(st);
    HCURSOR oldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
    // 1) Ghostscript (bundled): true per-page raster, 150 dpi
    st->pageBmp = GSRender::RenderPage(st->file, st->curPage, 150);
    // 2) system thumbnail (first page only, but better than nothing)
    if (!st->pageBmp && st->curPage == 1)
        st->pageBmp = FlattenOverWhite(GetShellImage(st->file, 1400, 1400));
    SetCursor(oldCur);
    if (st->pageBmp) {
        BITMAP b{};
        if (GetObject(st->pageBmp, sizeof(b), &b)) { st->bmpW = b.bmWidth; st->bmpH = b.bmHeight; }
    }
}

static std::map<HWND, ViewerState*> g_views;

static ViewerState* GetState(HWND h) {
    auto it = g_views.find(h);
    return it == g_views.end() ? NULL : it->second;
}

static std::wstring BaseName(const std::wstring& p) {
    const wchar_t* b = p.c_str();
    const wchar_t* s = wcsrchr(b, L'\\');
    if (!s) s = wcsrchr(b, L'/');
    return s ? (s + 1) : b;
}

static void UpdateScrollbars(ViewerState* st) {
    SCROLLINFO si{};
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0; si.nMax = 2000; si.nPage = 200; si.nPos = st->scrollY;
    SetScrollInfo(st->hwnd, SB_VERT, &si, TRUE);
    si.nPos = st->scrollX;
    SetScrollInfo(st->hwnd, SB_HORZ, &si, TRUE);
}

static void ShowAbout(HWND hwnd) {
    MessageBoxW(hwnd, GetString("about_text").c_str(), GetString("about_title").c_str(),
        MB_OK | MB_ICONINFORMATION);
}

static void OpenInExplorer(const std::wstring& file) {
    std::wstring args = L"/select,\"" + file + L"\"";
    ShellExecuteW(NULL, L"open", L"explorer.exe", args.c_str(), NULL, SW_SHOWNORMAL);
}

static void DoContextMenu(ViewerState* st) {
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING, 11, GetString("fit").c_str());
    AppendMenuW(m, MF_STRING, 12, GetString("zin").c_str());
    AppendMenuW(m, MF_STRING, 13, GetString("zout").c_str());
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING, 14, GetString("prev_page").c_str());
    AppendMenuW(m, MF_STRING, 15, GetString("next_page").c_str());
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING, 16, GetString("open_tc").c_str());
    AppendMenuW(m, MF_STRING, 17, GetString("open_exp").c_str());
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING, 18, GetString("clear_cache").c_str());
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING, 19, GetString("about_menu").c_str()); // last position = About
    POINT pt; GetCursorPos(&pt);
    int cmd = TrackPopupMenu(m, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, st->hwnd, NULL);
    DestroyMenu(m);
    switch (cmd) {
        case 11: st->fit = true; st->zoom = 1.0; st->scrollX = st->scrollY = 0; break;
        case 12: st->fit = false; st->zoom *= 1.25; if (st->zoom > 8) st->zoom = 8; break;
        case 13: st->fit = false; st->zoom /= 1.25; if (st->zoom < 0.2) st->zoom = 0.2; break;
        case 14: if (st->curPage > 1) { st->curPage--; RenderCurrentPage(st); } st->scrollY = 0; break;
        case 15: if (st->curPage < st->pdf.pageCount) { st->curPage++; RenderCurrentPage(st); } st->scrollY = 0; break;
        case 16: SendMessageW(st->hwndParent, WM_CLOSE, 0, 0); return; // back to TC file
        case 17: OpenInExplorer(st->file); return;
        case 18: ThumbnailCache::ClearCache(); MessageBoxW(st->hwnd, GetString("clear_cache").c_str(), GetString("about_title").c_str(), MB_OK | MB_ICONINFORMATION); return;
        case 19: ShowAbout(st->hwnd); return;
        default: return;
    }
    UpdateScrollbars(st);
    InvalidateRect(st->hwnd, NULL, TRUE);
}

static LRESULT CALLBACK ViewerWndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    ViewerState* st = GetState(h);
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(h, &ps);
            RECT rc; GetClientRect(h, &rc);
            FillRect(dc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
            if (st) {
                int margin = 12;
                int topY = 54;
                // header
                wchar_t head[512];
                swprintf_s(head, L"TCPDFview v%s  |  %s  |  %d/%d  |  %d%%",
                    TCPDFVIEW_VERSION, BaseName(st->file).c_str(),
                    st->curPage, st->pdf.pageCount > 0 ? st->pdf.pageCount : 1,
                    (int)(st->zoom * 100));
                SetBkMode(dc, TRANSPARENT);
                DrawTextW(dc, head, -1, &RECT{margin, 4, rc.right - margin, 50}, DT_LEFT | DT_WORDBREAK);

                if (st->pageBmp && st->bmpW > 0 && st->bmpH > 0) {
                    // Ghostscript raster of the current page (or system thumbnail for p.1)
                    int availW = rc.right - rc.left - 2 * margin;
                    int availH = rc.bottom - topY - margin;
                    if (availW < 50) availW = 50; if (availH < 50) availH = 50;
                    double fitScale = min((double)availW / st->bmpW, (double)availH / st->bmpH);
                    double scale = st->fit ? fitScale : fitScale * st->zoom;
                    int dw = max(50, (int)(st->bmpW * scale));
                    int dh = max(50, (int)(st->bmpH * scale));
                    int dx = margin + (st->fit ? (availW - dw) / 2 : -st->scrollX);
                    int dy = topY + (st->fit ? 0 : -st->scrollY);
                    HDC mem = CreateCompatibleDC(dc);
                    HBITMAP old = (HBITMAP)SelectObject(mem, st->pageBmp);
                    SetStretchBltMode(dc, HALFTONE);
                    StretchBlt(dc, dx, dy, dw, dh, mem, 0, 0, st->bmpW, st->bmpH, SRCCOPY);
                    SelectObject(mem, old);
                    DeleteDC(mem);
                    // frame
                    HPEN pen = CreatePen(PS_SOLID, 1, RGB(140, 140, 140));
                    HPEN op = (HPEN)SelectObject(dc, pen);
                    HBRUSH nob = (HBRUSH)GetStockObject(NULL_BRUSH);
                    HBRUSH ob = (HBRUSH)SelectObject(dc, nob);
                    Rectangle(dc, dx, dy, dx + dw, dy + dh);
                    SelectObject(dc, ob); SelectObject(dc, op);
                    DeleteObject(pen);
                } else {
                    // clean text fallback (pages >1 or no system thumbnail)
                    int pageW = rc.right - rc.left - 2 * margin;
                    int pageH = rc.bottom - topY - margin;
                    if (pageW < 50) pageW = 50; if (pageH < 50) pageH = 50;
                    RECT page{margin, topY, margin + pageW, topY + pageH};
                    HBRUSH white = CreateSolidBrush(RGB(255,255,255));
                    FillRect(dc, &page, white);
                    DeleteObject(white);
                    FrameRect(dc, &page, (HBRUSH)GetStockObject(GRAY_BRUSH));
                    RECT body{page.left + 10, page.top + 8, page.right - 10, page.bottom - 10};
                    int totalPages = st->pdf.pageCount > 0 ? st->pdf.pageCount : 1;
                    size_t per = st->pdf.lines.empty() ? 0 : (st->pdf.lines.size() + totalPages - 1) / totalPages;
                    size_t from = (size_t)(st->curPage - 1) * per;
                    std::wstring chunk;
                    for (size_t i = from; i < from + per && i < st->pdf.lines.size(); ++i) {
                        chunk += st->pdf.lines[i];
                        chunk += L"\r\n";
                    }
                    if (chunk.empty()) chunk = GetString("notext");
                    DrawTextW(dc, chunk.c_str(), -1, &body, DT_LEFT | DT_TOP | DT_WORDBREAK);
                }
            }
            EndPaint(h, &ps);
            return 0;
        }
        case WM_SIZE: if (st && st->fit) InvalidateRect(h, NULL, TRUE); return 0;
        case WM_VSCROLL: if (st) {
            int d = 0;
            switch (LOWORD(wp)) {
                case SB_LINEUP: d = -40; break;
                case SB_LINEDOWN: d = 40; break;
                case SB_PAGEUP: d = -400; break;
                case SB_PAGEDOWN: d = 400; break;
                case SB_THUMBTRACK: st->scrollY = HIWORD(wp); UpdateScrollbars(st); InvalidateRect(h, NULL, TRUE); return 0;
            }
            st->scrollY += d; if (st->scrollY < 0) st->scrollY = 0;
            UpdateScrollbars(st); InvalidateRect(h, NULL, TRUE); return 0;
        } break;
        case WM_HSCROLL: if (st) {
            int d = (LOWORD(wp) == SB_LINERIGHT) ? 40 : -40;
            if (LOWORD(wp) == SB_THUMBTRACK) st->scrollX = HIWORD(wp);
            else { st->scrollX += d; if (st->scrollX < 0) st->scrollX = 0; }
            UpdateScrollbars(st); InvalidateRect(h, NULL, TRUE); return 0;
        } break;
        case WM_MOUSEWHEEL: if (st) {
            int d = GET_WHEEL_DELTA_WPARAM(wp);
            st->scrollY -= d / 2; if (st->scrollY < 0) st->scrollY = 0;
            UpdateScrollbars(st); InvalidateRect(h, NULL, TRUE); return 0;
        } break;
        case WM_CONTEXTMENU: if (st) { DoContextMenu(st); return 0; } break;
        case WM_KEYDOWN: if (st) {
            int key = (int)wp;
            bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            if (key == '0' || key == VK_NUMPAD0 || key == VK_MULTIPLY || key == 106 || key == 42) {
                st->fit = true; st->zoom = 1.0; st->scrollX = st->scrollY = 0;
                UpdateScrollbars(st); InvalidateRect(h, NULL, TRUE); return 0;
            }
            if (key == VK_ADD || key == VK_OEM_PLUS || key == 107) {
                st->fit = false; st->zoom *= 1.25; if (st->zoom > 8) st->zoom = 8;
                UpdateScrollbars(st); InvalidateRect(h, NULL, TRUE); return 0;
            }
            if (key == VK_SUBTRACT || key == VK_OEM_MINUS || key == 109) {
                st->fit = false; st->zoom /= 1.25; if (st->zoom < 0.2) st->zoom = 0.2;
                UpdateScrollbars(st); InvalidateRect(h, NULL, TRUE); return 0;
            }
            if (key == VK_UP) { st->scrollY -= 40; if (st->scrollY < 0) st->scrollY = 0; UpdateScrollbars(st); InvalidateRect(h, NULL, TRUE); return 0; }
            if (key == VK_DOWN) { st->scrollY += 40; UpdateScrollbars(st); InvalidateRect(h, NULL, TRUE); return 0; }
            if (key == VK_LEFT) { st->scrollX -= 40; if (st->scrollX < 0) st->scrollX = 0; UpdateScrollbars(st); InvalidateRect(h, NULL, TRUE); return 0; }
            if (key == VK_RIGHT) { st->scrollX += 40; UpdateScrollbars(st); InvalidateRect(h, NULL, TRUE); return 0; }
            if (key == VK_NEXT) { if (st->curPage < st->pdf.pageCount) { st->curPage++; RenderCurrentPage(st); } st->scrollY = 0; UpdateScrollbars(st); InvalidateRect(h, NULL, TRUE); return 0; }
            if (key == VK_PRIOR) { if (st->curPage > 1) { st->curPage--; RenderCurrentPage(st); } st->scrollY = 0; UpdateScrollbars(st); InvalidateRect(h, NULL, TRUE); return 0; }
            if (key == VK_ESCAPE) { SendMessageW(st->hwndParent, WM_CLOSE, 0, 0); return 0; }
            if (key == VK_RETURN && !shift) { SendMessageW(st->hwndParent, WM_CLOSE, 0, 0); return 0; }
            if (key == VK_RETURN && shift) { OpenInExplorer(st->file); return 0; }
        } break;
    }
    return DefWindowProcW(h, msg, wp, lp);
}

static void RegisterViewerClass() {
    static bool done = false;
    if (done) return; done = true;
    WNDCLASSW wc{};
    wc.lpfnWndProc = ViewerWndProc;
    wc.hInstance = g_hInst;
    wc.lpszClassName = kClass;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);
}

static std::wstring A2W(const char* s) {
    if (!s) return L"";
    int n = MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, 0);
    std::wstring w(n ? n - 1 : 0, L'\0');
    if (n > 1) MultiByteToWideChar(CP_ACP, 0, s, -1, w.data(), n);
    return w;
}

static HWND CreateViewer(HWND parent, const std::wstring& file) {
    RegisterViewerClass();
    SetLanguage(DetectSystemLanguage());
    ThumbnailCache::InitCacheDir();
    GhostscriptInterface::Initialize();

    PdfInfo info = ParsePdf(file);
    // ListLoad must return NULL for unsupported files (unless forceshow handled by caller)
    if (!info.valid) return NULL;

    HWND hwnd = CreateWindowExW(0, kClass, L"TCPDFview",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
        0, 0, 200, 200, parent, NULL, g_hInst, NULL);
    if (!hwnd) return NULL;
    ViewerState* st = new ViewerState();
    st->hwnd = hwnd; st->hwndParent = parent; st->file = file; st->pdf = std::move(info);
    st->curPage = 1; st->zoom = 1.0; st->fit = true;
    RenderCurrentPage(st);
    g_views[hwnd] = st;
    UpdateScrollbars(st);
    SetFocus(hwnd);
    return hwnd;
}

extern "C" {

HWND __stdcall ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags) {
    return CreateViewer(ParentWin, A2W(FileToLoad));
}

HWND __stdcall ListLoadW(HWND ParentWin, WCHAR* FileToLoad, int ShowFlags) {
    if (!FileToLoad) return NULL;
    return CreateViewer(ParentWin, std::wstring(FileToLoad));
}

int __stdcall ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags) {
    ViewerState* st = GetState(PluginWin);
    if (!st) return LISTPLUGIN_ERROR;
    PdfInfo info = ParsePdf(A2W(FileToLoad));
    if (!info.valid) return LISTPLUGIN_ERROR;
    st->file = A2W(FileToLoad); st->pdf = std::move(info);
    st->curPage = 1; st->scrollX = st->scrollY = 0;
    RenderCurrentPage(st);
    UpdateScrollbars(st); InvalidateRect(PluginWin, NULL, TRUE);
    return LISTPLUGIN_OK;
}

int __stdcall ListLoadNextW(HWND ParentWin, HWND PluginWin, WCHAR* FileToLoad, int ShowFlags) {
    ViewerState* st = GetState(PluginWin);
    if (!st || !FileToLoad) return LISTPLUGIN_ERROR;
    PdfInfo info = ParsePdf(std::wstring(FileToLoad));
    if (!info.valid) return LISTPLUGIN_ERROR;
    st->file = FileToLoad; st->pdf = std::move(info);
    st->curPage = 1; st->scrollX = st->scrollY = 0;
    RenderCurrentPage(st);
    UpdateScrollbars(st); InvalidateRect(PluginWin, NULL, TRUE);
    return LISTPLUGIN_OK;
}

void __stdcall ListCloseWindow(HWND ListWin) {
    auto it = g_views.find(ListWin);
    if (it != g_views.end()) { FreePageBmp(it->second); delete it->second; g_views.erase(it); }
    DestroyWindow(ListWin);
}

void __stdcall ListGetDetectString(char* DetectString, int maxlen) {
    const char* s = "MULTIMEDIA & (EXT=\"PDF\" | ([0]=\"%\" & [1]=\"P\" & [2]=\"D\" & [3]=\"F\"))";
    strncpy_s(DetectString, maxlen, s, _TRUNCATE);
}

void __stdcall ListSetDefaultParams(ListDefaultParamStruct* dps) {
    if (!dps) return;
    char ini[MAX_PATH]{};
    strncpy_s(ini, dps->DefaultIniName, _TRUNCATE);
    g_iniPath = A2W(ini);
    // plugin dir = dir of this DLL
    wchar_t mod[MAX_PATH]{};
    GetModuleFileNameW(g_hInst, mod, MAX_PATH);
    std::wstring m(mod);
    size_t p = m.find_last_of(L"\\/");
    g_pluginDir = (p == std::wstring::npos) ? L"" : m.substr(0, p);
    GhostscriptInterface::SetPluginDir(g_pluginDir);
    GSRender::SetPluginDir(g_pluginDir);
}

static int PreviewPageFor(const std::wstring& file) {
    // thumbnails always show page 1
    (void)file; return 1;
}

static HBITMAP PreviewFor(const std::wstring& file, int w, int h) {
    HBITMAP cached = NULL;
    if (ThumbnailCache::LoadCachedBitmap(file, w, h, &cached)) return cached;
    HBITMAP bmp = GSRender::RenderThumb(file, PreviewPageFor(file), w, h); // Ghostscript first
    if (!bmp) bmp = FlattenOverWhite(GetShellImage(file, w, h)); // system fallback
    if (!bmp) bmp = GhostscriptInterface::RenderFirstPage(file, w, h); // badge last resort
    if (bmp) ThumbnailCache::StoreCachedBitmap(file, w, h, bmp);
    return bmp;
}

HBITMAP __stdcall ListGetPreviewBitmap(char* FileToLoad, int width, int height, char* contentbuf, int contentbuflen) {
    if (width <= 0 || height <= 0) return NULL;
    // quick header check when contentbuf available
    if (contentbuf && contentbuflen >= 5 && memcmp(contentbuf, "%PDF-", 5) != 0) {
        std::string fn = FileToLoad ? FileToLoad : "";
        if (fn.size() < 4 || _stricmp(fn.substr(fn.size() - 4).c_str(), ".pdf") != 0) return NULL;
    }
    ThumbnailCache::InitCacheDir();
    GhostscriptInterface::Initialize();
    return PreviewFor(A2W(FileToLoad), width, height);
}

HBITMAP __stdcall ListGetPreviewBitmapW(WCHAR* FileToLoad, int width, int height, char* contentbuf, int contentbuflen) {
    if (!FileToLoad || width <= 0 || height <= 0) return NULL;
    if (contentbuf && contentbuflen >= 5 && memcmp(contentbuf, "%PDF-", 5) != 0) {
        std::wstring fn(FileToLoad);
        if (fn.size() < 4 || _wcsicmp(fn.substr(fn.size() - 4).c_str(), L".pdf") != 0) return NULL;
    }
    ThumbnailCache::InitCacheDir();
    GhostscriptInterface::Initialize();
    return PreviewFor(std::wstring(FileToLoad), width, height);
}

int __stdcall ListSearchText(HWND ListWin, char* SearchString, int SearchParameter) {
    ViewerState* st = GetState(ListWin);
    if (!st || !SearchString) return LISTPLUGIN_ERROR;
    std::wstring needle = A2W(SearchString);
    if (!(SearchParameter & lcs_matchcase)) {
        for (auto& c : needle) c = towlower(c);
    }
    bool backwards = (SearchParameter & lcs_backwards) != 0;
    if (!backwards) {
        for (size_t i = 0; i < st->pdf.lines.size(); ++i) {
            std::wstring hay = st->pdf.lines[i];
            if (!(SearchParameter & lcs_matchcase)) for (auto& c : hay) c = towlower(c);
            if (hay.find(needle) != std::wstring::npos) return LISTPLUGIN_OK;
        }
    } else {
        for (size_t i = st->pdf.lines.size(); i-- > 0;) {
            std::wstring hay = st->pdf.lines[i];
            if (!(SearchParameter & lcs_matchcase)) for (auto& c : hay) c = towlower(c);
            if (hay.find(needle) != std::wstring::npos) return LISTPLUGIN_OK;
        }
    }
    return LISTPLUGIN_ERROR;
}

int __stdcall ListSearchTextW(HWND ListWin, WCHAR* SearchString, int SearchParameter) {
    ViewerState* st = GetState(ListWin);
    if (!st || !SearchString) return LISTPLUGIN_ERROR;
    std::wstring needle(SearchString);
    if (!(SearchParameter & lcs_matchcase)) for (auto& c : needle) c = towlower(c);
    for (auto& line : st->pdf.lines) {
        std::wstring hay = line;
        if (!(SearchParameter & lcs_matchcase)) for (auto& c : hay) c = towlower(c);
        if (hay.find(needle) != std::wstring::npos) return LISTPLUGIN_OK;
    }
    return LISTPLUGIN_ERROR;
}

int __stdcall ListSendCommand(HWND ListWin, int Command, int Parameter) {
    ViewerState* st = GetState(ListWin);
    if (!st) return LISTPLUGIN_ERROR;
    if (Command == lc_copy) {
        // copy current page text to clipboard
        if (!OpenClipboard(ListWin)) return LISTPLUGIN_ERROR;
        EmptyClipboard();
        std::wstring chunk;
        for (auto& l : st->pdf.lines) { chunk += l; chunk += L"\r\n"; if (chunk.size() > 60000) break; }
        size_t bytes = (chunk.size() + 1) * sizeof(wchar_t);
        HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (hg) {
            memcpy(GlobalLock(hg), chunk.c_str(), bytes);
            GlobalUnlock(hg);
            SetClipboardData(CF_UNICODETEXT, hg);
        }
        CloseClipboard();
        return LISTPLUGIN_OK;
    }
    return LISTPLUGIN_ERROR;
}

int __stdcall ListPrint(HWND ListWin, char* FileToPrint, char* DefPrinter, int PrintFlags, RECT* Margins) {
    return LISTPLUGIN_ERROR;
}
int __stdcall ListPrintW(HWND ListWin, WCHAR* FileToPrint, WCHAR* DefPrinter, int PrintFlags, RECT* Margins) {
    return LISTPLUGIN_ERROR;
}

} // extern "C"

BOOL APIENTRY DllMain(HINSTANCE h, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) { g_hInst = h; DisableThreadLibraryCalls(h); }
    return TRUE;
}
