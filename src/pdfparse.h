#pragma once
#include <string>
#include <vector>

struct PdfInfo {
    bool valid = false;
    int pageCount = 0;
    std::wstring title;
    std::vector<std::wstring> lines; // cleaned extractable text (may be empty for scanned/compressed PDFs)
    bool textCompressed = false;     // true when page content uses binary filters (Flate/DCT/JPX/...)
};

PdfInfo ParsePdf(const std::wstring& path);
std::wstring FileVersionString();
