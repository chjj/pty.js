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

#ifndef AGENT_H
#define AGENT_H

#include <windows.h>

#include <string>
#include <vector>

#include "EventLoop.h"
#include "DsrSender.h"
#include "Coord.h"
#include "SmallRect.h"
#include "ConsoleLine.h"
#include "Terminal.h"
#include "LargeConsoleRead.h"

class Win32Console;
class ConsoleInput;
class ReadBuffer;
class NamedPipe;
struct ConsoleScreenBufferInfo;

// We must be able to issue a single ReadConsoleOutputW call of
// MAX_CONSOLE_WIDTH characters, and a single read of approximately several
// hundred fewer characters than BUFFER_LINE_COUNT.
const int BUFFER_LINE_COUNT = 3000;
const int MAX_CONSOLE_WIDTH = 2500;
const int SYNC_MARKER_LEN = 16;

class Agent : public EventLoop, public DsrSender
{
public:
    Agent(LPCWSTR controlPipeName,
          LPCWSTR dataPipeName,
          int initialCols,
          int initialRows);
    virtual ~Agent();
    void sendDsr();

private:
    NamedPipe *makeSocket(LPCWSTR pipeName);
    void resetConsoleTracking(
        Terminal::SendClearFlag sendClear, const SmallRect &windowRect);

private:
    void pollControlSocket();
    void handlePacket(ReadBuffer &packet);
    int handleStartProcessPacket(ReadBuffer &packet);
    int handleSetSizePacket(ReadBuffer &packet);
    void pollDataSocket();

protected:
    virtual void onPollTimeout();
    virtual void onPipeIo(NamedPipe *namedPipe);

private:
    void markEntireWindowDirty(const SmallRect &windowRect);
    void scanForDirtyLines(const SmallRect &windowRect);
    void clearBufferLines(int firstRow, int count, WORD attributes);
    void resizeImpl(const ConsoleScreenBufferInfo &origInfo);
    void resizeWindow(int cols, int rows);
    void syncConsoleContentAndSize(bool forceResize);
    void syncConsoleTitle();
    void directScrapeOutput(const ConsoleScreenBufferInfo &info);
    void scrollingScrapeOutput(const ConsoleScreenBufferInfo &info);
    void reopenConsole();
    void freezeConsole();
    void unfreezeConsole();
    void syncMarkerText(CHAR_INFO (&output)[SYNC_MARKER_LEN]);
    int findSyncMarker();
    void createSyncMarker(int row);

private:
    bool m_useMark;
    Win32Console *m_console;
    NamedPipe *m_controlSocket;
    NamedPipe *m_dataSocket;
    bool m_closingDataSocket;
    Terminal *m_terminal;
    ConsoleInput *m_consoleInput;
    HANDLE m_childProcess;
    int m_childExitCode;

    int m_syncRow;
    int m_syncCounter;

    bool m_directMode;
    Coord m_ptySize;
    int m_scrapedLineCount;
    int m_scrolledCount;
    int m_maxBufferedLine;
    LargeConsoleReadBuffer m_readBuffer;
    std::vector<ConsoleLine> m_bufferData;
    int m_dirtyWindowTop;
    int m_dirtyLineCount;

    std::wstring m_currentTitle;
};

#endif // AGENT_H
