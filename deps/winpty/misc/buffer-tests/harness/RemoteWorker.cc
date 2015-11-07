#include "RemoteWorker.h"

#include <string>

#include "UnicodeConversions.h"
#include "Util.h"

#include <DebugClient.h>
#include <WinptyAssert.h>

DWORD RemoteWorker::dwDefaultCreationFlags = CREATE_NEW_CONSOLE;

RemoteWorker::RemoteWorker(decltype(DoNotSpawn)) :
    m_name(makeTempName("WinptyBufferTests")),
    m_parcel(m_name + "-shmem", ShmemParcel::CreateNew),
    m_startEvent(m_name + "-start"),
    m_finishEvent(m_name + "-finish")
{
    m_finishEvent.set();
}

RemoteWorker::RemoteWorker(SpawnParams params) : RemoteWorker(DoNotSpawn) {
    SpawnFailure dummy;
    m_process = spawn(m_name, params, dummy);
    ASSERT(m_process != nullptr && "Could not create RemoteWorker");
    m_valid = true;
    // Perform an RPC just to ensure that the worker process is ready, and
    // the console exists, before returning.
    rpc(Command::Hello);
}

RemoteWorker RemoteWorker::child(SpawnParams params) {
    auto ret = tryChild(params);
    ASSERT(ret.valid() && "Could not spawn child worker");
    return ret;
}

RemoteWorker RemoteWorker::tryChild(SpawnParams params, SpawnFailure *failure) {
    RemoteWorker ret(DoNotSpawn);
    cmd().u.spawn.spawnName = ret.m_name;
    cmd().u.spawn.spawnParams = params;
    rpc(Command::SpawnChild);
    if (cmd().handle == nullptr) {
        if (failure != nullptr) {
            *failure = cmd().u.spawn.spawnFailure;
        }
    } else {
        BOOL dupSuccess = DuplicateHandle(
            m_process,
            cmd().handle,
            GetCurrentProcess(),
            &ret.m_process,
            0, FALSE, DUPLICATE_SAME_ACCESS);
        ASSERT(dupSuccess && "RemoteWorker::child: DuplicateHandle failed");
        rpc(Command::CloseQuietly);
        ASSERT(cmd().success && "RemoteWorker::child: CloseHandle failed");
        ret.m_valid = true;
        // Perform an RPC just to ensure that the worker process is ready, and
        // the console exists, before returning.
        ret.rpc(Command::Hello);
    }
    return ret;
}

void RemoteWorker::exit() {
    cmd().dword = 0;
    rpcAsync(Command::Exit);
    DWORD result = WaitForSingleObject(m_process, INFINITE);
    ASSERT(result == WAIT_OBJECT_0 &&
        "WaitForSingleObject failed while killing worker");
    CloseHandle(m_process);
    m_valid = false;
}

CONSOLE_SELECTION_INFO RemoteWorker::selectionInfo() {
    rpc(Command::GetConsoleSelectionInfo);
    ASSERT(cmd().success);
    return cmd().u.consoleSelectionInfo;
}

void RemoteWorker::dumpConsoleHandles(BOOL writeToEach) {
    cmd().writeToEach = writeToEach;
    rpc(Command::DumpConsoleHandles);
}

std::vector<RemoteHandle> RemoteWorker::scanForConsoleHandles() {
    rpc(Command::ScanForConsoleHandles);
    auto &rpcTable = cmd().u.scanForConsoleHandles;
    std::vector<RemoteHandle> ret;
    for (int i = 0; i < rpcTable.count; ++i) {
        ret.push_back(RemoteHandle(rpcTable.table[i], *this));
    }
    return ret;
}

bool RemoteWorker::setTitleInternal(const std::wstring &wstr) {
    ASSERT(wstr.size() < cmd().u.consoleTitle.size());
    ASSERT(wstr.size() == wcslen(wstr.c_str()));
    wcscpy(cmd().u.consoleTitle.data(), wstr.c_str());
    rpc(Command::SetConsoleTitle);
    return cmd().success;
}

std::string RemoteWorker::title() {
    std::array<wchar_t, 1024> buf;
    DWORD ret = titleInternal(buf, buf.size());
    ret = std::min<DWORD>(ret, buf.size() - 1);
    buf[std::min<size_t>(buf.size() - 1, ret)] = L'\0';
    return narrowString(std::wstring(buf.data()));
}

// This API is more low-level than typical, because GetConsoleTitleW is buggy
// in older versions of Windows, and this method is used to test the bugs.
DWORD RemoteWorker::titleInternal(std::array<wchar_t, 1024> &buf, DWORD bufSize) {
    cmd().dword = bufSize;
    cmd().u.consoleTitle = buf;
    rpc(Command::GetConsoleTitle);
    buf = cmd().u.consoleTitle;
    return cmd().dword;
}

std::vector<DWORD> RemoteWorker::consoleProcessList() {
    rpc(Command::GetConsoleProcessList);
    DWORD count = cmd().dword;
    ASSERT(count <= cmd().u.processList.size());
    return std::vector<DWORD>(
        &cmd().u.processList[0],
        &cmd().u.processList[count]);
}

uint64_t RemoteWorker::lookupKernelObject(DWORD pid, HANDLE h) {
    const uint64_t h64 = reinterpret_cast<uint64_t>(h);
    cmd().lookupKernelObject.pid = pid;
    memcpy(&cmd().lookupKernelObject.handle, &h64, sizeof(h64));
    rpc(Command::LookupKernelObject);
    uint64_t ret;
    memcpy(&ret, &cmd().lookupKernelObject.kernelObject, sizeof(ret));
    return ret;
}

void RemoteWorker::rpc(Command::Kind kind) {
    rpcImpl(kind);
    m_finishEvent.wait();
}

void RemoteWorker::rpcAsync(Command::Kind kind) {
    rpcImpl(kind);
}

void RemoteWorker::rpcImpl(Command::Kind kind) {
    ASSERT(m_valid && "Cannot perform an RPC on an invalid RemoteWorker");
    m_finishEvent.wait();
    m_finishEvent.reset();
    cmd().kind = kind;
    m_startEvent.set();
}
