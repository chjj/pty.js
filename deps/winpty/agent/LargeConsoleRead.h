#ifndef LARGE_CONSOLE_READ_H
#define LARGE_CONSOLE_READ_H

#include <windows.h>
#include <stdlib.h>

#include <vector>

#include "SmallRect.h"
#include "../shared/DebugClient.h"
#include "../shared/WinptyAssert.h"

class Win32Console;

class LargeConsoleReadBuffer {
public:
    LargeConsoleReadBuffer();
    const SmallRect &rect() const { return m_rect; }
    const CHAR_INFO *lineData(int line) const {
        validateLineNumber(line);
        return &m_data[(line - m_rect.Top) * m_rectWidth];
    }

private:
    CHAR_INFO *lineDataMut(int line) {
        validateLineNumber(line);
        return &m_data[(line - m_rect.Top) * m_rectWidth];
    }

    void validateLineNumber(int line) const {
        if (line < m_rect.Top || line > m_rect.Bottom) {
            trace("Fatal error: LargeConsoleReadBuffer: invalid line %d for "
                  "read rect %s", line, m_rect.toString().c_str());
            abort();
        }
    }

    SmallRect m_rect;
    int m_rectWidth;
    std::vector<CHAR_INFO> m_data;

    friend void largeConsoleRead(LargeConsoleReadBuffer &out,
                                 Win32Console &console,
                                 const SmallRect &readArea);
};

#endif // LARGE_CONSOLE_READ_H
