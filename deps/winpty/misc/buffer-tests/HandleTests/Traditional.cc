#include <TestCommon.h>

REGISTER(Test_HandleDuplication, isTraditionalConio);
static void Test_HandleDuplication() {
    // A traditional console handle cannot be duplicated to another process,
    // and it must be duplicated using the GetConsoleProcess() pseudo-value.
    // (This tests targetProcess != psuedo-value, but it doesn't test
    // sourceProcess != pseudo-value.  Not worth the trouble.)
    Worker p, other;
    p.getStdout().setFirstChar('x');
    CHECK_EQ(p.getStdout().dup().firstChar(), 'x');
    CHECK_EQ(p.getStdout().dup(p).value(), INVALID_HANDLE_VALUE);
    CHECK_EQ(p.getStdout().dup(other).value(), INVALID_HANDLE_VALUE);
}

REGISTER(Test_NewConsole_Resets_ConsoleHandleSet, isTraditionalConio);
static void Test_NewConsole_Resets_ConsoleHandleSet() {
    // Test that creating a new console properly resets everything.
    Worker p;

    // Open some handles to demonstrate the "clean slate" outcome.
    auto orig = stdHandles(p);
    p.getStdin().dup(true).setStdin();
    p.newBuffer(true).setStderr().dup(true).setStdout().activate();
    for (auto &h : orig) {
        h.close();
    }

    auto checkClean = [](Worker &proc) {
        proc.dumpConsoleHandles();
        CHECK_EQ(proc.getStdin().uvalue(), 0x3u);
        CHECK_EQ(proc.getStdout().uvalue(), 0x7u);
        CHECK_EQ(proc.getStderr().uvalue(), 0xbu);
        auto handles = proc.scanForConsoleHandles();
        CHECK(handleValues(handles) == (std::vector<HANDLE> {
            proc.getStdin().value(),
            proc.getStdout().value(),
            proc.getStderr().value(),
        }));
        CHECK(allInheritable(handles));
    };

    // A child with a new console is reset to a blank slate.
    for (int inherit = 0; inherit <= 1; ++inherit) {
        auto c1 = p.child({ inherit != 0, CREATE_NEW_CONSOLE });
        checkClean(c1);
        auto c2 = p.child({ inherit != 0, CREATE_NO_WINDOW });
        checkClean(c2);

        // Starting a child from a DETACHED_PROCESS also produces a clean
        // configuration.
        Worker detachedParent({ false, DETACHED_PROCESS });
        auto pipe = newPipe(detachedParent, true);
        std::get<0>(pipe).setStdin();
        std::get<1>(pipe).setStdout().dup(true).setStdout();
        Worker c3 = detachedParent.child({ inherit != 0, 0 });
        checkClean(c3);
    }

    // Similarly, detaching and allocating a new console resets the
    // ConsoleHandleSet.
    p.detach();
    p.alloc();
    checkClean(p);
}

REGISTER(Test_CreateProcess_DetachedProcess, isTraditionalConio);
static void Test_CreateProcess_DetachedProcess() {
    // A child with DETACHED_PROCESS has no console, and its standard handles
    // are set to 0 by default.
    Worker p;

    p.getStdin().dup(TRUE).setStdin();
    p.getStdout().dup(TRUE).setStdout();
    p.getStderr().dup(TRUE).setStderr();

    auto c = p.child({ true, DETACHED_PROCESS });

    CHECK(c.getStdin().uvalue() == 0);
    CHECK(c.getStdout().uvalue() == 0);
    CHECK(c.getStderr().uvalue() == 0);
    CHECK(c.scanForConsoleHandles().empty());
    CHECK(c.consoleWindow() == NULL);

    // XXX: What do GetConsoleCP and GetConsoleOutputCP do when no console is attached?

    // Verify that we have a blank slate even with an implicit console
    // creation.
    auto c2 = c.child({ true });
    auto c2h = c2.scanForConsoleHandles();
    CHECK(handleValues(c2h) == (std::vector<HANDLE> {
        c2.getStdin().value(),
        c2.getStdout().value(),
        c2.getStderr().value(),
    }));
}

