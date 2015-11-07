// Copyright (c) 2015 Ryan Prichard
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#include "ConsoleFont.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include <algorithm>
#include <string>
#include <vector>

#include "../shared/DebugClient.h"
#include "../shared/OsModule.h"
#include "../shared/WinptyAssert.h"
#include "../shared/winpty_wcsnlen.h"

namespace {

#define COUNT_OF(x) (sizeof(x) / sizeof((x)[0]))

// See https://en.wikipedia.org/wiki/List_of_CJK_fonts
const wchar_t kMSGothic[] = { 0xff2d, 0xff33, 0x0020, 0x30b4, 0x30b7, 0x30c3, 0x30af, 0 }; // Japanese
const wchar_t kNSimSun[] = { 0x65b0, 0x5b8b, 0x4f53, 0 }; // Simplified Chinese
const wchar_t kMingLight[] = { 0x7d30, 0x660e, 0x9ad4, 0 }; // Traditional Chinese
const wchar_t kGulimChe[] = { 0xad74, 0xb9bc, 0xccb4, 0 }; // Korean

struct Font {
    int codePage;
    const wchar_t *faceName;
    int pxSize;
};

const Font kFonts[] = {
    // MS Gothic double-width handling seems to be broken with console versions
    // prior to Windows 10 (including Windows 10's legacy mode), and it's
    // especially broken in Windows 8 and 8.1.  AFAICT, MS Gothic at size 9
    // avoids problems in Windows 7 and minimizes problems in 8/8.1.
    //
    // Test by running: misc/Utf16Echo A2 A3 2014 3044 30FC 4000
    //
    // The first three codepoints are always rendered as half-width with the
    // Windows Japanese fonts.  (Of these, the first two must be half-width,
    // but U+2014 could be either.)  The last three are rendered as full-width,
    // and they are East_Asian_Width=Wide.
    //
    // Windows 7 fails by modeling all codepoints as full-width with font
    // sizes 14 and above.
    //
    // Windows 8 gets U+00A2, U+00A3, U+2014, U+30FC, and U+4000 wrong, but
    // using a point size not listed in the console properties dialog
    // (e.g. "9") is less wrong:
    //
    //             |        code point               |
    //  font       | 00A2 00A3 2014 3044 30FC 4000   | cell size
    // ------------+---------------------------------+----------
    //  8          |  F    F    F    F    H    H     |   4x8
    //  9          |  F    F    F    F    F    F     |   5x9
    //  16         |  F    F    F    F    H    H     |   8x16
    // raster 6x13 |  H    H    H    F    F    H(*)  |   6x13
    //
    // (*) The Raster Font renders U+4000 as a white box (i.e. an unsupported
    // character).
    //
    { 932, kMSGothic, 9 },

    // kNSimSun: I verified that `misc/Utf16Echo A2 A3 2014 3044 30FC 4000`
    // did something sensible with Windows 8.  It *did* with the listed font
    // sizes, but not with unlisted sizes.  Listed sizes:
    //  - 6 ==> 3x7px
    //  - 8 ==> 4x9px
    //  - 10 ==> 5x11px
    //  - 12 ==> 6x14px
    //  - 14 ==> 7x16px
    //  - 16 ==> 8x18px
    //  ...
    //  - 36 ==> 18x41px
    //  - 72 ==> 36x82px
    // U+2014 is modeled and rendered as full-width.
    { 936, kNSimSun, 8 },

    // kMingLight: I verified that `misc/Utf16Echo A2 A3 2014 3044 30FC 4000`
    // did something sensible with Windows 8.  It *did* with the listed font
    // sizes, but not with unlisted sizes.  Listed sizes:
    //  - 6 => 3x7px
    //  - 8 => 4x10px
    //  - 10 => 5x12px
    //  - 12 => 6x14px
    //  - 14 => 7x17px
    //  - 16 => 8x19px
    //  ...
    //  - 36 => 18x43px
    //  - 72 => 36x86px
    // U+2014 is modeled and rendered as full-width.
    { 950, kMingLight, 8 },

    // kGulimChe: I verified that `misc/Utf16Echo A2 A3 2014 3044 30FC 4000`
    // did something sensible with Windows 8.  It *did* with the listed font
    // sizes, but not with unlisted sizes.  Listed sizes:
    //  - 6 ==> 3x7px
    //  - 8 ==> 4x9px
    //  - 10 ==> 5x11px
    //  - 12 ==> 6x14px
    //  - 14 ==> 7x16px
    //  - 16 ==> 8x18px
    //  ...
    //  - 36 ==> 18x41px
    //  - 72 ==> 36x83px
    { 949, kGulimChe, 8 },

    // Listed sizes:
    //  - 5 ==> 2x5px
    //  - 6 ==> 3x6px
    //  - 7 ==> 3x6px
    //  - 8 ==> 4x8px
    //  - 10 ==> 5x10px
    //  - 12 ==> 6x12px
    //  - 14 ==> 7x14px
    //  - 16 ==> 8x16px
    //  ...
    //  - 36 ==> 17x36px
    //  - 72 ==> 34x72px
    { 0, L"Consolas", 8 },

    // Listed sizes:
    //  - 5 ==> 3x5px
    //  - 6 ==> 4x6px
    //  - 7 ==> 4x7px
    //  - 8 ==> 5x8px
    //  - 10 ==> 6x10px
    //  - 12 ==> 7x12px
    //  - 14 ==> 8x14px
    //  - 16 ==> 10x16px
    //  ...
    //  - 36 ==> 22x36px
    //  - 72 ==> 43x72px
    { 0, L"Lucida Console", 6 },
};

class OsModule {
    HMODULE m_module;
public:
    OsModule(const wchar_t *fileName) {
        m_module = LoadLibraryW(fileName);
        ASSERT(m_module != NULL);
    }
    ~OsModule() {
        FreeLibrary(m_module);
    }
    HMODULE handle() const { return m_module; }
    FARPROC proc(const char *funcName) {
        FARPROC ret = GetProcAddress(m_module, funcName);
        if (ret == NULL) {
            trace("GetProcAddress: %s is missing", funcName);
        }
        return ret;
    }
};

// Some of these types and functions are missing from the MinGW headers.
// Others are undocumented.

struct AGENT_CONSOLE_FONT_INFO {
    DWORD nFont;
    COORD dwFontSize;
};

struct AGENT_CONSOLE_FONT_INFOEX {
    ULONG cbSize;
    DWORD nFont;
    COORD dwFontSize;
    UINT FontFamily;
    UINT FontWeight;
    WCHAR FaceName[LF_FACESIZE];
};

// undocumented XP API
typedef BOOL WINAPI SetConsoleFont_t(
            HANDLE hOutput,
            DWORD dwFontIndex);

// XP and up
typedef BOOL WINAPI GetCurrentConsoleFont_t(
            HANDLE hOutput,
            BOOL bMaximumWindow,
            AGENT_CONSOLE_FONT_INFO *lpConsoleCurrentFont);

// XP and up
typedef COORD WINAPI GetConsoleFontSize_t(
            HANDLE hConsoleOutput,
            DWORD nFont);

// Vista and up
typedef BOOL WINAPI GetCurrentConsoleFontEx_t(
            HANDLE hConsoleOutput,
            BOOL bMaximumWindow,
            AGENT_CONSOLE_FONT_INFOEX *lpConsoleCurrentFontEx);

// Vista and up
typedef BOOL WINAPI SetCurrentConsoleFontEx_t(
            HANDLE hConsoleOutput,
            BOOL bMaximumWindow,
            AGENT_CONSOLE_FONT_INFOEX *lpConsoleCurrentFontEx);

#define GET_MODULE_PROC(mod, funcName) \
    m_##funcName = reinterpret_cast<funcName##_t*>((mod).proc(#funcName)); \

#define DEFINE_ACCESSOR(funcName) \
    funcName##_t &funcName() const { \
        ASSERT(valid()); \
        return *m_##funcName; \
    }

