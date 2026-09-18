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
std::wstring GetString(const std::string& key);
