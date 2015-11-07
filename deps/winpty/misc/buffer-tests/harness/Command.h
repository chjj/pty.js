#pragma once

#include "FixedSizeString.h"
#include "Spawn.h"

#include <array>
#include <cstdint>

struct Command {
    enum Kind {
        AllocConsole,
        AttachConsole,
        Close,
        CloseQuietly,
        DumpConsoleHandles,
        DumpStandardHandles,
        Duplicate,
        Exit,
        FreeConsole,
        GetConsoleProcessList,
        GetConsoleScreenBufferInfo,
        GetConsoleSelectionInfo,
        GetConsoleTitle,
        GetConsoleWindow,
        GetHandleInformation,
        GetNumberOfConsoleInputEvents,
        GetStdin,
        GetStderr,
        GetStdout,
        Hello,
        LookupKernelObject,
        NewBuffer,
        OpenConin,
        OpenConout,
        ReadConsoleOutput,
        ScanForConsoleHandles,
        SetConsoleTitle,
        SetHandleInformation,
        SetStdin,
        SetStderr,
        SetStdout,
        SetActiveBuffer,
        SpawnChild,
        System,
        WriteConsoleOutput,
        WriteText,
    };

    // These fields must appear first so that the LookupKernelObject RPC will
    // work.  This RPC occurs from 32-bit test programs to a 64-bit worker.
    // In that case, most of this struct's fields do not have the same
    // addresses or sizes.
    Kind kind;
    struct {
        uint32_t pid;
        uint32_t handle[2];
        uint32_t kernelObject[2];
    } lookupKernelObject;

    HANDLE handle;
    HANDLE targetProcess;
    DWORD dword;
    BOOL success;
    BOOL bInheritHandle;
    BOOL writeToEach;
    HWND hwnd;
    union {
        CONSOLE_SCREEN_BUFFER_INFO consoleScreenBufferInfo;
        CONSOLE_SELECTION_INFO consoleSelectionInfo;
        struct {
            FixedSizeString<128> spawnName;
            SpawnParams spawnParams;
            SpawnFailure spawnFailure;
        } spawn;
        FixedSizeString<1024> writeText;
        FixedSizeString<1024> systemText;
        std::array<wchar_t, 1024> consoleTitle;
        std::array<DWORD, 1024> processList;
        struct {
            DWORD mask;
            DWORD flags;
        } setFlags;
        struct {
            int count;
            std::array<HANDLE, 1024> table;
        } scanForConsoleHandles;
        struct {
            std::array<CHAR_INFO, 1024> buffer;
            COORD bufferSize;
            COORD bufferCoord;
            SMALL_RECT ioRegion;
        } consoleIo;
    } u;
};
