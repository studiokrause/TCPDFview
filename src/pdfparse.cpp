#include "pdfparse.h"
#include <fstream>
#include <sstream>
#include <algorithm>

#define TCPDFVIEW_VERSION L"0.1"

std::wstring FileVersionString() { return TCPDFVIEW_VERSION; }

static std::string ReadFileHead(const std::wstring& path, size_t maxBytes) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    std::string data;
    data.resize(maxBytes);
    f.read(data.data(), maxBytes);
    data.resize((size_t)f.gcount());
    return data;
}

static std::wstring ToW(const std::string& s) {
    // PDF text is usually WinAnsi/PDFDoc; do lossy latin1->wchar conversion
    std::wstring w;
    w.reserve(s.size());
    for (unsigned char c : s) {
        if (c == '\r') continue;
        w.push_back((wchar_t)c);
    }
    return w;
}

// Extract text inside ( ... ) with Tj/TJ operators, very tolerant.
static void ExtractParenText(const std::string& data, std::vector<std::wstring>& out) {
    std::string cur;
    bool inParen = false;
    int depth = 0;
    bool esc = false;
    for (size_t i = 0; i < data.size(); ++i) {
        char c = data[i];
        if (!inParen) {
            if (c == '(') { inParen = true; depth = 1; cur.clear(); esc = false; }
            continue;
        }
        if (esc) { cur.push_back(c); esc = false; continue; }
        if (c == '\\') { esc = true; continue; }
        if (c == '(') { depth++; cur.push_back(c); continue; }
        if (c == ')') {
            depth--;
            if (depth == 0) {
                inParen = false;
                if (cur.size() > 1) {
                    // heuristic: keep strings with printable chars
                    size_t printable = 0;
                    for (unsigned char ch : cur) if (ch >= 32 && ch < 127) printable++;
                    if (printable * 2 >= cur.size())
                        out.push_back(ToW(cur));
                }
                cur.clear();
            } else cur.push_back(c);
            continue;
        }
        cur.push_back(c);
        if (cur.size() > 4096) { inParen = false; cur.clear(); }
    }
}

PdfInfo ParsePdf(const std::wstring& path) {
    PdfInfo info;
    std::string head = ReadFileHead(path, 10);
    if (head.size() < 5 || head.compare(0, 5, "%PDF-") != 0)
        return info; // not a pdf

    // read up to 8 MB for parsing (enough for text preview)
    std::string data = ReadFileHead(path, 8 * 1024 * 1024);

    // page count: count "/Type /Page" but not "/Type /Pages"
    int pages = 0;
    const char* needle = "/Type";
    size_t pos = 0;
    while ((pos = data.find(needle, pos)) != std::string::npos) {
        size_t q = pos + 5;
        while (q < data.size() && (data[q] == ' ' || data[q] == '\t' || data[q] == '\r' || data[q] == '\n' || data[q] == '/')) q++;
        if (data.compare(q, 4, "Page") == 0) {
            size_t after = q + 4;
            if (after >= data.size() || (data[after] != 's' && data[after] != 'S'))
                pages++;
        }
        pos += 5;
    }
    if (pages <= 0) pages = 1;
    if (pages > 10000) pages = 10000;
    info.pageCount = pages;

    std::vector<std::wstring> raw;
    ExtractParenText(data, raw);
    // wrap long lines at ~100 chars
    for (auto& l : raw) {
        if (l.size() <= 120) { info.lines.push_back(l); continue; }
        size_t s = 0;
        while (s < l.size()) {
            info.lines.push_back(l.substr(s, 120));
            s += 120;
        }
        if (info.lines.size() > 2000) break;
    }
    info.valid = true;
    return info;
}