REGISTER(Test_Creation_bInheritHandles_Flag, isTraditionalConio);
static void Test_Creation_bInheritHandles_Flag() {
    // The bInheritHandles flags to CreateProcess has no effect on console
    // handles.
    Worker p;
    for (auto &h : (Handle[]){
        p.getStdin(),
        p.getStdout(),
        p.getStderr(),
        p.newBuffer(false),
        p.newBuffer(true),
    }) {
        h.dup(false);
        h.dup(true);
    }
    auto cY = p.child({ true });
    auto cN = p.child({ false });
    auto &hv = handleValues;
    CHECK(hv(cY.scanForConsoleHandles()) == hv(inheritableHandles(p.scanForConsoleHandles())));
    CHECK(hv(cN.scanForConsoleHandles()) == hv(inheritableHandles(p.scanForConsoleHandles())));
}

REGISTER(Test_HandleAllocationOrder, isTraditionalConio);
static void Test_HandleAllocationOrder() {
    // When a new handle is created, it always assumes the lowest unused value.
    Worker p;

    auto h3 = p.getStdin();
    auto h7 = p.getStdout();
    auto hb = p.getStderr();
    auto hf = h7.dup(true);
    auto h13 = h3.dup(true);
    auto h17 = hb.dup(true);

    CHECK(h3.uvalue() == 0x3);
    CHECK(h7.uvalue() == 0x7);
    CHECK(hb.uvalue() == 0xb);
    CHECK(hf.uvalue() == 0xf);
    CHECK(h13.uvalue() == 0x13);
    CHECK(h17.uvalue() == 0x17);

    hf.close();
    h13.close();
    h7.close();

    h7 = h3.dup(true);
    hf = h3.dup(true);
    h13 = h3.dup(true);
    auto h1b = h3.dup(true);

    CHECK(h7.uvalue() == 0x7);
    CHECK(hf.uvalue() == 0xf);
    CHECK(h13.uvalue() == 0x13);
    CHECK(h1b.uvalue() == 0x1b);
}

REGISTER(Test_InheritNothing, isTraditionalConio);
static void Test_InheritNothing() {
    // It's possible for the standard handles to be non-inheritable.
    //
    // Avoid calling DuplicateHandle(h, FALSE), because it produces inheritable
    // console handles on Windows 7.
    Worker p;
    auto conin = p.openConin();
    auto conout = p.openConout();
    p.getStdin().close();
    p.getStdout().close();
    p.getStderr().close();
    conin.setStdin();
    conout.setStdout().dup().setStderr();
    p.dumpConsoleHandles();

    auto c = p.child({ true });
    // The child has no open console handles.
    CHECK(c.scanForConsoleHandles().empty());
    c.dumpConsoleHandles();
    // The standard handle values are inherited, even though they're invalid.
    CHECK(c.getStdin().value() == p.getStdin().value());
    CHECK(c.getStdout().value() == p.getStdout().value());
    CHECK(c.getStderr().value() == p.getStderr().value());
    // Verify a console is attached.
    CHECK(c.openConin().value() != INVALID_HANDLE_VALUE);
    CHECK(c.openConout().value() != INVALID_HANDLE_VALUE);
    CHECK(c.newBuffer().value() != INVALID_HANDLE_VALUE);
}

