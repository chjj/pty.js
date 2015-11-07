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

#include <windows.h>

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <vector>

#include "../include/winpty.h"
#include "../shared/DebugClient.h"

// Create a manual reset, initially unset event.
static HANDLE createEvent() {
    return CreateEvent(NULL, TRUE, FALSE, NULL);
}

static std::vector<unsigned char> filterContent(
        const std::vector<unsigned char> &content) {
    std::vector<unsigned char> result;
    auto it = content.begin();
    const auto itEnd = content.end();
    while (it < itEnd) {
        if (*it == '\r') {
            // Filter out carriage returns.  Sometimes the output starts with
            // a single CR; other times, it has multiple CRs.
            it++;
        } else if (*it == '\x1b' && (it + 1) < itEnd && *(it + 1) == '[') {
            // Filter out escape sequences.  They have no interior letters and
            // end with a single letter.
            it += 2;
            while (it < itEnd && !isalpha(*it)) {
                it++;
            }
            it++;
        } else {
            // Let everything else through.
            result.push_back(*it);
            it++;
        }
    }
    return result;
}

// Read bytes from the overlapped file handle until the file is closed or
// until an I/O error occurs.
static std::vector<unsigned char> readAll(HANDLE handle) {
    const HANDLE event = createEvent();
    unsigned char buf[1024];
    std::vector<unsigned char> result;
    while (true) {
        OVERLAPPED over;
        memset(&over, 0, sizeof(over));
        over.hEvent = event;
        DWORD amount = 0;
        BOOL ret = ReadFile(handle, buf, sizeof(buf), &amount, &over);
        if (!ret && GetLastError() == ERROR_IO_PENDING)
            ret = GetOverlappedResult(handle, &over, &amount, TRUE);
        if (!ret || amount == 0)
            break;
        result.insert(result.end(), buf, buf + amount);
    }
    CloseHandle(event);
    return result;
}

static void parentTest() {
    wchar_t program[1024];
    wchar_t cmdline[1024];
    GetModuleFileNameW(NULL, program, 1024);
    snwprintf(cmdline, sizeof(cmdline) / sizeof(cmdline[0]),
              L"\"%s\" CHILD", program);

    winpty_t *pty = winpty_open(80, 25);
    assert(pty != NULL);
    int ret = winpty_start_process(pty, program, cmdline, NULL, NULL);
    assert(ret == 0);

    HANDLE input = winpty_get_data_pipe(pty);
    auto content = readAll(input);
    content = filterContent(content);

    std::vector<unsigned char> expectedContent = {
        'H', 'I', '\n', 'X', 'Y', '\n'
    };
    assert(winpty_get_exit_code(pty) == 42);
    assert(content == expectedContent);
    winpty_close(pty);
}

static void childTest() {
    printf("HI\nXY\n");
    exit(42);
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        parentTest();
    } else {
        childTest();
    }
    return 0;
}
