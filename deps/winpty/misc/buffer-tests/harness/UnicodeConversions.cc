#include "UnicodeConversions.h"

#include <windows.h>

#include <vector>

#include <WinptyAssert.h>

std::string narrowString(const std::wstring &input)
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

std::wstring widenString(const std::string &input)
{
    int widelen = MultiByteToWideChar(
        CP_UTF8, 0,
        input.data(), input.size(),
        NULL, 0);
    if (widelen <= 0) {
        return std::wstring();
    }
    std::vector<wchar_t> tmp(widelen);
    int widelen2 = MultiByteToWideChar(
        CP_UTF8, 0,
        input.data(), input.size(),
        tmp.data(), tmp.size());
    ASSERT(widelen2 == widelen);
    return std::wstring(tmp.data(), tmp.size());
}
