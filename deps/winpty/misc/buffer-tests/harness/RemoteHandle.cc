#include "RemoteHandle.h"

#include <string>
#include <vector>

#include "RemoteWorker.h"

#include <DebugClient.h>
#include <WinptyAssert.h>

RemoteHandle RemoteHandle::dup(HANDLE h, RemoteWorker &target,
                               BOOL bInheritHandle) {
    HANDLE targetHandle;
    BOOL success = DuplicateHandle(
        GetCurrentProcess(),
        h,
        target.m_process,
        &targetHandle,
        0, bInheritHandle, DUPLICATE_SAME_ACCESS);
    ASSERT(success && "DuplicateHandle failed");
    return RemoteHandle(targetHandle, target);
}

RemoteHandle &RemoteHandle::activate() {
    worker().cmd().handle = m_value;
    worker().rpc(Command::SetActiveBuffer);
    return *this;
}

void RemoteHandle::write(const std::string &msg) {
    worker().cmd().handle = m_value;
    worker().cmd().u.writeText = msg;
    worker().rpc(Command::WriteText);
}

void RemoteHandle::close() {
    worker().cmd().handle = m_value;
    worker().rpc(Command::Close);
}

RemoteHandle &RemoteHandle::setStdin() {
    worker().cmd().handle = m_value;
    worker().rpc(Command::SetStdin);
    return *this;
}

RemoteHandle &RemoteHandle::setStdout() {
    worker().cmd().handle = m_value;
    worker().rpc(Command::SetStdout);
    return *this;
}

RemoteHandle &RemoteHandle::setStderr() {
    worker().cmd().handle = m_value;
    worker().rpc(Command::SetStderr);
    return *this;
}

RemoteHandle RemoteHandle::dupImpl(RemoteWorker *target, BOOL bInheritHandle) {
    HANDLE targetProcessFromSource;

    if (target == nullptr) {
        targetProcessFromSource = GetCurrentProcess();
    } else {
        // Allow the source worker to see the target worker.
        targetProcessFromSource = INVALID_HANDLE_VALUE;
        BOOL success = DuplicateHandle(
            GetCurrentProcess(),
            target->m_process,
            worker().m_process,
            &targetProcessFromSource,
            0, FALSE, DUPLICATE_SAME_ACCESS);
        ASSERT(success && "Process handle duplication failed");
    }

    // Do the user-level duplication in the source process.
    worker().cmd().handle = m_value;
    worker().cmd().targetProcess = targetProcessFromSource;
    worker().cmd().bInheritHandle = bInheritHandle;
    worker().rpc(Command::Duplicate);
    HANDLE retHandle = worker().cmd().handle;

    if (target != nullptr) {
        // Cleanup targetProcessFromSource.
        worker().cmd().handle = targetProcessFromSource;
        worker().rpc(Command::CloseQuietly);
        ASSERT(worker().cmd().success &&
            "Error closing remote process handle");
    }

    return RemoteHandle(retHandle, target != nullptr ? *target : worker());
}

CONSOLE_SCREEN_BUFFER_INFO RemoteHandle::screenBufferInfo() {
    CONSOLE_SCREEN_BUFFER_INFO ret;
    bool success = tryScreenBufferInfo(&ret);
    ASSERT(success && "GetConsoleScreenBufferInfo failed");
    return ret;
}

bool RemoteHandle::tryScreenBufferInfo(CONSOLE_SCREEN_BUFFER_INFO *info) {
    worker().cmd().handle = m_value;
    worker().rpc(Command::GetConsoleScreenBufferInfo);
    if (worker().cmd().success && info != nullptr) {
        *info = worker().cmd().u.consoleScreenBufferInfo;
    }
    return worker().cmd().success;
}

DWORD RemoteHandle::flags() {
    DWORD ret;
    bool success = tryFlags(&ret);
    ASSERT(success && "GetHandleInformation failed");
    return ret;
}

