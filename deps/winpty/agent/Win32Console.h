// Copyright (c) 2011-2012 Ryan Prichard
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

#ifndef WIN32CONSOLE_H
#define WIN32CONSOLE_H

#include <windows.h>
#include <wchar.h>

#include <string>
#include <vector>

#include "Coord.h"
#include "SmallRect.h"

struct ConsoleScreenBufferInfo : CONSOLE_SCREEN_BUFFER_INFO
{
    ConsoleScreenBufferInfo()
    {
        memset(this, 0, sizeof(*this));
    }

    Coord bufferSize() const        { return dwSize;    }
    SmallRect windowRect() const    { return srWindow;  }
    Coord cursorPosition() const    { return dwCursorPosition; }
};

class Win32Console
{
public:
    Win32Console();
    ~Win32Console();

    HANDLE conin();
    HANDLE conout();
    HWND hwnd();
    void postCloseMessage();
    void clearLines(int row, int count, const ConsoleScreenBufferInfo &info);
    void clearAllLines(const ConsoleScreenBufferInfo &info);

    // Buffer and window sizes.
    ConsoleScreenBufferInfo bufferInfo();
    Coord bufferSize();
    SmallRect windowRect();
    void resizeBuffer(const Coord &size);
    void moveWindow(const SmallRect &rect);

    // Cursor.
    Coord cursorPosition();
    void setCursorPosition(const Coord &point);

    // Input stream.
    void writeInput(const INPUT_RECORD *ir, int count=1);
    bool processedInputMode();

    // Screen content.
    void read(const SmallRect &rect, CHAR_INFO *data);
    void write(const SmallRect &rect, const CHAR_INFO *data);

    // Title.
    std::wstring title();
    void setTitle(const std::wstring &title);

    void setTextAttribute(WORD attributes);

private:
    HANDLE m_conin;
    HANDLE m_conout;
    std::vector<wchar_t> m_titleWorkBuf;
};

#endif // WIN32CONSOLE_H
