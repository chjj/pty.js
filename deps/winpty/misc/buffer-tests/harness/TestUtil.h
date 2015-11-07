#pragma once

#include <windows.h>

#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "NtHandleQuery.h"
#include "RemoteHandle.h"
#include "Spawn.h"

class RemoteWorker;

#define CHECK(cond) \
    do {                                                                      \
        if (!(cond)) {                                                        \
            recordFailure(__FUNCTION__);                                      \
            trace("%s:%d: ERROR: check failed: " #cond, __FILE__, __LINE__);  \
            std::cout << __FILE__ << ":" << __LINE__                          \
                      << (": ERROR: check failed: " #cond)                    \
                      << std::endl;                                           \
        }                                                                     \
    } while(0)

#define CHECK_EQ(actual, expected) \
    do {                                                                      \
        auto a = (actual);                                                    \
        auto e = (expected);                                                  \
        if (a != e) {                                                         \
            recordFailure(__FUNCTION__);                                      \
            trace("%s:%d: ERROR: check failed "                               \
                  "(" #actual " != " #expected ")", __FILE__, __LINE__);      \
            std::cout << __FILE__ << ":" << __LINE__                          \
                      << ": ERROR: check failed "                             \
                      << ("(" #actual " != " #expected "): ")                 \
                      << a << " != " << e                                     \
                      << std::endl;                                           \
        }                                                                     \
    } while(0)

#define REGISTER(name, cond) \
    static void name(); \
    int g_register_ ## cond ## _ ## name = (registerTest(#name, cond, name), 0)

template <typename T>
static void extendVector(std::vector<T> &base, const std::vector<T> &to_add) {
    base.insert(base.end(), to_add.begin(), to_add.end());
}

// Test registration
void printTestName(const std::string &name);
void recordFailure(const std::string &name);
std::vector<std::string> failedTests();
void registerTest(const std::string &name, bool(&cond)(), void(&func)());
using RegistrationTable = std::vector<std::tuple<std::string, bool(*)(), void(*)()>>;
RegistrationTable registeredTests();
inline bool always() { return true; }

bool compareObjectHandles(RemoteHandle h1, RemoteHandle h2);

// NT kernel handle->object snapshot
class ObjectSnap {
public:
    ObjectSnap();
    uint64_t object(RemoteHandle h);
    bool eq(std::initializer_list<RemoteHandle> handles);
    bool eq(RemoteHandle h1, RemoteHandle h2) { return eq({h1, h2}); }
private:
    bool m_hasTable = false;
    std::vector<SYSTEM_HANDLE_ENTRY> m_table;
};

// Misc
std::tuple<RemoteHandle, RemoteHandle> newPipe(
        RemoteWorker &w, BOOL inheritable=FALSE);
std::string windowText(HWND hwnd);

// "domain-specific" routines: perhaps these belong outside the harness?
void checkInitConsoleHandleSet(RemoteWorker &child);
void checkInitConsoleHandleSet(RemoteWorker &child, RemoteWorker &source);
bool isUsableConsoleHandle(RemoteHandle h);
bool isUsableConsoleInputHandle(RemoteHandle h);
bool isUsableConsoleOutputHandle(RemoteHandle h);
bool isUnboundConsoleObject(RemoteHandle h);
void checkModernConsoleHandleInit(RemoteWorker &proc,
                                  bool in, bool out, bool err);
RemoteWorker childWithDummyInheritList(RemoteWorker &p, SpawnParams sp,
                                       bool dummyPipeInInheritList);
