#include <TestCommon.h>

REGISTER(Test_CreateProcess_ModeCombos, always);
static void Test_CreateProcess_ModeCombos() {
    // It is often unclear how (or whether) various combinations of
    // CreateProcess parameters work when combined.  Try to test the ambiguous
    // combinations.

    SpawnFailure failure;

    {
        // CREATE_NEW_CONSOLE | DETACHED_PROCESS ==> call fails
        Worker p;
        auto c = p.tryChild({ false, CREATE_NEW_CONSOLE | DETACHED_PROCESS }, &failure);
        CHECK(!c.valid());
        CHECK_EQ(failure.kind, SpawnFailure::CreateProcess);
        CHECK_EQ(failure.errCode, (DWORD)ERROR_INVALID_PARAMETER);
    }
    {
        // CREATE_NO_WINDOW | CREATE_NEW_CONSOLE ==> CREATE_NEW_CONSOLE dominates
        Worker p;
        auto c = p.tryChild({ false, CREATE_NO_WINDOW | CREATE_NEW_CONSOLE }, &failure);
        CHECK(c.valid());
        CHECK(c.consoleWindow() != nullptr);
        CHECK(IsWindowVisible(c.consoleWindow()));
    }
    {
        // CREATE_NO_WINDOW | DETACHED_PROCESS ==> DETACHED_PROCESS dominates
        Worker p;
        auto c = p.tryChild({ false, CREATE_NO_WINDOW | DETACHED_PROCESS }, &failure);
        CHECK(c.valid());
        CHECK_EQ(c.newBuffer().value(), INVALID_HANDLE_VALUE);
    }
}

REGISTER(Test_CreateProcess_STARTUPINFOEX, isAtLeastVista);
static void Test_CreateProcess_STARTUPINFOEX() {
    // STARTUPINFOEX tests.

    Worker p;
    SpawnFailure failure;
    auto pipe1 = newPipe(p, true);
    auto ph1 = std::get<0>(pipe1);
    auto ph2 = std::get<1>(pipe1);

    auto testSetup = [&](SpawnParams sp, size_t cb, HANDLE inherit) {
        sp.sui.cb = cb;
        sp.inheritCount = 1;
        sp.inheritList = { inherit };
        return p.tryChild(sp, &failure);
    };

    {
        // The STARTUPINFOEX parameter is ignored if
        // EXTENDED_STARTUPINFO_PRESENT isn't present.
        auto c = testSetup({true}, sizeof(STARTUPINFOEXW), ph1.value());
        CHECK(c.valid());
        auto ch2 = Handle::invent(ph2.value(), c);
        // i.e. ph2 was inherited, because ch2 identifies the same thing.
        CHECK(compareObjectHandles(ph2, ch2));
    }
    {
        // If EXTENDED_STARTUPINFO_PRESENT is specified, but the cb value
        // is wrong, the API call fails.
        auto c = testSetup({true, EXTENDED_STARTUPINFO_PRESENT},
            sizeof(STARTUPINFOW), ph1.value());
        CHECK(!c.valid());
        CHECK_EQ(failure.kind, SpawnFailure::CreateProcess);
        CHECK_EQ(failure.errCode, (DWORD)ERROR_INVALID_PARAMETER);
    }
}

REGISTER(Test_CreateNoWindow_HiddenVsNothing, always);
static void Test_CreateNoWindow_HiddenVsNothing() {

    Worker p;
    auto c = p.child({ false, CREATE_NO_WINDOW });

    if (isAtLeastWin7()) {
        // As of Windows 7, GetConsoleWindow returns NULL.
        CHECK(c.consoleWindow() == nullptr);
    } else {
        // On earlier operating systems, GetConsoleWindow returns a handle
        // to an invisible window.
        CHECK(c.consoleWindow() != nullptr);
        CHECK(!IsWindowVisible(c.consoleWindow()));
    }
}

// MSDN's CreateProcess page currently has this note in it:
//
//     Important  The caller is responsible for ensuring that the standard
//     handle fields in STARTUPINFO contain valid handle values. These fields
//     are copied unchanged to the child process without validation, even when
//     the dwFlags member specifies STARTF_USESTDHANDLES. Incorrect values can
//     cause the child process to misbehave or crash. Use the Application
//     Verifier runtime verification tool to detect invalid handles.
//
// XXX: The word "even" here sticks out.  Verify that the standard handle
// fields in STARTUPINFO are ignored when STARTF_USESTDHANDLES is not
// specified.