class XPFontAPI {
public:
    XPFontAPI() : m_kernel32(L"kernel32.dll") {
        GET_MODULE_PROC(m_kernel32, GetCurrentConsoleFont);
        GET_MODULE_PROC(m_kernel32, GetConsoleFontSize);
    }

    bool valid() const {
        return m_GetCurrentConsoleFont != NULL &&
            m_GetConsoleFontSize != NULL;
    }

    DEFINE_ACCESSOR(GetCurrentConsoleFont)
    DEFINE_ACCESSOR(GetConsoleFontSize)

private:
    OsModule m_kernel32;
    GetCurrentConsoleFont_t *m_GetCurrentConsoleFont;
    GetConsoleFontSize_t *m_GetConsoleFontSize;
};

class UndocumentedXPFontAPI : public XPFontAPI {
public:
    UndocumentedXPFontAPI() : m_kernel32(L"kernel32.dll") {
        GET_MODULE_PROC(m_kernel32, SetConsoleFont);
    }

    bool valid() const {
        return this->XPFontAPI::valid() &&
            m_SetConsoleFont != NULL;
    }

    DEFINE_ACCESSOR(SetConsoleFont)

private:
    OsModule m_kernel32;
    SetConsoleFont_t *m_SetConsoleFont;
};

class VistaFontAPI : public XPFontAPI {
public:
    VistaFontAPI() : m_kernel32(L"kernel32.dll") {
        GET_MODULE_PROC(m_kernel32, GetCurrentConsoleFontEx);
        GET_MODULE_PROC(m_kernel32, SetCurrentConsoleFontEx);
    }

