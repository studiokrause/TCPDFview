#include "pdfparse.h"
#include <fstream>

#define TCPDFVIEW_VERSION L"0.6"

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

static bool IsCleanText(const std::string& s) {
    if (s.size() < 4 || s.size() > 600) return false;
    size_t ok = 0;
    for (unsigned char c : s) {
        if (c == 9 || c == 10 || c == 13 || (c >= 32 && c < 127)) ok++;
        else if (c >= 0xC0) ok++; // allow UTF-8 lead bytes / latin diacritics
    }
    return ok * 10 >= s.size() * 7; // >=70% printable
}

static std::wstring ToW(const std::string& s) {
    // bytes are WinAnsi-ish; map 1:1 except control chars
    std::wstring w;
    w.reserve(s.size());
    for (unsigned char c : s) {
        if (c == '\r') continue;
        if (c < 9 || (c > 13 && c < 32)) continue; // drop control garbage
        w.push_back((wchar_t)c);
    }
    // trim
    while (!w.empty() && (w.back() == L' ' || w.back() == L'\t')) w.pop_back();
    size_t p = 0;
    while (p < w.size() && (w[p] == L' ' || w[p] == L'\t')) p++;
    if (p) w.erase(0, p);
    return w;
}

// Extract text inside ( ... ), skipping binary stream content implicitly
// via the IsCleanText quality gate (compressed data never passes it).
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
        if (c == '(') { depth++; if (cur.size() < 4096) cur.push_back(c); continue; }
        if (c == ')') {
            depth--;
            if (depth == 0) {
                inParen = false;
                if (IsCleanText(cur)) {
                    std::wstring w = ToW(cur);
                    if (w.size() >= 3) out.push_back(w);
                }
                cur.clear();
                if (out.size() >= 2000) return;
            } else if (cur.size() < 4096) cur.push_back(c);
            continue;
        }
        if (cur.size() < 4096) cur.push_back(c);
        else { inParen = false; cur.clear(); } // runaway binary -> abort token
    }
}

PdfInfo ParsePdf(const std::wstring& path) {
    PdfInfo info;
    std::string head = ReadFileHead(path, 10);
    if (head.size() < 5 || head.compare(0, 5, "%PDF-") != 0)
        return info;

    std::string data = ReadFileHead(path, 8 * 1024 * 1024);

    int pages = 0;
    size_t pos = 0;
    while ((pos = data.find("/Type", pos)) != std::string::npos) {
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

    info.textCompressed =
        data.find("FlateDecode") != std::string::npos ||
        data.find("DCTDecode") != std::string::npos ||
        data.find("JPXDecode") != std::string::npos ||
        data.find("CCITTFaxDecode") != std::string::npos ||
        data.find("LZWDecode") != std::string::npos;

    // Scrub binary streams: keep text only from outside streams or
    // from streams without a binary filter (real page text in
    // uncompressed PDFs still extracts; Flate/DCT/JPX/etc. is skipped).
    std::string scrubbed;
    scrubbed.reserve(data.size());
    size_t cursor = 0;
    while (cursor < data.size()) {
        size_t s = data.find("stream", cursor);
        if (s == std::string::npos) { scrubbed.append(data, cursor, std::string::npos); break; }
        // "stream" must be followed by EOL to be a stream keyword
        size_t after = s + 6;
        bool isStream = after < data.size() && (data[after] == '\r' || data[after] == '\n');
        if (!isStream) { scrubbed.append(data, cursor, s - cursor + 6); cursor = s + 6; continue; }
        scrubbed.append(data, cursor, s - cursor);
        size_t e = data.find("endstream", after);
        if (e == std::string::npos) break;
        std::string dict = data.substr(cursor > 600 ? cursor - 600 : 0, s - (cursor > 600 ? cursor - 600 : 0));
        bool binary = dict.find("FlateDecode") != std::string::npos ||
                      dict.find("DCTDecode") != std::string::npos ||
                      dict.find("JPXDecode") != std::string::npos ||
                      dict.find("CCITTFaxDecode") != std::string::npos ||
                      dict.find("LZWDecode") != std::string::npos ||
                      dict.find("RunLengthDecode") != std::string::npos ||
                      dict.find("JBIG2Decode") != std::string::npos ||
                      dict.find("Crypt") != std::string::npos;
        if (!binary)
            scrubbed.append(data, after, e - after); // keep readable stream content
        cursor = e + 9;
    }

    std::vector<std::wstring> raw;
    ExtractParenText(scrubbed, raw);
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
