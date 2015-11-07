#include "NtHandleQuery.h"

#include <DebugClient.h>
#include <OsModule.h>

// internal definitions copied from mingw-w64's winternl.h and ntstatus.h.

#define STATUS_SUCCESS ((NTSTATUS)0x00000000)
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004)

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation = 0,
    SystemProcessorInformation = 1,
    SystemPerformanceInformation = 2,
    SystemTimeOfDayInformation = 3,
    SystemProcessInformation = 5,
    SystemProcessorPerformanceInformation = 8,
    SystemHandleInformation = 16,
    SystemPagefileInformation = 18,
    SystemInterruptInformation = 23,
    SystemExceptionInformation = 33,
    SystemRegistryQuotaInformation = 37,
    SystemLookasideInformation = 45
} SYSTEM_INFORMATION_CLASS;

typedef NTSTATUS NTAPI NtQuerySystemInformation_Type(
    SYSTEM_INFORMATION_CLASS SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength);

std::vector<SYSTEM_HANDLE_ENTRY> queryNtHandles() {
    OsModule ntdll(L"ntdll.dll");
    auto funcPtr = ntdll.proc("NtQuerySystemInformation");
    ASSERT(funcPtr != NULL && "NtQuerySystemInformation API is missing");
    auto func = reinterpret_cast<NtQuerySystemInformation_Type*>(funcPtr);
    static std::vector<char> buf(1024);
    while (true) {
        ULONG returnLength = 0;
        auto ret = func(
            SystemHandleInformation,
            buf.data(),
            buf.size(),
            &returnLength);
        if (ret == STATUS_INFO_LENGTH_MISMATCH) {
            buf.resize(buf.size() * 2);
            continue;
        } else if (ret == STATUS_SUCCESS) {
            break;
        } else {
            trace("Could not query NT handles, status was 0x%x",
                static_cast<unsigned>(ret));
            return {};
        }
    }
    auto &info = *reinterpret_cast<SYSTEM_HANDLE_INFORMATION*>(buf.data());
    std::vector<SYSTEM_HANDLE_ENTRY> ret(info.Count);
    std::copy(info.Handle, info.Handle + info.Count, ret.begin());
    return ret;
}

// Get the ObjectPointer (underlying NT object) for the NT handle.
void *ntHandlePointer(const std::vector<SYSTEM_HANDLE_ENTRY> &table,
                      DWORD pid, HANDLE h) {
    HANDLE ret = nullptr;
    for (auto &entry : table) {
        if (entry.OwnerPid == pid &&
                entry.HandleValue == reinterpret_cast<uint64_t>(h)) {
            ASSERT(ret == nullptr);
            ret = entry.ObjectPointer;
        }
    }
    return ret;
}