    bool valid() const {
        return this->XPFontAPI::valid() &&
            m_GetCurrentConsoleFontEx != NULL &&
            m_SetCurrentConsoleFontEx != NULL;
    }

    DEFINE_ACCESSOR(GetCurrentConsoleFontEx)
    DEFINE_ACCESSOR(SetCurrentConsoleFontEx)

private:
    OsModule m_kernel32;
    GetCurrentConsoleFontEx_t *m_GetCurrentConsoleFontEx;
    SetCurrentConsoleFontEx_t *m_SetCurrentConsoleFontEx;
};

static std::vector<std::pair<DWORD, COORD> > readFontTable(XPFontAPI &api, HANDLE conout) {
    std::vector<std::pair<DWORD, COORD> > ret;
    for (DWORD i = 0;; ++i) {
        COORD size = api.GetConsoleFontSize()(conout, i);
        if (size.X == 0 && size.Y == 0) {
            break;
        }
        ret.push_back(std::make_pair(i, size));
    }
    return ret;
}

static void dumpFontTable(HANDLE conout, const char *prefix) {
    if (!isTracingEnabled()) {
        return;
    }
    XPFontAPI api;
    if (!api.valid()) {
        trace("dumpFontTable: cannot dump font table -- missing APIs");
        return;
    }
    std::vector<std::pair<DWORD, COORD> > table = readFontTable(api, conout);
    std::string line;
    char tmp[128];
    size_t first = 0;
    while (first < table.size()) {
        size_t last = std::min(table.size() - 1, first + 10 - 1);
        sprintf(tmp, "%sfonts %02u-%02u:",
            prefix, static_cast<unsigned>(first), static_cast<unsigned>(last));
        line = tmp;
        for (size_t i = first; i <= last; ++i) {
            if (i % 10 == 5) {
                line += "  - ";
            }
            sprintf(tmp, " %2dx%-2d", table[i].second.X, table[i].second.Y);
            line += tmp;
        }
        trace("%s", line.c_str());
        first = last + 1;
    }
}

