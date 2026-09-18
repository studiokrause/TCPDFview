#pragma once
#include <string>
#include <windows.h>

// Tries to use Ghostscript DLL from plugin dir if present (gsdll64.dll/gsdll32.dll).
// Falls back to built-in GDI preview when absent. Never fails the preview.
class GhostscriptInterface {
public:
    static void SetPluginDir(const std::wstring& dir);
    static bool Initialize();
    static void Shutdown();
    static bool Available();
    // Render first page to an HBITMAP of max width/height (keeps aspect via 4:3/A4-ish fallback).
    // Returns NULL on failure.
    static HBITMAP RenderFirstPage(const std::wstring& pdfPath, int maxW, int maxH);
};
