#pragma once

#include <windows.h>

#include <string>

#include "RemoteHandle.h"

struct SpawnParams {
    BOOL bInheritHandles = FALSE;
    DWORD dwCreationFlags = 0;
    STARTUPINFOW sui = { sizeof(STARTUPINFOW), 0 };
    static const size_t NoInheritList = static_cast<size_t>(~0);
    size_t inheritCount = NoInheritList;
    std::array<HANDLE, 1024> inheritList = {};
    bool nativeWorkerBitness = false;

    SpawnParams(bool bInheritHandles=false, DWORD dwCreationFlags=0) :
        bInheritHandles(bInheritHandles),
        dwCreationFlags(dwCreationFlags)
    {
    }

    SpawnParams(bool bInheritHandles, DWORD dwCreationFlags,
                std::vector<RemoteHandle> stdHandles) :
        bInheritHandles(bInheritHandles),
        dwCreationFlags(dwCreationFlags)
    {
        ASSERT(stdHandles.size() == 3);
        sui.dwFlags |= STARTF_USESTDHANDLES;
        sui.hStdInput = stdHandles[0].value();
        sui.hStdOutput = stdHandles[1].value();
        sui.hStdError = stdHandles[2].value();
    }
};

struct SpawnFailure {
    enum Kind { Success, CreateProcess, UpdateProcThreadAttribute };
    Kind kind = Success;
    DWORD errCode = 0;
};

HANDLE spawn(const std::string &workerName,
             const SpawnParams &params,
             SpawnFailure &error);