static std::string narrowString(const std::wstring &input)
{
    int mblen = WideCharToMultiByte(
        CP_UTF8, 0,
        input.data(), input.size(),
        NULL, 0, NULL, NULL);
    if (mblen <= 0) {
        return std::string();
    }
    std::vector<char> tmp(mblen);
    int mblen2 = WideCharToMultiByte(
        CP_UTF8, 0,
        input.data(), input.size(),
        tmp.data(), tmp.size(),
        NULL, NULL);
    ASSERT(mblen2 == mblen);
    return std::string(tmp.data(), tmp.size());
}

static std::string stringToCodePoints(const std::wstring &str) {
    std::string ret = "(";
    for (size_t i = 0; i < str.size(); ++i) {
        char tmp[32];
        sprintf(tmp, "%X", str[i]);
        if (ret.size() > 1) {
            ret.push_back(' ');
        }
        ret += tmp;
    }
    ret.push_back(')');
    return ret;
}

static void dumpFontInfoEx(
        const AGENT_CONSOLE_FONT_INFOEX &infoex,
        const char *prefix) {
    if (!isTracingEnabled()) {
        return;
    }
    std::wstring faceName(infoex.FaceName,
        winpty_wcsnlen(infoex.FaceName, COUNT_OF(infoex.FaceName)));
    trace("%snFont=%u dwFontSize=(%d,%d) "
        "FontFamily=0x%x FontWeight=%u FaceName=%s %s",
        prefix,
        static_cast<unsigned>(infoex.nFont),
        infoex.dwFontSize.X, infoex.dwFontSize.Y,
        infoex.FontFamily, infoex.FontWeight, narrowString(faceName).c_str(),
        stringToCodePoints(faceName).c_str());
}

static void dumpVistaFont(VistaFontAPI &api, HANDLE conout, const char *prefix) {
    if (!isTracingEnabled()) {
        return;
    }
    AGENT_CONSOLE_FONT_INFOEX infoex = {0};
    infoex.cbSize = sizeof(infoex);
    if (!api.GetCurrentConsoleFontEx()(conout, FALSE, &infoex)) {
        trace("GetCurrentConsoleFontEx call failed");
        return;
    }
    dumpFontInfoEx(infoex, prefix);
}

static void dumpXPFont(XPFontAPI &api, HANDLE conout, const char *prefix) {
    if (!isTracingEnabled()) {
        return;
    }
    AGENT_CONSOLE_FONT_INFO info = {0};
    if (!api.GetCurrentConsoleFont()(conout, FALSE, &info)) {
        trace("GetCurrentConsoleFont call failed");
        return;
    }
    trace("%snFont=%u dwFontSize=(%d,%d)",
        prefix,
        static_cast<unsigned>(info.nFont),
        info.dwFontSize.X, info.dwFontSize.Y);
}

static bool setFontVista(
        VistaFontAPI &api,
        HANDLE conout,
        const wchar_t *faceName,
        int pxSize) {
    AGENT_CONSOLE_FONT_INFOEX infoex = {0};
    infoex.cbSize = sizeof(AGENT_CONSOLE_FONT_INFOEX);
    infoex.dwFontSize.Y = pxSize;
    infoex.FontWeight = 400;
    wcsncpy(infoex.FaceName, faceName, COUNT_OF(infoex.FaceName));
    dumpFontInfoEx(infoex, "setFontVista: setting font to: ");
    if (!api.SetCurrentConsoleFontEx()(conout, FALSE, &infoex)) {
        trace("setFontVista: SetCurrentConsoleFontEx call failed");
        return false;
    }
    memset(&infoex, 0, sizeof(infoex));
    infoex.cbSize = sizeof(infoex);
    if (!api.GetCurrentConsoleFontEx()(conout, FALSE, &infoex)) {
        trace("setFontVista: GetCurrentConsoleFontEx call failed");
        return false;
    }
    if (wcsncmp(infoex.FaceName, faceName, COUNT_OF(infoex.FaceName)) != 0) {
        trace("setFontVista: face name was not set");
        dumpFontInfoEx(infoex, "setFontVista: post-call font: ");
        return false;
    }
    // We'd like to verify that the new font size is correct, but we can't
    // predict what it will be, even though we just set it to `pxSize` through
    // an apprently symmetric interface.  For the Chinese and Korean fonts, the
    // new `infoex.dwFontSize.Y` value can be slightly larger than the height
    // we specified.
    return true;
}

