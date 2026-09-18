#pragma once
#include <windows.h>
#include <string>

// System thumbnail/render of a file via IShellItemImageFactory
// (uses the PDF thumbnail provider registered in Windows: Edge/PDF24/Adobe).
// Returns HBITMAP (caller owns it, DeleteObject) or NULL when unavailable.
// NOTE: providers often return 32bpp PARGB bitmaps; blitting those with
// StretchBlt/BitBlt shows transparency as BLACK. Always pass the result
// through FlattenOverWhite() before displaying or caching.
HBITMAP GetShellImage(const std::wstring& path, int width, int height);

// Composite src over a white background into a new opaque 24bpp DIB.
// Deletes src and returns the new bitmap (or src itself on failure).
HBITMAP FlattenOverWhite(HBITMAP src);
