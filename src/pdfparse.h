#pragma once
#include <string>
#include <vector>

struct PdfInfo {
    bool valid = false;
    int pageCount = 0;
    std::wstring title;
    std::vector<std::wstring> lines; // extracted text lines (all pages, split)
};

PdfInfo ParsePdf(const std::wstring& path);
std::wstring FileVersionString();
