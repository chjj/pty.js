#pragma once

#include <windows.h>

#include <cstdint>
#include <string>
#include <vector>

#include <WinptyAssert.h>

class RemoteWorker;

class RemoteHandle {
    friend class RemoteWorker;

private:
    RemoteHandle(HANDLE value, RemoteWorker &worker) :
        m_value(value), m_worker(&worker)
    {
    }

public:
    static RemoteHandle invent(HANDLE h, RemoteWorker &worker) {
        return RemoteHandle(h, worker);
    }
    static RemoteHandle invent(uint64_t h, RemoteWorker &worker) {
        return RemoteHandle(reinterpret_cast<HANDLE>(h), worker);
    }
    RemoteHandle &activate();
    void write(const std::string &msg);
    void close();
    RemoteHandle &setStdin();
    RemoteHandle &setStdout();
    RemoteHandle &setStderr();
private:
    RemoteHandle dupImpl(RemoteWorker *target, BOOL bInheritHandle);
public:
    RemoteHandle dup(RemoteWorker &target, BOOL bInheritHandle=FALSE) {
        return dupImpl(&target, bInheritHandle);
    }
    RemoteHandle dup(BOOL bInheritHandle=FALSE) {
        return dupImpl(nullptr, bInheritHandle);
    }
    static RemoteHandle dup(HANDLE h, RemoteWorker &target,
                            BOOL bInheritHandle=FALSE);
    CONSOLE_SCREEN_BUFFER_INFO screenBufferInfo();
    bool tryScreenBufferInfo(CONSOLE_SCREEN_BUFFER_INFO *info=nullptr);
    DWORD flags();
    bool tryFlags(DWORD *flags=nullptr);
    void setFlags(DWORD mask, DWORD flags);
    bool trySetFlags(DWORD mask, DWORD flags);
    bool inheritable() {
        return flags() & HANDLE_FLAG_INHERIT;
    }
    void setInheritable(bool inheritable) {
        auto success = trySetInheritable(inheritable);
        ASSERT(success && "setInheritable failed");
    }
    bool trySetInheritable(bool inheritable) {
        return trySetFlags(HANDLE_FLAG_INHERIT,
                           inheritable ? HANDLE_FLAG_INHERIT : 0);
    }
    wchar_t firstChar();
    RemoteHandle &setFirstChar(wchar_t ch);
    bool tryNumberOfConsoleInputEvents(DWORD *ret=nullptr);
    HANDLE value() const { return m_value; }
    uint64_t uvalue() const { return reinterpret_cast<uint64_t>(m_value); }
    bool isTraditionalConsole() const { return (uvalue() & 3) == 3; }
    RemoteWorker &worker() const { return *m_worker; }

private:
    HANDLE m_value;
    RemoteWorker *m_worker;
};

std::vector<RemoteHandle> inheritableHandles(
    const std::vector<RemoteHandle> &vec);
std::vector<uint64_t> handleInts(const std::vector<RemoteHandle> &vec);
std::vector<HANDLE> handleValues(const std::vector<RemoteHandle> &vec);
std::vector<RemoteHandle> stdHandles(RemoteWorker &worker);
void setStdHandles(std::vector<RemoteHandle> handles);
bool allInheritable(const std::vector<RemoteHandle> &vec);
