#include <windows.h>

#include <stdint.h>
#include <stdio.h>

#include <algorithm>
#include <vector>

#include "Command.h"
#include "Event.h"
#include "NtHandleQuery.h"
#include "OsVersion.h"
#include "ShmemParcel.h"
#include "Spawn.h"

#include <DebugClient.h>

static const char *g_prefix = "";

static const char *successOrFail(BOOL ret) {
    return ret ? "ok" : "FAILED";
}

static HANDLE openConHandle(const wchar_t *name, BOOL bInheritHandle) {
    // If sa isn't provided, the handle defaults to not-inheritable.
    SECURITY_ATTRIBUTES sa = {0};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = bInheritHandle;

    trace("%sOpening %ls...", g_prefix, name);
    HANDLE conout = CreateFileW(name,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                &sa,
                OPEN_EXISTING, 0, NULL);
    trace("%sOpening %ls... 0x%I64x", g_prefix, name, (int64_t)conout);
    return conout;
}

static HANDLE createBuffer(BOOL bInheritHandle) {
    // If sa isn't provided, the handle defaults to not-inheritable.
    SECURITY_ATTRIBUTES sa = {0};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = bInheritHandle;

    trace("%sCreating a new buffer...", g_prefix);
    HANDLE conout = CreateConsoleScreenBuffer(
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                &sa,
                CONSOLE_TEXTMODE_BUFFER, NULL);

    trace("%sCreating a new buffer... 0x%I64x", g_prefix, (int64_t)conout);
    return conout;
}

static void writeTest(HANDLE conout, const char *msg) {
    char writeData[256];
    sprintf(writeData, "%s%s\n", g_prefix, msg);

    trace("%sWriting to 0x%I64x: '%s'...",
        g_prefix, (int64_t)conout, msg);
    DWORD actual = 0;
    BOOL ret = WriteConsoleA(conout, writeData, strlen(writeData), &actual, NULL);
    trace("%sWriting to 0x%I64x: '%s'... %s",
        g_prefix, (int64_t)conout, msg,
        successOrFail(ret && actual == strlen(writeData)));
}

static void setConsoleActiveScreenBuffer(HANDLE conout) {
    trace("SetConsoleActiveScreenBuffer(0x%I64x) called...",
        (int64_t)conout);
    trace("SetConsoleActiveScreenBuffer(0x%I64x) called... %s",
        (int64_t)conout,
        successOrFail(SetConsoleActiveScreenBuffer(conout)));
}

static void dumpStandardHandles() {
    trace("stdin=0x%I64x stdout=0x%I64x stderr=0x%I64x",
        (int64_t)GetStdHandle(STD_INPUT_HANDLE),
        (int64_t)GetStdHandle(STD_OUTPUT_HANDLE),
        (int64_t)GetStdHandle(STD_ERROR_HANDLE));
}

static std::vector<HANDLE> scanForConsoleHandles() {
    std::vector<HANDLE> ret;
    if (isModernConio()) {
        // As of Windows 8, console handles are real kernel handles.
        for (unsigned int i = 0x4; i <= 0x1000; i += 4) {
            HANDLE h = reinterpret_cast<HANDLE>(i);
            DWORD mode;
            if (GetConsoleMode(h, &mode)) {
                ret.push_back(h);
            }
        }
    } else {
        for (unsigned int i = 0x3; i < 0x3 + 100 * 4; i += 4) {
            HANDLE h = reinterpret_cast<HANDLE>(i);
            DWORD mode;
            if (GetConsoleMode(h, &mode)) {
                ret.push_back(h);
            }
        }
    }
    return ret;
}

static void dumpConsoleHandles(bool writeToEach) {
    std::string dumpLine = "";
    for (HANDLE h : scanForConsoleHandles()) {
        char buf[32];
        sprintf(buf, "0x%I64x", (int64_t)h);
        dumpLine += buf;
        dumpLine.push_back('(');
        CONSOLE_SCREEN_BUFFER_INFO info;
        bool is_output = false;
        DWORD count;
        if (GetNumberOfConsoleInputEvents(h, &count)) {
            dumpLine.push_back('I');
        }
        if (GetConsoleScreenBufferInfo(h, &info)) {
            is_output = true;
            dumpLine.push_back('O');
            CHAR_INFO charInfo;
            SMALL_RECT readRegion = {};
            if (ReadConsoleOutputW(h, &charInfo, {1,1}, {0,0}, &readRegion)) {
                wchar_t ch = charInfo.Char.UnicodeChar;
                if (ch != L' ') {
                    dumpLine.push_back((char)ch);
                }
            }
        }
        {
            DWORD flags = 0;
            if (GetHandleInformation(h, &flags)) {
                dumpLine.push_back((flags & HANDLE_FLAG_INHERIT) ? '^' : '_');
            }
        }
        dumpLine += ") ";
        if (writeToEach && is_output) {
            char msg[256];
            sprintf(msg, "%d: Writing to 0x%I64x",
                (int)GetCurrentProcessId(), (int64_t)h);
            writeTest(h, msg);
        }
    }
    trace("Valid console handles:%s", dumpLine.c_str());
}

