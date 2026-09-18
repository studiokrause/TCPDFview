# Plan Projektu: TCPDFview (v0.1)

Wtyczka do Total Commandera umożliwiająca podgląd PDF w listerze, generowanie miniatur, obsługę cache'u, wielojęzyczność oraz integrację z Ghostscript.

## 1. Architektura i Technologie
- **Język:** C++ (architektura Total Commander WLX - Lister Plugin oraz WDX/Thumbnail).
- **Renderowanie:** Ghostscript DLL (`gsdll32.dll` / `gsdll64.dll`).
- **Wersjonowanie:** `v0.1`, Autor: `Studio Krause / gemini-flash-lite-latest`, Licencja: `AGPL`.
- **Języki:** PL, EN, DE, FR, ES, IT.

## 2. Harmonogram Zadań
1. **Szkielet Wtyczki i Lister:**
   - Obsługa funkcji Listera (`ListLoad`, `ListCloseWindow`, `ListKeydown`, itp.).
   - Skróty klawiszowe: `0`, `*`, `+`, `-`, Strzałki, `PgUp`/`PgDn`, `Esc`, `Enter`, `Shift+Enter`.
   - Menu kontekstowe i okno "O programie" (autor, licencja AGPL, wersja).
2. **Miniatury i Cache:**
   - Obsługa miniatur PDF.
   - Mechanizm cache z walidacją zmian plików oraz opcja "Wyczyść cache" w menu kontekstowym.
3. **Wielojęzyczność i Ghostscript:**
   - Tłumaczenia interfejsu (6 języków).
   - Dołączenie plików dystrybucyjnych Ghostscript.
4. **Repozytorium GitHub i Release:**
   - Inicjalizacja repozytorium, stworzenie prywatnego repo na GitHubie przez `gh`.
   - `README.md` z opisem, wersją i licencją.
   - Zbudowanie paczki instalacyjnej (ZIP) i publikacja Release.
