#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>

#include "TestUtil.cc"
#include "../shared/DebugClient.cc"

static void queryCurrentConsoleFont(HANDLE conout, BOOL max) {
    CONSOLE_FONT_INFO info = {0};
    if (!GetCurrentConsoleFont(conout, max, &info)) {
        cprintf(L"GetCurrentConsoleFont call failed\n");
    } else {
        cprintf(L"info(max=%d): nFont=%u dwFontSize=(%d,%d)\n",
            max, static_cast<unsigned>(info.nFont),
            info.dwFontSize.X, info.dwFontSize.Y);
    }
}

static void queryCurrentConsoleFontEx(HANDLE conout, BOOL max) {
    CONSOLE_FONT_INFOEX infoex = {0};
    infoex.cbSize = sizeof(infoex);
    if (!GetCurrentConsoleFontEx(conout, max, &infoex)) {
        cprintf(L"GetCurrentConsoleFontEx call failed\n");
    } else {
        wchar_t faceName[LF_FACESIZE + 1];
        memcpy(faceName, infoex.FaceName, sizeof(faceName));
        faceName[LF_FACESIZE] = L'\0';
        cprintf(L"infoex(max=%d): nFont=%u dwFontSize=(%d,%d) "
            L"FontFamily=0x%x FontWeight=%u "
            L"FaceName=\"%ls\"",
            max, static_cast<unsigned>(infoex.nFont),
            infoex.dwFontSize.X, infoex.dwFontSize.Y,
            infoex.FontFamily, infoex.FontWeight,
            faceName);
        cprintf(L" (");
        for (int i = 0; i < LF_FACESIZE; ++i) {
            if (i > 0) {
                cprintf(L" ");
            }
            cprintf(L"%X", infoex.FaceName[i]);
            if (infoex.FaceName[i] == L'\0') {
                break;
            }
        }
        cprintf(L")\n");
    }
}

int main() {
    const HANDLE conout = GetStdHandle(STD_OUTPUT_HANDLE);
    queryCurrentConsoleFont(conout, FALSE);
    queryCurrentConsoleFont(conout, TRUE);
    queryCurrentConsoleFontEx(conout, FALSE);
    queryCurrentConsoleFontEx(conout, TRUE);
    const COORD largest = GetLargestConsoleWindowSize(conout);
    cprintf(L"largestConsoleWindowSize=(%d,%d)\n", largest.X, largest.Y);
    for (int i = 0;; ++i) {
        const COORD size = GetConsoleFontSize(conout, i);
        if (size.X == 0 && size.Y == 0) {
            break;
        }
        cprintf(L"font %d: %dx%d\n", i, size.X, size.Y);
    }
    HMODULE kernel32 = LoadLibraryW(L"kernel32.dll");
    FARPROC proc = GetProcAddress(kernel32, "GetNumberOfConsoleFonts");
    if (proc == NULL) {
        cprintf(L"Could not get address of GetNumberOfConsoleFonts\n");
    } else {
        cprintf(L"GetNumberOfConsoleFonts returned %d\n",
            reinterpret_cast<int WINAPI(*)(HANDLE)>(proc)(conout));
    }
    cprintf(L"InputCP=%u OutputCP=%u", GetConsoleCP(), GetConsoleOutputCP());
    return 0;
}
