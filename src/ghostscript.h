#pragma once
#include <string>

class GhostscriptInterface {
public:
    static bool Initialize();
    static void Shutdown();
    static bool RenderPageToBMP(const std::wstring& pdfPath, int pageNum, const std::wstring& bmpPath, int width, int height);
};