bool RemoteHandle::tryFlags(DWORD *flags) {
    worker().cmd().handle = m_value;
    worker().rpc(Command::GetHandleInformation);
    if (worker().cmd().success && flags != nullptr) {
        *flags = worker().cmd().dword;
    }
    return worker().cmd().success;
}

void RemoteHandle::setFlags(DWORD mask, DWORD flags) {
    bool success = trySetFlags(mask, flags);
    ASSERT(success && "SetHandleInformation failed");
}

bool RemoteHandle::trySetFlags(DWORD mask, DWORD flags) {
    worker().cmd().handle = m_value;
    worker().cmd().u.setFlags.mask = mask;
    worker().cmd().u.setFlags.flags = flags;
    worker().rpc(Command::SetHandleInformation);
    return worker().cmd().success;
}

wchar_t RemoteHandle::firstChar() {
    // The "first char" is useful for identifying which output buffer a handle
    // refers to.
    worker().cmd().handle = m_value;
    const SMALL_RECT region = {};
    auto &io = worker().cmd().u.consoleIo;
    io.bufferSize = { 1, 1 };
    io.bufferCoord = {};
    io.ioRegion = region;
    worker().rpc(Command::ReadConsoleOutput);
    ASSERT(worker().cmd().success);
    ASSERT(!memcmp(&io.ioRegion, &region, sizeof(region)));
    return io.buffer[0].Char.UnicodeChar;
}

RemoteHandle &RemoteHandle::setFirstChar(wchar_t ch) {
    // The "first char" is useful for identifying which output buffer a handle
    // refers to.
    worker().cmd().handle = m_value;
    const SMALL_RECT region = {};
    auto &io = worker().cmd().u.consoleIo;
    io.buffer[0].Char.UnicodeChar = ch;
    io.buffer[0].Attributes = 7;
    io.bufferSize = { 1, 1 };
    io.bufferCoord = {};
    io.ioRegion = region;
    worker().rpc(Command::WriteConsoleOutput);
    ASSERT(worker().cmd().success);
    ASSERT(!memcmp(&io.ioRegion, &region, sizeof(region)));
    return *this;
}

bool RemoteHandle::tryNumberOfConsoleInputEvents(DWORD *ret) {
    worker().cmd().handle = m_value;
    worker().rpc(Command::GetNumberOfConsoleInputEvents);
    if (worker().cmd().success && ret != nullptr) {
        *ret = worker().cmd().dword;
    }
    return worker().cmd().success;
}

std::vector<RemoteHandle> inheritableHandles(
        const std::vector<RemoteHandle> &vec) {
    std::vector<RemoteHandle> ret;
    for (auto h : vec) {
        if (h.inheritable()) {
            ret.push_back(h);
        }
    }
    return ret;
}

std::vector<uint64_t> handleInts(const std::vector<RemoteHandle> &vec) {
    std::vector<uint64_t> ret;
    for (auto h : vec) {
        ret.push_back(reinterpret_cast<uint64_t>(h.value()));
    }
    return ret;
}

std::vector<HANDLE> handleValues(const std::vector<RemoteHandle> &vec) {
    std::vector<HANDLE> ret;
    for (auto h : vec) {
        ret.push_back(h.value());
    }
    return ret;
}

// It would make more sense to use a std::tuple here, but it's inconvenient.
std::vector<RemoteHandle> stdHandles(RemoteWorker &worker) {
    return {
        worker.getStdin(),
        worker.getStdout(),
        worker.getStderr(),
    };
}

// It would make more sense to use a std::tuple here, but it's inconvenient.
void setStdHandles(std::vector<RemoteHandle> handles) {
    ASSERT(handles.size() == 3);
    handles[0].setStdin();
    handles[1].setStdout();
    handles[2].setStderr();
}

bool allInheritable(const std::vector<RemoteHandle> &vec) {
    return handleValues(vec) == handleValues(inheritableHandles(vec));
}
