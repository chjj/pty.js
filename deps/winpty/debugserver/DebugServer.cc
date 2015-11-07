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

#include <cstdio>
#include <cstdlib>

#include <windows.h>

// A message may not be larger than this size.
const int MSG_SIZE = 4096;

int main() {
    HANDLE serverPipe = CreateNamedPipeW(
        L"\\\\.\\pipe\\DebugServer",
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
        PIPE_UNLIMITED_INSTANCES,
        MSG_SIZE,
        MSG_SIZE,
        10 * 1000,
        NULL);

    char msgBuffer[MSG_SIZE];

    while (true) {
        if (!ConnectNamedPipe(serverPipe, NULL)) {
            fprintf(stderr, "Error: ConnectNamedPipe failed\n");
            fflush(stderr);
            exit(1);
        }
        DWORD bytesRead = 0;
        if (!ReadFile(serverPipe, msgBuffer, MSG_SIZE, &bytesRead, NULL)) {
            fprintf(stderr, "Error: ReadFile on pipe failed\n");
            fflush(stderr);
            DisconnectNamedPipe(serverPipe);
            continue;
        }
        msgBuffer[bytesRead] = '\0';
        printf("%s\n", msgBuffer);
        fflush(stdout);

        DWORD bytesWritten = 0;
        WriteFile(serverPipe, "OK", 2, &bytesWritten, NULL);
        DisconnectNamedPipe(serverPipe);
    }
}