template <typename T>
void handleConsoleIoCommand(Command &cmd, T func) {
    const auto sz = cmd.u.consoleIo.bufferSize;
    ASSERT(static_cast<size_t>(sz.X) * sz.Y <= cmd.u.consoleIo.buffer.size());
    cmd.success = func(cmd.handle, cmd.u.consoleIo.buffer.data(),
        cmd.u.consoleIo.bufferSize, cmd.u.consoleIo.bufferCoord,
        &cmd.u.consoleIo.ioRegion);
}

int main(int argc, char *argv[]) {
    std::string workerName = argv[1];

    ShmemParcelTyped<Command> parcel(workerName + "-shmem", ShmemParcel::OpenExisting);
    Event startEvent(workerName + "-start");
    Event finishEvent(workerName + "-finish");
    Command &cmd = parcel.value();

    dumpStandardHandles();

    while (true) {
        startEvent.wait();
        startEvent.reset();
        switch (cmd.kind) {
            case Command::AllocConsole:
                trace("Calling AllocConsole...");
                cmd.success = AllocConsole();
                trace("Calling AllocConsole... %s",
                    successOrFail(cmd.success));
                break;
            case Command::AttachConsole:
                trace("Calling AttachConsole(%u)...",
                    (unsigned int)cmd.dword);
                cmd.success = AttachConsole(cmd.dword);
                trace("Calling AttachConsole(%u)... %s",
                    (unsigned int)cmd.dword, successOrFail(cmd.success));
                break;
            case Command::Close:
                trace("closing 0x%I64x...",
                    (int64_t)cmd.handle);
                cmd.success = CloseHandle(cmd.handle);
                trace("closing 0x%I64x... %s",
                    (int64_t)cmd.handle, successOrFail(cmd.success));
                break;
            case Command::CloseQuietly:
                cmd.success = CloseHandle(cmd.handle);
                break;
            case Command::DumpStandardHandles:
                dumpStandardHandles();
                break;
            case Command::DumpConsoleHandles:
                dumpConsoleHandles(cmd.writeToEach);
                break;
            case Command::Duplicate: {
                HANDLE sourceHandle = cmd.handle;
                cmd.success = DuplicateHandle(
                    GetCurrentProcess(),
                    sourceHandle,
                    cmd.targetProcess,
                    &cmd.handle,
                    0, cmd.bInheritHandle, DUPLICATE_SAME_ACCESS);
                if (!cmd.success) {
                    cmd.handle = INVALID_HANDLE_VALUE;
                }
                trace("dup 0x%I64x to pid %u... %s, 0x%I64x",
                    (int64_t)sourceHandle,
                    (unsigned int)GetProcessId(cmd.targetProcess),
                    successOrFail(cmd.success),
                    (int64_t)cmd.handle);
                break;
            }
            case Command::Exit:
                trace("exiting");
                ExitProcess(cmd.dword);
                break;
            case Command::FreeConsole:
                trace("Calling FreeConsole...");
                cmd.success = FreeConsole();
                trace("Calling FreeConsole... %s", successOrFail(cmd.success));
                break;
            case Command::GetConsoleProcessList:
                cmd.dword = GetConsoleProcessList(cmd.u.processList.data(),
                                                  cmd.u.processList.size());
                break;
            case Command::GetConsoleScreenBufferInfo:
                cmd.u.consoleScreenBufferInfo = {};
                cmd.success = GetConsoleScreenBufferInfo(
                    cmd.handle, &cmd.u.consoleScreenBufferInfo);
                break;
            case Command::GetConsoleSelectionInfo:
                cmd.u.consoleSelectionInfo = {};
                cmd.success = GetConsoleSelectionInfo(&cmd.u.consoleSelectionInfo);
                break;
            case Command::GetConsoleTitle:
                // GetConsoleTitle is buggy, so make the worker API for it very
                // explicit so we can test its bugginess.
                ASSERT(cmd.dword <= cmd.u.consoleTitle.size());
                cmd.dword = GetConsoleTitleW(cmd.u.consoleTitle.data(), cmd.dword);
                break;
            case Command::GetConsoleWindow:
                cmd.hwnd = GetConsoleWindow();
                break;
            case Command::GetHandleInformation:
                cmd.success = GetHandleInformation(cmd.handle, &cmd.dword);
                break;
            case Command::GetNumberOfConsoleInputEvents:
                cmd.success = GetNumberOfConsoleInputEvents(cmd.handle, &cmd.dword);
                break;
            case Command::GetStdin:
                cmd.handle = GetStdHandle(STD_INPUT_HANDLE);
                break;
            case Command::GetStderr:
                cmd.handle = GetStdHandle(STD_ERROR_HANDLE);
                break;
            case Command::GetStdout:
                cmd.handle = GetStdHandle(STD_OUTPUT_HANDLE);
                break;
            case Command::Hello:
                // NOOP for Worker startup synchronization.
                break;
            case Command::LookupKernelObject: {
                uint64_t h64;
                memcpy(&h64, &cmd.lookupKernelObject.handle, sizeof(h64));
                auto handles = queryNtHandles();
                uint64_t result =
                    reinterpret_cast<uint64_t>(
                        ntHandlePointer(
                            handles, cmd.lookupKernelObject.pid,
                                reinterpret_cast<HANDLE>(h64)));
                memcpy(&cmd.lookupKernelObject.kernelObject,
                       &result, sizeof(result));
                trace("LOOKUP: p%d: 0x%I64x => 0x%I64x",
                    (int)cmd.lookupKernelObject.pid,
                    h64,
                    result);
                break;
            }
            case Command::NewBuffer:
                cmd.handle = createBuffer(cmd.bInheritHandle);
                break;
            case Command::OpenConin:
                cmd.handle = openConHandle(L"CONIN$", cmd.bInheritHandle);
                break;
            case Command::OpenConout:
                cmd.handle = openConHandle(L"CONOUT$", cmd.bInheritHandle);
                break;
            case Command::ReadConsoleOutput:
                handleConsoleIoCommand(cmd, ReadConsoleOutputW);
                break;
            case Command::ScanForConsoleHandles: {
                auto ret = scanForConsoleHandles();
                ASSERT(ret.size() <= cmd.u.scanForConsoleHandles.table.size());
                cmd.u.scanForConsoleHandles.count = ret.size();
                std::copy(ret.begin(), ret.end(),
                          cmd.u.scanForConsoleHandles.table.begin());
                break;
            }
            case Command::SetConsoleTitle: {
                auto nul = std::find(cmd.u.consoleTitle.begin(),
                                     cmd.u.consoleTitle.end(), L'\0');
                ASSERT(nul != cmd.u.consoleTitle.end());
                cmd.success = SetConsoleTitleW(cmd.u.consoleTitle.data());
                break;
            }
            case Command::SetHandleInformation:
                cmd.success = SetHandleInformation(
                    cmd.handle, cmd.u.setFlags.mask, cmd.u.setFlags.flags);
                break;
            case Command::SetStdin:
                SetStdHandle(STD_INPUT_HANDLE, cmd.handle);
                trace("setting stdin to 0x%I64x", (int64_t)cmd.handle);
                break;
            case Command::SetStderr:
                SetStdHandle(STD_ERROR_HANDLE, cmd.handle);
                trace("setting stderr to 0x%I64x", (int64_t)cmd.handle);
                break;
            case Command::SetStdout:
                SetStdHandle(STD_OUTPUT_HANDLE, cmd.handle);
                trace("setting stdout to 0x%I64x", (int64_t)cmd.handle);
                break;
            case Command::SetActiveBuffer:
                setConsoleActiveScreenBuffer(cmd.handle);
                break;
            case Command::SpawnChild:
                trace("Spawning child...");
                cmd.handle = spawn(cmd.u.spawn.spawnName.str(),
                                   cmd.u.spawn.spawnParams,
                                   cmd.u.spawn.spawnFailure);
                if (cmd.handle != nullptr) {
                    trace("Spawning child... pid %u",
                        (unsigned int)GetProcessId(cmd.handle));
                }
                break;
            case Command::System:
                cmd.dword = system(cmd.u.systemText.c_str());
                break;
            case Command::WriteConsoleOutput:
                handleConsoleIoCommand(cmd, WriteConsoleOutputW);
                break;
            case Command::WriteText:
                writeTest(cmd.handle, cmd.u.writeText.c_str());
                break;
        }
        finishEvent.set();
    }
    return 0;
}
