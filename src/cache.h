#pragma once
#include <string>

class ThumbnailCache {
public:
    static bool InitCacheDir();
    static std::wstring GetCacheFilePath(const std::wstring& pdfPath, int pageNum);
    static bool IsCacheValid(const std::wstring& pdfPath, const std::wstring& cachePath);
    static void ClearCache();
};
