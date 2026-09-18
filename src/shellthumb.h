#pragma once
#include <windows.h>
#include <string>

// System thumbnail/render of a file via IShellItemImageFactory
// (uses the PDF thumbnail provider registered in Windows: Edge/PDF24/Adobe).
// Returns HBITMAP (caller owns it, DeleteObject) or NULL when unavailable.
HBITMAP GetShellImage(const std::wstring& path, int width, int height);
