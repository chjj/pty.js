#pragma once

#include <windows.h>

#include <vector>

typedef struct _SYSTEM_HANDLE_ENTRY {
    ULONG OwnerPid;
    BYTE ObjectType;
    BYTE HandleFlags;
    USHORT HandleValue;
    PVOID ObjectPointer;
    ULONG AccessMask;
} SYSTEM_HANDLE_ENTRY, *PSYSTEM_HANDLE_ENTRY;

typedef struct _SYSTEM_HANDLE_INFORMATION {
    ULONG Count;
    SYSTEM_HANDLE_ENTRY Handle[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

std::vector<SYSTEM_HANDLE_ENTRY> queryNtHandles();
void *ntHandlePointer(const std::vector<SYSTEM_HANDLE_ENTRY> &table,
                      DWORD pid, HANDLE h);
