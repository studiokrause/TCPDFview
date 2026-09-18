# TCPDFview

Total Commander Lister (WLX) plugin — PDF preview + thumbnails with cache.

- **Author:** Studio Krause / muse-spark
- **Version:** 0.6
- **License:** AGPL-3.0 (due to Ghostscript — see `LICENSE` and note below)
- **Type:** Lister plugin (`WLX`): 32-bit `tcpdfview.wlx`, 64-bit `tcpdfview.wlx64`
- **Languages:** PL, EN, DE, FR, ES, IT (auto-detected from system UI language)

## Features (per specyfikacja.txt)

1. **Lister preview (F3 / Ctrl+Q):**
   - `0` / `*` (main + numpad) — fit page to window
   - `/` (main + numpad) — fit to lister width
   - `+` / `-` — zoom in/out
   - Arrows — scroll by a few lines; `PgUp`/`PgDn` — previous/next page
   - `Esc` — close; `Enter` — back to file in TC; `Shift+Enter` — reveal file in Windows Explorer
   - Search (`ListSearchText`) and copy-text (`lc_copy`) supported
2. **Thumbnails:** `ListGetPreviewBitmap(W)` renders page 1 via Ghostscript, aspect-fitted to TC's requested size (system thumbnail, then badge as fallbacks).
3. **Thumbnail cache:** `%TEMP%\TCPDFview\cache\*.bmp + *.meta` (FNV-1a of path + file size + mtime). Reused across previews; invalidated when the PDF changes. Right-click menu → **Clear cache**.
4. **Ghostscript 10.08.0 bundled:** `gsdll32.dll` / `gsdll64.dll` + `Resource/` + `iccprofiles/` + `lib/` ship inside the installer ZIP (see `gs/`). The plugin renders every page through `gsapi` (`bmp16m` device, 150 dpi in Lister). The whole plugin is therefore licensed **AGPL-3.0**.
5. **Context menu:** all commands available on right-click; last item is **About** (author + version + AGPL).

## Rendering (v0.6)

Every page is rasterized by the bundled Ghostscript (`gsapi`, `bmp16m`,
150 dpi, antialiased text+graphics) — no more shell icons. Fallback chain
per page: Ghostscript → system thumbnail (page 1) → clean extracted text
(binary PDF streams are skipped, no mojibake). Thumbnails are composited
over white (AlphaBlend), so 32-bit ARGB sources never show black.

## Installation (Total Commander auto-install)

1. In Total Commander, navigate to the downloaded ZIP (`TCPDFview_v0.6_x86.zip` for 32-bit TC, `TCPDFview_v0.6_x64.zip` for 64-bit TC) and press **Enter** — TC reads `pluginst.inf` (`[plugininstall]`, `type=wlx`) and offers installation.
2. Confirm the directory (default `TCPDFview`).
3. Press `F3` on any `.pdf` — detection string is `MULTIMEDIA & (EXT="PDF" | ([0]="%" & [1]="P" & [2]="D" & [3]="F"))`.

Manual: unpack to e.g. `C:\totalcmd\Plugins\wlx\TCPDFview\`, then Configuration → Options → Plugins → Lister → Add → pick `.wlx` / `.wlx64`.

## ZIP contents (each arch)

| File | Purpose |
|---|---|
| `tcpdfview.wlx` (x86) / `tcpdfview.wlx64` (x64) | plugin DLL with correct WLX exports |
| `gsdll32.dll` (x86) / `gsdll64.dll` (x64) | Ghostscript 10.08.0 interpreter DLL |
| `Resource/` + `iccprofiles/` + `lib/` | Ghostscript support files (fonts, init, ICC) |
| `pluginst.inf` | TC auto-install manifest (`[plugininstall]`) |
| `README.md` | this file |
| `LICENSE` | AGPL-3.0 (full text; covers plugin + bundled Ghostscript) |

Verified exports (both DLLs): `ListLoad/W`, `ListLoadNext/W`, `ListCloseWindow`, `ListGetDetectString`, `ListSetDefaultParams`, `ListGetPreviewBitmap/W`, `ListSearchText/W`, `ListSendCommand`, `ListPrint/W`.

## Build from source

```powershell
cmake -B build_x86 -A Win32; cmake --build build_x86 --config Release
cmake -B build_x64 -A x64;  cmake --build build_x64 --config Release
# outputs: build_x86\Release\tcpdfview.wlx, build_x64\Release\tcpdfview.wlx64
```

Requires CMake + MSVC (Windows SDK). Ghostscript trees are fetched by
`tools\fetch-gs.ps1` (needs 7-Zip) into git-ignored `gs-stage\`.

## Ghostscript note + AGPL

Installers bundle Ghostscript **10.08.0** binaries from the official release:
https://github.com/ArtifexSoftware/ghostpdl-downloads/releases/tag/gs10080
(sources: same page / https://www.ghostscript.com). Ghostscript is AGPL-3.0 —
hence this plugin is distributed under **AGPL-3.0** (full text in `LICENSE`).

## Changelog

- **0.6** — `/` fits page to lister width (new menu item + 6 translations); "Clear cache" now confirms with "Cache has been cleared" (localized); true page count via Ghostscript `pdfpagecount` (fixes PgDn stopping after first pages in long PDFs with object streams/compressed xref).
- **0.5** — Polish/menu encoding fix: all UI strings use `\u` escapes (encoding-independent) + `/utf-8` build flag; About/menu/about-text now show correct national characters in all 6 languages.
- **0.4** — Ghostscript rendering: every page rasterized via bundled `gsdll` (`gsapi`, 150 dpi); DLL + `Resource/` + `iccprofiles/` + `lib/` inside both installers; full AGPL-3.0 text; `tools/fetch-gs.ps1` reproduces the bundle.
- **0.3** — black-preview fix: shell bitmaps flattened over white (AlphaBlend, `msimg32`); verified on 32-bit TC path (x86 DLL in 32-bit process) and x64.
- **0.2** — real rendering: system thumbnail provider draws page 1 in Lister and thumbnails; parser skips binary streams (no mojibake); clean text fallback for other pages.
- **0.1** — first working release: WLX-compliant viewer, thumbnails + validating cache, 6 languages, x86/x64 TC installers.
