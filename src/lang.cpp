#include "lang.h"
#include <map>
#include <windows.h>

static Lang currentLang = LANG_EN;

void SetLanguage(Lang lang) {
    currentLang = lang;
}

Lang DetectSystemLanguage() {
    LANGID lid = GetUserDefaultUILanguage();
    WORD primLang = PRIMARYLANGID(lid);
    switch (primLang) {
        case LANG_POLISH: return LANG_PL;
        case LANG_GERMAN: return LANG_DE;
        case LANG_FRENCH: return LANG_FR;
        case LANG_SPANISH: return LANG_ES;
        case LANG_ITALIAN: return LANG_IT;
        default: return LANG_EN;
    }
}

std::wstring GetString(const std::string& key) {
    static std::map<std::string, std::map<Lang, std::wstring>> dict = {
        {"about_title", {
            {LANG_PL, L"O programie - TCPDFview"},
            {LANG_EN, L"About - TCPDFview"},
            {LANG_DE, L"Über - TCPDFview"},
            {LANG_FR, L"À propos - TCPDFview"},
            {LANG_ES, L"Acerca de - TCPDFview"},
            {LANG_IT, L"Informazioni - TCPDFview"}
        }},
        {"about_text", {
            {LANG_PL, L"TCPDFview v0.1\nAutor: Studio Krause / gemini-flash-lite-latest\nLicencja: AGPL (zgodnie z Ghostscript)\nWtyczka podglądu PDF dla Total Commander."},
            {LANG_EN, L"TCPDFview v0.1\nAuthor: Studio Krause / gemini-flash-lite-latest\nLicense: AGPL (due to Ghostscript)\nPDF viewer plugin for Total Commander."},
            {LANG_DE, L"TCPDFview v0.1\nAutor: Studio Krause / gemini-flash-lite-latest\nLizenz: AGPL (aufgrund von Ghostscript)\nPDF-Betrachter-Plugin für Total Commander."},
            {LANG_FR, L"TCPDFview v0.1\nAuteur : Studio Krause / gemini-flash-lite-latest\nLicence : AGPL (en raison de Ghostscript)\nPlugin de visualisation PDF pour Total Commander."},
            {LANG_ES, L"TCPDFview v0.1\nAutor: Studio Krause / gemini-flash-lite-latest\nLicencia: AGPL (debido a Ghostscript)\nPlugin visor de PDF para Total Commander."},
            {LANG_IT, L"TCPDFview v0.1\nAutore: Studio Krause / gemini-flash-lite-latest\nLicenza: AGPL (a causa di Ghostscript)\nPlugin visualizzatore PDF per Total Commander."}
        }},
        {"clear_cache", {
            {LANG_PL, L"Wyczyść cache"},
            {LANG_EN, L"Clear cache"},
            {LANG_DE, L"Cache leeren"},
            {LANG_FR, L"Effacer le cache"},
            {LANG_ES, L"Limpiar caché"},
            {LANG_IT, L"Cancella cache"}
        }},
        {"about_menu", {
            {LANG_PL, L"O programie"},
            {LANG_EN, L"About"},
            {LANG_DE, L"Über"},
            {LANG_FR, L"À propos"},
            {LANG_ES, L"Acerca de"},
            {LANG_IT, L"Informazioni"}
        }}
    };

    if (dict.find(key) != dict.end() && dict[key].find(currentLang) != dict[key].end()) {
        return dict[key][currentLang];
    }
    return std::wstring(key.begin(), key.end());
}
