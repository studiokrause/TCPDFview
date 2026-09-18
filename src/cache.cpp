#include "cache.h"
#include <shlobj.h>
#include <fstream>
#include <vector>

static std::wstring g_cacheDir;

static uint64_t Fnv1a(const std::wstring& s) {
    uint64_t h = 1469598103934665603ULL;
    for (wchar_t c : s) {
        wchar_t lc = (c >= L'A' && c <= L'Z') ? (wchar_t)(c + 32) : c;
        h ^= (uint64_t)lc;
        h *= 1099511628211ULL;
    }
    return h;
}

static bool GetFileId(const std::wstring& path, uint64_t& size, uint64_t& mtime) {
    WIN32_FILE_ATTRIBUTE_DATA fad{};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fad)) return false;
    size = ((uint64_t)fad.nFileSizeHigh << 32) | fad.nFileSizeLow;
    mtime = ((uint64_t)fad.ftLastWriteTime.dwHighDateTime << 32) | fad.ftLastWriteTime.dwLowDateTime;
    return true;
}

std::wstring ThumbnailCache::CacheDir() { return g_cacheDir; }

bool ThumbnailCache::InitCacheDir() {
    wchar_t tmp[MAX_PATH]{};
    if (!GetTempPathW(MAX_PATH, tmp)) return false;
    g_cacheDir = std::wstring(tmp) + L"TCPDFview\\cache";
    // create recursively
    std::wstring cur;
    for (size_t i = 0; i < g_cacheDir.size(); ++i) {
        cur.push_back(g_cacheDir[i]);
        if (g_cacheDir[i] == L'\\' && cur.size() > 4)
            CreateDirectoryW(cur.c_str(), NULL);
    }
    CreateDirectoryW(g_cacheDir.c_str(), NULL);
    return true;
}

std::wstring ThumbnailCache::GetCacheBmpPath(const std::wstring& pdfPath, int width, int height) {
    if (g_cacheDir.empty()) InitCacheDir();
    uint64_t h = Fnv1a(pdfPath);
    wchar_t name[128]{};
    swprintf_s(name, L"\\%016llx_%dx%d.bmp", (unsigned long long)h, width, height);
    return g_cacheDir + name;
}

static std::wstring MetaPath(const std::wstring& bmp) { return bmp + L".meta"; }

bool ThumbnailCache::IsCacheValid(const std::wstring& pdfPath, const std::wstring& bmpPath) {
    if (GetFileAttributesW(bmpPath.c_str()) == INVALID_FILE_ATTRIBUTES) return false;
    std::ifstream m(MetaPath(bmpPath));
    if (!m) return false;
    uint64_t s1 = 0, t1 = 0;
    m >> s1 >> t1;
    uint64_t s2 = 0, t2 = 0;
    if (!GetFileId(pdfPath, s2, t2)) return false;
    return s1 == s2 && t1 == t2;
}

bool ThumbnailCache::LoadCachedBitmap(const std::wstring& pdfPath, int width, int height, HBITMAP* out) {
    std::wstring bmp = GetCacheBmpPath(pdfPath, width, height);
    if (!IsCacheValid(pdfPath, bmp)) return false;
    HBITMAP h = (HBITMAP)LoadImageW(NULL, bmp.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (!h) return false;
    *out = h;
    return true;
}

bool ThumbnailCache::StoreCachedBitmap(const std::wstring& pdfPath, int width, int height, HBITMAP bmp) {
    if (!bmp) return false;
    std::wstring path = GetCacheBmpPath(pdfPath, width, height);
    // save via GDI: create DIB and write BMP file
    BITMAP bm{};
    if (!GetObjectW(bmp, sizeof(bm), &bm)) return false;
    HDC screen = GetDC(NULL);
    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = bm.bmWidth;
    bi.bmiHeader.biHeight = bm.bmHeight;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 24;
    bi.bmiHeader.biCompression = BI_RGB;
    int stride = ((bm.bmWidth * 3 + 3) & ~3);
    std::vector<BYTE> bits((size_t)stride * bm.bmHeight);
    HDC dc = CreateCompatibleDC(screen);
    HBITMAP old = (HBITMAP)SelectObject(dc, bmp);
    int lines = GetDIBits(dc, bmp, 0, bm.bmHeight, bits.data(), &bi, DIB_RGB_COLORS);
    SelectObject(dc, old);
    DeleteDC(dc);
    ReleaseDC(NULL, screen);
    if (lines == 0) return false;
    BITMAPFILEHEADER fh{};
    fh.bfType = 0x4D42;
    fh.bfOffBits = sizeof(fh) + sizeof(BITMAPINFOHEADER);
    fh.bfSize = fh.bfOffBits + (DWORD)bits.size();
    HANDLE hf = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hf == INVALID_HANDLE_VALUE) return false;
    DWORD wr = 0;
    WriteFile(hf, &fh, sizeof(fh), &wr, NULL);
    WriteFile(hf, &bi.bmiHeader, sizeof(BITMAPINFOHEADER), &wr, NULL);
    WriteFile(hf, bits.data(), (DWORD)bits.size(), &wr, NULL);
    CloseHandle(hf);
    uint64_t s = 0, t = 0;
    if (GetFileId(pdfPath, s, t)) {
        std::ofstream m(MetaPath(path));
        m << s << " " << t;
    }
    return true;
}

void ThumbnailCache::ClearCache() {
    if (g_cacheDir.empty()) InitCacheDir();
    std::wstring spec = g_cacheDir + L"\\*";
    WIN32_FIND_DATAW fd{};
    HANDLE h = FindFirstFileW(spec.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (fd.cFileName[0] == L'.') continue;
        std::wstring p = g_cacheDir + L"\\" + fd.cFileName;
        DeleteFileW(p.c_str());
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}
