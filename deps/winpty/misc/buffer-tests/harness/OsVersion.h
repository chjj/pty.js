#pragma once

#include <windows.h>

#include <tuple>

#include <WinptyAssert.h>

inline std::tuple<int, int> osversion() {
    OSVERSIONINFOW info = { sizeof(info) };
    ASSERT(GetVersionExW(&info));
    return std::make_tuple(info.dwMajorVersion, info.dwMinorVersion);
}

inline bool isWorkstation() {
    OSVERSIONINFOEXW info = { sizeof(info) };
    ASSERT(GetVersionExW(reinterpret_cast<OSVERSIONINFO*>(&info)));
    return info.wProductType == VER_NT_WORKSTATION;
}

inline bool isWin7() {
    return osversion() == std::make_tuple(6, 1);
}

inline bool isAtLeastVista() {
    return osversion() >= std::make_tuple(6, 0);
}

inline bool isAtLeastWin7() {
    return osversion() >= std::make_tuple(6, 1);
}

inline bool isAtLeastWin8() {
    return osversion() >= std::make_tuple(6, 2);
}

inline bool isAtLeastWin8_1() {
    return osversion() >= std::make_tuple(6, 3);
}

inline bool isTraditionalConio() {
    return !isAtLeastWin8();
}

inline bool isModernConio() {
    return isAtLeastWin8();
}
