#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <string>
#include "lang.h"
#include "cache.h"
#include "ghostscript.h"

// Total Commander Lister Plugin Interface Definitions
#define LST_OK          0
#define LST_ERROR       1

#define LST_CB_enthalt 0
#define LST_CB_sound  1

struct ListerWindowInfo {
    HWND hwndParent;
    HWND hwndViewer;
    std::wstring currentFile;
    int currentPage;
    double zoom;
};

static ListerWindowInfo* g_activeLister = nullptr;

extern "C" {

HWND __stdcall ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags) {
    SetLanguage(DetectSystemLanguage());
    GhostscriptInterface::Initialize();
    ThumbnailCache::InitCacheDir();

    HWND hwndViewer = CreateWindowExW(
        0, L"STATIC", L"TCPDFview PDF Viewer",
        WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
        0, 0, 100, 100, ParentWin, NULL, GetModuleHandle(NULL), NULL
    );

    g_activeLister = new ListerWindowInfo{ ParentWin, hwndViewer, std::wstring(FileToLoad, FileToLoad + strlen(FileToLoad)), 1, 1.0 };
    return hwndViewer;
}

void __stdcall ListCloseWindow(HWND ListWin) {
    if (g_activeLister && g_activeLister->hwndViewer == ListWin) {
        delete g_activeLister;
        g_activeLister = nullptr;
    }
    GhostscriptInterface::Shutdown();
}

int __stdcall ListKeydown(HWND ListWin, int Key, int Modifier) {
    if (!g_activeLister) return LST_ERROR;

    // Keyboard shortcuts:
    // '0' or '*': Fit page to window
    if (Key == '0' || Key == '*' || Key == VK_MULTIPLY) {
        g_activeLister->zoom = 1.0;
        InvalidateRect(g_activeLister->hwndViewer, NULL, TRUE);
        return LST_OK;
    }
    // '+' / '-': Zoom in / out
    if (Key == VK_ADD || Key == 187 || Key == '+') { g_activeLister->zoom *= 1.1; InvalidateRect(g_activeLister->hwndViewer, NULL, TRUE); return LST_OK; }
    if (Key == VK_SUBTRACT || Key == 189 || Key == '-') { g_activeLister->zoom /= 1.1; InvalidateRect(g_activeLister->hwndViewer, NULL, TRUE); return LST_OK; }

    // Page Up / Page Down
    if (Key == VK_NEXT) { g_activeLister->currentPage++; InvalidateRect(g_activeLister->hwndViewer, NULL, TRUE); return LST_OK; }
    if (Key == VK_PRIOR) { if (g_activeLister->currentPage > 1) g_activeLister->currentPage--; InvalidateRect(g_activeLister->hwndViewer, NULL, TRUE); return LST_OK; }

    // Esc: Close
    if (Key == VK_ESCAPE) {
        SendMessage(g_activeLister->hwndParent, WM_CLOSE, 0, 0);
        return LST_OK;
    }

    // Enter: Open file in TC
    if (Key == VK_RETURN && !(Modifier & 4)) { // no Shift
        return LST_OK;
    }

    // Shift + Enter: Open file in Explorer
    if (Key == VK_RETURN && (Modifier & 4)) {
        ShellExecuteW(NULL, L"open", g_activeLister->currentFile.c_str(), NULL, NULL, SW_SHOWNORMAL);
        return LST_OK;
    }

    return 1; // unhandled
}

void __stdcall ListNotificationReceived(HWND ListWin, int Message, WPARAM Param) {
    if (Message == 1) { // Right click / Context menu
        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, 1001, GetString("clear_cache").c_str());
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, MF_STRING, 1002, GetString("about_menu").c_str());

        POINT pt;
        GetCursorPos(&pt);
        int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, ListWin, NULL);
        DestroyMenu(hMenu);

        if (cmd == 1001) {
            ThumbnailCache::ClearCache();
        } else if (cmd == 1002) {
            MessageBoxW(ListWin, GetString("about_text").c_str(), GetString("about_title").c_str(), MB_OK | MB_ICONINFORMATION);
        }
    }
}

int __stdcall ThumbExtract(char* FileToLoad, int Side, HBITMAP* Thumbnail, char* AdditionalInfo, int MaxAdditionalInfoLen) {
    return LST_OK;
}

} // extern "C"