REGISTER(Test_AttachConsole_And_CreateProcess_Inheritance, isTraditionalConio);
static void Test_AttachConsole_And_CreateProcess_Inheritance() {
    Worker p;
    Worker unrelated({ false, DETACHED_PROCESS });

    auto conin = p.getStdin().dup(true);
    auto conout1 = p.getStdout().dup(true);
    auto conout2 = p.getStderr().dup(true);
    p.openConout(false); // an extra handle for checkInitConsoleHandleSet testing
    p.openConout(true);  // an extra handle for checkInitConsoleHandleSet testing
    p.getStdin().close();
    p.getStdout().close();
    p.getStderr().close();
    conin.setStdin();
    conout1.setStdout();
    conout2.setStderr();

    auto c = p.child({ true });

    auto c2 = c.child({ true });
    c2.detach();
    c2.attach(c);

    unrelated.attach(p);

    // The first child will have the same standard handles as the parent.
    CHECK(c.getStdin().value() == p.getStdin().value());
    CHECK(c.getStdout().value() == p.getStdout().value());
    CHECK(c.getStderr().value() == p.getStderr().value());

    // AttachConsole sets the handles to (0x3, 0x7, 0xb) regardless of handle
    // validity.  In this case, c2 initially had non-default handles, and it
    // attached to a process that has and also initially had non-default
    // handles.  Nevertheless, the new standard handles are the defaults.
    for (auto proc : {&c2, &unrelated}) {
        CHECK(proc->getStdin().uvalue() == 0x3);
        CHECK(proc->getStdout().uvalue() == 0x7);
        CHECK(proc->getStderr().uvalue() == 0xb);
    }

    // The set of inheritable console handles in these processes exactly match
    // that of the parent.
    checkInitConsoleHandleSet(c, p);
    checkInitConsoleHandleSet(c2, p);
    checkInitConsoleHandleSet(unrelated, p);
}

REGISTER(Test_Detach_Implicitly_Closes_Handles, isTraditionalConio);
static void Test_Detach_Implicitly_Closes_Handles() {
    // After detaching, calling GetHandleInformation fails on previous console
    // handles.

    Worker p;
    Handle orig[] = {
        p.getStdin(),
        p.getStdout(),
        p.getStderr(),
        p.getStdin().dup(TRUE),
        p.getStdout().dup(TRUE),
        p.getStderr().dup(TRUE),
        p.openConin(TRUE),
        p.openConout(TRUE),
    };

    p.detach();
    for (auto h : orig) {
        CHECK(!h.tryFlags());
    }
}

REGISTER(Test_AttachConsole_AllocConsole_StdHandles, isTraditionalConio);
static void Test_AttachConsole_AllocConsole_StdHandles() {
    // Verify that AttachConsole does the right thing w.r.t. console handle
    // sets and standard handles.

    auto check = [](bool newConsole, bool useStdHandles) {
        trace("checking: newConsole=%d useStdHandles=%d",
            newConsole, useStdHandles);
        Worker p;
        SpawnParams sp = useStdHandles
            ? SpawnParams { true, 0, stdHandles(p) }
            : SpawnParams { false, 0 };
        p.openConout(false); // 0x0f
        p.openConout(true);  // 0x13

        auto c = p.child(sp);
        auto pipe = newPipe(c, true);
        std::get<0>(pipe).setStdin();
        std::get<1>(pipe).setStdout().setStdout();
        auto origStdHandles = stdHandles(c);
        c.detach();
        CHECK(handleValues(stdHandles(c)) == handleValues(origStdHandles));

        if (newConsole) {
            c.alloc();
            checkInitConsoleHandleSet(c);
        } else {
            Worker other;
            auto out = other.newBuffer(true, 'N');      // 0x0f
            other.openConin(false);                     // 0x13
            auto in = other.openConin(true);            // 0x17
            out.activate();                             // activate new buffer
            other.getStdin().close();                   // close 0x03
            other.getStdout().close();                  // close 0x07
            other.getStderr().close();                  // close 0x0b
            in.setStdin();                              // 0x17
            out.setStdout().dup(true).setStderr();      // 0x0f and 0x1b
            c.attach(other);
            checkInitConsoleHandleSet(c, other);
        }

        if (useStdHandles) {
            CHECK(handleValues(stdHandles(c)) == handleValues(origStdHandles));
        } else {
            CHECK(handleInts(stdHandles(c)) ==
                (std::vector<uint64_t> { 0x3, 0x7, 0xb }));
        }
    };

    check(false, false);
    check(false, true);
    check(true, false);
    check(true, true);
}
