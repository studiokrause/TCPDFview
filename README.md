# TCPDFview

Total Commander Lister (WLX) plugin — PDF preview + thumbnails with cache.

- **Author:** Studio Krause / muse-spark
- **Version:** 0.1
- **License:** AGPL-3.0 (due to Ghostscript — see `LICENSE` and note below)
- **Type:** Lister plugin (`WLX`): 32-bit `tcpdfview.wlx`, 64-bit `tcpdfview.wlx64`
- **Languages:** PL, EN, DE, FR, ES, IT (auto-detected from system UI language)

## Features (per specyfikacja.txt)

1. **Lister preview (F3 / Ctrl+Q):**
   - `0` / `*` (main + numpad) — fit page to window
   - `+` / `-` — zoom in/out
   - Arrows — scroll by a few lines; `PgUp`/`PgDn` — previous/next page
   - `Esc` — close; `Enter` — back to file in TC; `Shift+Enter` — reveal file in Windows Explorer
   - Search (`ListSearchText`) and copy-text (`lc_copy`) supported
2. **Thumbnails:** `ListGetPreviewBitmap(W)` renders a PDF badge thumbnail, scaled to TC's requested size.
3. **Thumbnail cache:** `%TEMP%\TCPDFview\cache\*.bmp + *.meta` (FNV-1a of path + file size + mtime). Reused across previews; invalidated when the PDF changes. Right-click menu → **Clear cache**.
4. **Ghostscript:** the plugin dynamically loads `gsdll64.dll` / `gsdll32.dll` from its own folder when present (hi-res raster in v0.2). v0.1 works out-of-the-box with a built-in GDI preview + text extraction, so no binary is bundled. Because the design targets Ghostscript, the whole plugin is licensed **AGPL-3.0**.
5. **Context menu:** all commands available on right-click; last item is **About** (author + version + AGPL).

## Installation (Total Commander auto-install)

1. In Total Commander, navigate to the downloaded ZIP (`TCPDFview_v0.1_x86.zip` for 32-bit TC, `TCPDFview_v0.1_x64.zip` for 64-bit TC) and press **Enter** — TC reads `pluginst.inf` (`[plugininstall]`, `type=wlx`) and offers installation.
2. Confirm the directory (default `TCPDFview`).
3. Press `F3` on any `.pdf` — detection string is `MULTIMEDIA & (EXT="PDF" | ([0]="%" & [1]="P" & [2]="D" & [3]="F"))`.

Manual: unpack to e.g. `C:\totalcmd\Plugins\wlx\TCPDFview\`, then Configuration → Options → Plugins → Lister → Add → pick `.wlx` / `.wlx64`.

## ZIP contents (each arch)

| File | Purpose |
|---|---|
| `tcpdfview.wlx` (x86) / `tcpdfview.wlx64` (x64) | plugin DLL with correct WLX exports |
| `pluginst.inf` | TC auto-install manifest (`[plugininstall]`) |
| `README.md` | this file |
| `LICENSE` | AGPL-3.0 |

Verified exports (both DLLs): `ListLoad/W`, `ListLoadNext/W`, `ListCloseWindow`, `ListGetDetectString`, `ListSetDefaultParams`, `ListGetPreviewBitmap/W`, `ListSearchText/W`, `ListSendCommand`, `ListPrint/W`.

## Build from source

```powershell
cmake -B build_x86 -A Win32; cmake --build build_x86 --config Release
cmake -B build_x64 -A x64;  cmake --build build_x64 --config Release
# outputs: build_x86\Release\tcpdfview.wlx, build_x64\Release\tcpdfview.wlx64
```

Requires CMake + MSVC (Windows SDK). No third-party deps for v0.1.

## Ghostscript note + AGPL

Place official `gsdll64.dll` (or `gsdll32.dll`) next to the plugin to enable future hi-res rendering. Ghostscript is (A)GPL — hence this plugin is distributed under **AGPL-3.0**. Download: https://www.ghostscript.com/releases/gsdnld.html

## Changelog

- **0.1** — first working release: WLX-compliant viewer, thumbnails + validating cache, 6 languages, x86/x64 TC installers.
