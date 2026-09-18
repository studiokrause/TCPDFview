#pragma once
#include <string>
#include <windows.h>

// Real PDF rasterization via bundled Ghostscript DLL (gsdll32/64.dll).
// Thread-safe (internal lock; GS allows one instance per process).
// All functions return NULL/false on failure; callers must fall back.
class GSRender {
public:
    // pluginDir: folder containing gsdll32.dll/gsdll64.dll + Resource/ + iccprofiles/
    static void SetPluginDir(const std::wstring& dir);
    static bool Available(); // DLL present and loadable

    // Render one page to an HBITMAP (caller owns it). dpi clamped 36..300.
    static HBITMAP RenderPage(const std::wstring& pdfPath, int page, int dpi);

    // Render one page aspect-fitted onto an exact w×h white bitmap (for thumbnails).
    static HBITMAP RenderThumb(const std::wstring& pdfPath, int page, int w, int h);
};
