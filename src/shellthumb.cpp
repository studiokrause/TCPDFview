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
