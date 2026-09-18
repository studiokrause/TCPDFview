#include "lang.h"
#include <map>
#include <windows.h>

static Lang currentLang = LANG_EN;

void SetLanguage(Lang lang) { currentLang = lang; }

Lang DetectSystemLanguage() {
    LANGID lid = GetUserDefaultUILanguage();
    switch (PRIMARYLANGID(lid)) {
        case LANG_POLISH: return LANG_PL;
        case LANG_GERMAN: return LANG_DE;
        case LANG_FRENCH: return LANG_FR;
        case LANG_SPANISH: return LANG_ES;
        case LANG_ITALIAN: return LANG_IT;
        default: return LANG_EN;
    }
}

std::wstring GetString(const std::string& key) {
    static const std::map<std::string, std::map<Lang, std::wstring>> dict = {
        {"about_title", {{LANG_PL,L"O programie - TCPDFview"},{LANG_EN,L"About - TCPDFview"},{LANG_DE,L"\u00dcber - TCPDFview"},{LANG_FR,L"\u00c0 propos - TCPDFview"},{LANG_ES,L"Acerca de - TCPDFview"},{LANG_IT,L"Informazioni - TCPDFview"}}},
        {"about_text", {
            {LANG_PL,L"TCPDFview v0.6\nAutor: Studio Krause / muse-spark\nLicencja: AGPL-3.0 (ze wzgl\u0119du na Ghostscript)\nWtyczka podgl\u0105du PDF dla Total Commander (WLX)."},
            {LANG_EN,L"TCPDFview v0.6\nAuthor: Studio Krause / muse-spark\nLicense: AGPL-3.0 (due to Ghostscript)\nPDF viewer plugin for Total Commander (WLX)."},
            {LANG_DE,L"TCPDFview v0.6\nAutor: Studio Krause / muse-spark\nLizenz: AGPL-3.0 (wegen Ghostscript)\nPDF-Betrachter-Plugin f\u00fcr Total Commander (WLX)."},
            {LANG_FR,L"TCPDFview v0.6\nAuteur : Studio Krause / muse-spark\nLicence : AGPL-3.0 (\u00e0 cause de Ghostscript)\nPlugin de visualisation PDF pour Total Commander (WLX)."},
            {LANG_ES,L"TCPDFview v0.6\nAutor: Studio Krause / muse-spark\nLicencia: AGPL-3.0 (debido a Ghostscript)\nPlugin visor de PDF para Total Commander (WLX)."},
            {LANG_IT,L"TCPDFview v0.6\nAutore: Studio Krause / muse-spark\nLicenza: AGPL-3.0 (a causa di Ghostscript)\nPlugin visualizzatore PDF per Total Commander (WLX)."}}},
        {"notext", {
            {LANG_PL,L"(brak warstwy tekstowej na tej stronie \u2014 strona 1 renderowana jest graficznie przez system)"},
            {LANG_EN,L"(no text layer on this page \u2014 page 1 is rendered graphically by the system)"},
            {LANG_DE,L"(keine Textebene auf dieser Seite \u2014 Seite 1 wird vom System grafisch gerendert)"},
            {LANG_FR,L"(aucune couche de texte sur cette page \u2014 la page 1 est rendue graphiquement par le syst\u00e8me)"},
            {LANG_ES,L"(sin capa de texto en esta p\u00e1gina \u2014 la p\u00e1gina 1 la renderiza gr\u00e1ficamente el sistema)"},
            {LANG_IT,L"(nessun livello di testo in questa pagina \u2014 la pagina 1 \u00e8 resa graficamente dal sistema)"}}},
        {"clear_cache", {{LANG_PL,L"Wyczy\u015b\u0107 cache"},{LANG_EN,L"Clear cache"},{LANG_DE,L"Cache leeren"},{LANG_FR,L"Effacer le cache"},{LANG_ES,L"Limpiar cach\u00e9"},{LANG_IT,L"Cancella cache"}}},
        {"cache_cleared", {
            {LANG_PL,L"Cache zosta\u0142o wyczyszczone"},
            {LANG_EN,L"Cache has been cleared"},
            {LANG_DE,L"Cache wurde geleert"},
            {LANG_FR,L"Le cache a \u00e9t\u00e9 effac\u00e9"},
            {LANG_ES,L"Cach\u00e9 eliminada"},
            {LANG_IT,L"Cache cancellata"}}},
        {"fit_width", {
            {LANG_PL,L"Dopasuj do szeroko\u015bci (/)"},
            {LANG_EN,L"Fit to width (/)"},
            {LANG_DE,L"An Breite anpassen (/)"},
            {LANG_FR,L"Ajuster \u00e0 la largeur (/)"},
            {LANG_ES,L"Ajustar al ancho (/)"},
            {LANG_IT,L"Adatta alla larghezza (/)"}}},
        {"about_menu", {{LANG_PL,L"O programie"},{LANG_EN,L"About"},{LANG_DE,L"\u00dcber"},{LANG_FR,L"\u00c0 propos"},{LANG_ES,L"Acerca de"},{LANG_IT,L"Informazioni"}}},
        {"fit", {{LANG_PL,L"Dopasuj do okna (0 / *)"},{LANG_EN,L"Fit to window (0 / *)"},{LANG_DE,L"An Fenster anpassen (0 / *)"},{LANG_FR,L"Ajuster \u00e0 la fen\u00eatre (0 / *)"},{LANG_ES,L"Ajustar a ventana (0 / *)"},{LANG_IT,L"Adatta alla finestra (0 / *)"}}},
        {"zin", {{LANG_PL,L"Powi\u0119ksz (+)"},{LANG_EN,L"Zoom in (+)"},{LANG_DE,L"Vergr\u00f6\u00dfern (+)"},{LANG_FR,L"Zoom avant (+)"},{LANG_ES,L"Acercar (+)"},{LANG_IT,L"Ingrandisci (+)"}}},
        {"zout", {{LANG_PL,L"Pomniejsz (-)"},{LANG_EN,L"Zoom out (-)"},{LANG_DE,L"Verkleinern (-)"},{LANG_FR,L"Zoom arri\u00e8re (-)"},{LANG_ES,L"Alejar (-)"},{LANG_IT,L"Riduci (-)"}}},
        {"prev_page", {{LANG_PL,L"Poprzednia strona (PgUp)"},{LANG_EN,L"Previous page (PgUp)"},{LANG_DE,L"Vorherige Seite (PgUp)"},{LANG_FR,L"Page pr\u00e9c\u00e9dente (PgUp)"},{LANG_ES,L"P\u00e1gina anterior (PgUp)"},{LANG_IT,L"Pagina precedente (PgUp)"}}},
        {"next_page", {{LANG_PL,L"Nast\u0119pna strona (PgDn)"},{LANG_EN,L"Next page (PgDn)"},{LANG_DE,L"N\u00e4chste Seite (PgDn)"},{LANG_FR,L"Page suivante (PgDn)"},{LANG_ES,L"P\u00e1gina siguiente (PgDn)"},{LANG_IT,L"Pagina successiva (PgDn)"}}},
        {"open_tc", {{LANG_PL,L"Otw\u00f3rz w TC (Enter)"},{LANG_EN,L"Open in TC (Enter)"},{LANG_DE,L"In TC \u00f6ffnen (Enter)"},{LANG_FR,L"Ouvrir dans TC (Entr\u00e9e)"},{LANG_ES,L"Abrir en TC (Enter)"},{LANG_IT,L"Apri in TC (Invio)"}}},
        {"open_exp", {{LANG_PL,L"Otw\u00f3rz w Explorerze (Shift+Enter)"},{LANG_EN,L"Open in Explorer (Shift+Enter)"},{LANG_DE,L"In Explorer \u00f6ffnen (Shift+Enter)"},{LANG_FR,L"Ouvrir dans l'Explorateur (Maj+Entr\u00e9e)"},{LANG_ES,L"Abrir en Explorador (Shift+Enter)"},{LANG_IT,L"Apri in Explorer (Maiusc+Invio)"}}},
    };
    auto it = dict.find(key);
    if (it != dict.end()) {
        auto jt = it->second.find(currentLang);
        if (jt != it->second.end()) return jt->second;
        auto je = it->second.find(LANG_EN);
        if (je != it->second.end()) return je->second;
    }
    return std::wstring(key.begin(), key.end());
}
