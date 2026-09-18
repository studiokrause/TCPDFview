#pragma once
#include <string>

enum Lang {
    LANG_PL = 0,
    LANG_EN,
    LANG_DE,
    LANG_FR,
    LANG_ES,
    LANG_IT
};

void SetLanguage(Lang lang);
Lang DetectSystemLanguage();
// keys: about_title, about_text, clear_cache, about_menu, fit, zin, zout,
// prev_page, next_page, open_tc, open_exp, notext
std::wstring GetString(const std::string& key);
