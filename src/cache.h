#pragma once
#include <string>
#include <windows.h>

// Real thumbnail cache in %TEMP%\TCPDFview\cache.
// Key = FNV-1a(path lowercase) + size + mtime. Value = BMP + .meta sidecar.
class ThumbnailCache {
public:
    static bool InitCacheDir();
    static std::wstring CacheDir();
    static std::wstring GetCacheBmpPath(const std::wstring& pdfPath, int width, int height);
    static bool IsCacheValid(const std::wstring& pdfPath, const std::wstring& bmpPath);
    static bool LoadCachedBitmap(const std::wstring& pdfPath, int width, int height, HBITMAP* out);
    static bool StoreCachedBitmap(const std::wstring& pdfPath, int width, int height, HBITMAP bmp);
    static void ClearCache();
};