static void setSmallFontVista(VistaFontAPI &api, HANDLE conout) {
    int codePage = GetConsoleOutputCP();
    for (size_t i = 0; i < COUNT_OF(kFonts); ++i) {
        if (kFonts[i].codePage == 0 || kFonts[i].codePage == codePage) {
            if (setFontVista(api, conout,
                             kFonts[i].faceName, kFonts[i].pxSize)) {
                trace("setSmallFontVista: success");
                return;
            }
        }
    }
    trace("setSmallFontVista: failure");
}

struct FontSizeComparator {
    bool operator()(const std::pair<DWORD, COORD> &obj1,
                    const std::pair<DWORD, COORD> &obj2) const {
        int score1 = obj1.second.X + obj1.second.Y;
        int score2 = obj2.second.X + obj2.second.Y;
        return score1 < score2;
    }
};

static void setSmallFontXP(UndocumentedXPFontAPI &api, HANDLE conout) {
    // Read the console font table and sort it from smallest to largest.
    std::vector<std::pair<DWORD, COORD> > table = readFontTable(api, conout);
    std::sort(table.begin(), table.end(), FontSizeComparator());
    for (size_t i = 0; i < table.size(); ++i) {
        // Skip especially narrow fonts to permit narrower terminals.
        if (table[i].second.X < 4) {
            continue;
        }
        trace("setSmallFontXP: setting font to %u",
            static_cast<unsigned>(table[i].first));
        if (!api.SetConsoleFont()(conout, table[i].first)) {
            trace("setSmallFontXP: SetConsoleFont call failed");
            continue;
        }
        AGENT_CONSOLE_FONT_INFO info;
        if (!api.GetCurrentConsoleFont()(conout, FALSE, &info)) {
            trace("setSmallFontXP: GetCurrentConsoleFont call failed");
            return;
        }
        if (info.nFont != table[i].first) {
            trace("setSmallFontXP: font was not set");
            dumpXPFont(api, conout, "setSmallFontXP: post-call font: ");
            continue;
        }
        trace("setSmallFontXP: success");
        return;
    }
    trace("setSmallFontXP: failure");
}

} // anonymous namespace

// A Windows console window can never be larger than the desktop window.  To
// maximize the possible size of the console in rows*cols, try to configure
// the console with a small font.  Unfortunately, we cannot make the font *too*
// small, because there is also a minimum window size in pixels.
void setSmallFont(HANDLE conout) {
    trace("setSmallFont: attempting to set a small font (CP=%u OutputCP=%u)",
        static_cast<unsigned>(GetConsoleCP()),
        static_cast<unsigned>(GetConsoleOutputCP()));
    VistaFontAPI vista;
    if (vista.valid()) {
        dumpVistaFont(vista, conout, "previous font: ");
        dumpFontTable(conout, "previous font table: ");
        setSmallFontVista(vista, conout);
        dumpVistaFont(vista, conout, "new font: ");
        dumpFontTable(conout, "new font table: ");
        return;
    }
    UndocumentedXPFontAPI xp;
    if (xp.valid()) {
        dumpXPFont(xp, conout, "previous font: ");
        dumpFontTable(conout, "previous font table: ");
        setSmallFontXP(xp, conout);
        dumpXPFont(xp, conout, "new font: ");
        dumpFontTable(conout, "new font table: ");
        return;
    }
    trace("setSmallFont: neither Vista nor XP APIs detected -- giving up");
    dumpFontTable(conout, "font table: ");
}
