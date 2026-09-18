#include "shellthumb.h"
#include <shobjidl.h>

HBITMAP GetShellImage(const std::wstring& path, int width, int height) {
    if (width < 16) width = 16; if (width > 2048) width = 2048;
    if (height < 16) height = 16; if (height > 2048) height = 2048;

    HRESULT hrCo = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    bool uninit = SUCCEEDED(hrCo); // RPC_E_CHANGED_MODE -> already init, don't uninit

    HBITMAP result = NULL;
    IShellItem* item = NULL;
    if (SUCCEEDED(SHCreateItemFromParsingName(path.c_str(), NULL, IID_PPV_ARGS(&item)))) {
        IShellItemImageFactory* f = NULL;
        if (SUCCEEDED(item->QueryInterface(IID_PPV_ARGS(&f)))) {
            SIZE sz{(LONG)width, (LONG)height};
            HBITMAP bmp = NULL;
            if (SUCCEEDED(f->GetImage(sz, SIIGBF_BIGGERSIZEOK, &bmp)))
                result = bmp;
            f->Release();
        }
        item->Release();
    }
    if (uninit) CoUninitialize();
    return result;
}

HBITMAP FlattenOverWhite(HBITMAP src) {
    if (!src) return NULL;
    BITMAP b{};
    if (!GetObject(src, sizeof(b), &b) || b.bmWidth <= 0 || b.bmHeight <= 0)
        return src; // unknown format: leave as-is
    if (b.bmBitsPixel <= 24)
        return src; // opaque already, nothing to do

    HDC screen = GetDC(NULL);
    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = b.bmWidth;
    bi.bmiHeader.biHeight = b.bmHeight;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 24;
    bi.bmiHeader.biCompression = BI_RGB;
    void* bits = NULL;
    HBITMAP dst = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (!dst || !bits) {
        if (dst) DeleteObject(dst);
        ReleaseDC(NULL, screen);
        return src;
    }
    HDC ddc = CreateCompatibleDC(screen);
    HDC sdc = CreateCompatibleDC(screen);
    HBITMAP od = (HBITMAP)SelectObject(ddc, dst);
    HBITMAP os = (HBITMAP)SelectObject(sdc, src);
    RECT r{0, 0, b.bmWidth, b.bmHeight};
    FillRect(ddc, &r, (HBRUSH)GetStockObject(WHITE_BRUSH));
    BLENDFUNCTION bf{};
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = AC_SRC_ALPHA;
    if (!AlphaBlend(ddc, 0, 0, b.bmWidth, b.bmHeight, sdc, 0, 0, b.bmWidth, b.bmHeight, bf)) {
        // provider bitmap has no usable alpha -> plain copy
        BitBlt(ddc, 0, 0, b.bmWidth, b.bmHeight, sdc, 0, 0, SRCCOPY);
    }
    SelectObject(ddc, od);
    SelectObject(sdc, os);
    DeleteDC(ddc);
    DeleteDC(sdc);
    ReleaseDC(NULL, screen);
    DeleteObject(src);
    return dst;
}
