#include "LargeConsoleRead.h"

#include <stdlib.h>

#include "Agent.h"
#include "Win32Console.h"

namespace {

// Returns true if the OS version is at least Windows 8 (6.2).  This function
// should work correctly regardless of how the executable is manifested.
static bool isWindows8OrGreater() {
    OSVERSIONINFO info = {0};
    info.dwOSVersionInfoSize = sizeof(info);
    if (!GetVersionEx(&info)) {
        return false;
    }
    if (info.dwMajorVersion > 6) {
        return true;
    }
    if (info.dwMajorVersion < 6) {
        return false;
    }
    return info.dwMinorVersion >= 2;
}

} // anonymous namespace

LargeConsoleReadBuffer::LargeConsoleReadBuffer() :
    m_rect(0, 0, 0, 0), m_rectWidth(0)
{
}

void largeConsoleRead(LargeConsoleReadBuffer &out,
                      Win32Console &console,
                      const SmallRect &readArea) {
    ASSERT(readArea.Left >= 0 &&
           readArea.Top >= 0 &&
           readArea.Right >= readArea.Left &&
           readArea.Bottom >= readArea.Top &&
           readArea.width() <= MAX_CONSOLE_WIDTH);
    const size_t count = readArea.width() * readArea.height();
    if (out.m_data.size() < count) {
        out.m_data.resize(count);
    }
    out.m_rect = readArea;
    out.m_rectWidth = readArea.width();

    static const bool useLargeReads = isWindows8OrGreater();
    if (useLargeReads) {
        console.read(readArea, out.m_data.data());
    } else {
        const int maxReadLines = std::max(1, MAX_CONSOLE_WIDTH / readArea.width());
        int curLine = readArea.Top;
        while (curLine <= readArea.Bottom) {
            const SmallRect subReadArea(
                readArea.Left,
                curLine,
                readArea.width(),
                std::min(maxReadLines, readArea.Bottom + 1 - curLine));
            console.read(subReadArea, out.lineDataMut(curLine));
            curLine = subReadArea.Bottom + 1;
        }
    }
}
