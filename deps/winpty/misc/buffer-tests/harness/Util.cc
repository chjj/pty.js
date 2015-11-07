#include "Util.h"

#include <windows.h>

#include <cstdint>
#include <sstream>
#include <string>

#include "UnicodeConversions.h"

#include <OsModule.h>
#include <WinptyAssert.h>

namespace {

static std::string timeString() {
    FILETIME fileTime;
    GetSystemTimeAsFileTime(&fileTime);
    auto ret = ((uint64_t)fileTime.dwHighDateTime << 32) |
                fileTime.dwLowDateTime;
    return std::to_string(ret);
}

} // anonymous namespace

std::string pathDirName(const std::string &path)
{
    std::string::size_type pos = path.find_last_of("\\/");
    if (pos == std::string::npos) {
        return std::string();
    } else {
        return path.substr(0, pos);
    }
}

// Wrapper for GetModuleFileNameW.  Returns a UTF-8 string.  Aborts on error.
std::string getModuleFileName(HMODULE module)
{
    const DWORD size = 4096;
    wchar_t filename[size];
    DWORD actual = GetModuleFileNameW(module, filename, size);
    ASSERT(actual > 0 && actual < size);
    return narrowString(filename);
}

// Convert GetLastError()'s error code to a presentable message such as:
//
//   <87:The parameter is incorrect.>
//
std::string errorString(DWORD errCode) {
    // MSDN has this note about "Windows 10":
    //
    //     Windows 10:
    //
    //     LocalFree is not in the modern SDK, so it cannot be used to free
    //     the result buffer. Instead, use HeapFree (GetProcessHeap(),
    //     allocatedMessage). In this case, this is the same as calling
    //     LocalFree on memory.
    //
    //     Important: LocalAlloc() has different options: LMEM_FIXED, and
    //     LMEM_MOVABLE. FormatMessage() uses LMEM_FIXED, so HeapFree can be
    //     used. If LMEM_MOVABLE is used, HeapFree cannot be used.
    //
    // My interpretation of this note is:
    //  * "Windows 10" really just means, "the latest MS SDK", which supports
    //    Windows 10, as well as older releases.
    //  * In every NT kernel ever, HeapFree is perfectly fine to use with
    //    LocalAlloc LMEM_FIXED allocations.
    //  * In every NT kernel ever, the FormatMessage buffer can be freed with
    //    HeapFree.
    // The note is clumsy, though.  Without clarity, I can't safely use
    // HeapFree, but apparently LocalFree calls stop compiling in the newest
    // SDK.
    //
    // Instead, I'll use a fixed-size buffer.

    std::stringstream ss;
    ss << "<" << errCode << ":";
    std::vector<wchar_t> msgBuf(1024);
    DWORD ret = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        msgBuf.data(),
        msgBuf.size(),
        nullptr);
    if (ret == 0) {
        ss << "FormatMessageW failed:";
        ss << GetLastError();
    } else {
        msgBuf[msgBuf.size() - 1] = L'\0';
        std::string msg = narrowString(std::wstring(msgBuf.data()));
        if (msg.size() >= 2 && msg.substr(msg.size() - 2) == "\r\n") {
            msg.resize(msg.size() - 2);
        }
        ss << msg;
    }
    ss << ">";
    return ss.str();
}

bool isWow64() {
    static bool valueInitialized = false;
    static bool value = false;
    if (!valueInitialized) {
        OsModule kernel32(L"kernel32.dll");
        auto proc = reinterpret_cast<decltype(IsWow64Process)*>(
            kernel32.proc("IsWow64Process"));
        BOOL isWow64 = FALSE;
        BOOL ret = proc(GetCurrentProcess(), &isWow64);
        value = ret && isWow64;
        valueInitialized = true;
    }
    return value;
}

std::string makeTempName(const std::string &baseName) {
    static int workerCounter = 0;
    static auto initialTimeString = timeString();
    return baseName + "-" +
        std::to_string(static_cast<int>(GetCurrentProcessId())) + "-" +
        initialTimeString + "-" +
        std::to_string(++workerCounter);
}
