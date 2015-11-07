#include <TestCommon.h>

REGISTER(Test_AttachConsole_AllocConsole_StdHandles, isModernConio);
static void Test_AttachConsole_AllocConsole_StdHandles() {
    // Verify that AttachConsole does the right thing w.r.t. console handle
    // sets and standard handles.

    auto check = [](bool newConsole, bool useStdHandles, int nullIndex) {
        trace("checking: newConsole=%d useStdHandles=%d nullIndex=%d",
            newConsole, useStdHandles, nullIndex);
        Worker p;
        SpawnParams sp = useStdHandles
            ? SpawnParams { true, 0, stdHandles(p) }
            : SpawnParams { false, 0 };

        auto c = p.child(sp);
        auto pipe = newPipe(c, true);
        std::get<0>(pipe).setStdin();
        std::get<1>(pipe).setStdout().setStdout();

        if (nullIndex == 0) {
            Handle::invent(nullptr, c).setStdin();
        } else if (nullIndex == 1) {
            Handle::invent(nullptr, c).setStdout();
        } else if (nullIndex == 2) {
            Handle::invent(nullptr, c).setStderr();
        }

        auto origStdHandles = stdHandles(c);
        c.detach();
        CHECK(handleValues(stdHandles(c)) == handleValues(origStdHandles));

        if (newConsole) {
            c.alloc();
        } else {
            Worker other;
            c.attach(other);
        }

        if (useStdHandles) {
            auto curHandles = stdHandles(c);
            for (int i = 0; i < 3; ++i) {
                if (i != nullIndex) {
                    CHECK(curHandles[i].value() == origStdHandles[i].value());
                }
            }
            checkModernConsoleHandleInit(c,
                nullIndex == 0,
                nullIndex == 1,
                nullIndex == 2);
        } else {
            checkModernConsoleHandleInit(c, true, true, true);
        }
    };

    for (int i = -1; i < 3; ++i) {
        check(false, false, i);
        check(false, true, i);
        check(true, false, i);
        check(true, true, i);
    }
}

REGISTER(Test_Unbound_vs_Bound, isModernConio);
static void Test_Unbound_vs_Bound() {
    {
        // An Unbound output handle refers to the initial buffer.
        Worker p;
        auto ob = p.getStdout().setFirstChar('O');
        p.newBuffer(true, 'N').activate().setStdout().setStderr();
        CHECK_EQ(ob.firstChar(), 'O');

        // The handle can come from another process.
        Worker p2;
        CHECK_EQ(p2.getStdout().dup(p).firstChar(), 'O');

        // CONOUT$ will use the new buffer, though.
        CHECK_EQ(p.openConout().firstChar(), 'N');
    }
    {
        // A Bound handle from another process does not work.
        Worker wa;
        Worker wb;
        wa.getStdout().setFirstChar('a');
        wb.getStdout().setFirstChar('b');
        auto a_b = wb.openConout().dup(wa);
        auto a_c = wb.newBuffer(false, 'c').dup(wa);
        CHECK(a_b.tryFlags());
        CHECK(a_c.tryFlags());
        CHECK(!a_b.tryScreenBufferInfo());
        CHECK(!a_c.tryScreenBufferInfo());

        // We can *make* them work, though, if we reattach p to p2's console.
        wa.detach();
        CHECK(a_b.tryFlags() && a_c.tryFlags());
        wa.attach(wb);
        CHECK(a_b.tryScreenBufferInfo() && a_b.firstChar() == 'b');
        CHECK(a_c.tryScreenBufferInfo() && a_c.firstChar() == 'c');
    }
}

REGISTER(Test_Console_Without_Processes, isModernConio);
static void Test_Console_Without_Processes() {
    auto waitForTitle = [](HWND hwnd, const std::string &title) {
        for (int i = 0; i < 100 && (windowText(hwnd) != title); ++i) {
            Sleep(20);
        }
    };
    auto waitForNotTitle = [](HWND hwnd, const std::string &title) {
        for (int i = 0; i < 100 && (windowText(hwnd) == title); ++i) {
            Sleep(20);
        }
    };
    {
        // It is possible to have a console with no attached process.  Verify
        // that the console window has the expected title even after its only
        // process detaches.  The window dies once the duplicated Bound handle
        // is closed.
        Worker p({ false, CREATE_NEW_CONSOLE });
        auto bound = p.openConout();
        auto hwnd = p.consoleWindow();
        auto title = makeTempName(__FUNCTION__);
        p.setTitle(title);
        waitForTitle(hwnd, title);
        p.detach();
        Sleep(200);
        CHECK_EQ(windowText(hwnd), title);
        bound.close();
        waitForNotTitle(hwnd, title);
        CHECK(windowText(hwnd) != title);
    }
}

REGISTER(Test_Implicit_Buffer_Reference, isModernConio);
static void Test_Implicit_Buffer_Reference() {
    // Test that a process attached to a console holds an implicit reference
    // to the screen buffer that was active at attachment.
    auto activeFirstChar = [](Worker &proc) {
        auto b = proc.openConout();
        auto ret = b.firstChar();
        b.close();
        return ret;
    };
    {
        Worker p;
        Worker p2({ false, DETACHED_PROCESS });
        p.getStdout().setFirstChar('A');
        auto b = p.newBuffer(false, 'B').activate();
        auto pipe = newPipe(p, true);

        // Spawn a child process that has no console handles open.
        SpawnParams sp({ true, EXTENDED_STARTUPINFO_PRESENT, {
            std::get<0>(pipe),
            std::get<1>(pipe),
            std::get<1>(pipe),
        }});
        sp.sui.cb = sizeof(STARTUPINFOEXW);
        sp.inheritCount = 2;
        sp.inheritList = {
            std::get<0>(pipe).value(),
            std::get<1>(pipe).value(),
        };
        auto c = p.child(sp);
        CHECK_EQ(c.scanForConsoleHandles().size(), 0u);

        // Now close the only open handle to the B buffer.  The active
        // buffer remains A, because the child implicitly references B.
        b.close();
        CHECK_EQ(activeFirstChar(p), 'B');
        c.detach();

        // Once the child detaches, B is freed, and A activates.
        CHECK_EQ(activeFirstChar(p), 'A');
    }
}

REGISTER(Test_FreeConsole_Closes_Handles, isModernConio);
static void Test_FreeConsole_Closes_Handles() {
    auto check = [](Worker &proc, bool ineq, bool outeq, bool erreq) {
        auto dupin = proc.getStdin().dup();
        auto dupout = proc.getStdout().dup();
        auto duperr = proc.getStderr().dup();
        proc.detach();
        ObjectSnap snap;
        CHECK_EQ(snap.eq(proc.getStdin(), dupin), ineq);
        CHECK_EQ(snap.eq(proc.getStdout(), dupout), outeq);
        CHECK_EQ(snap.eq(proc.getStderr(), duperr), erreq);
        dupin.close();
        dupout.close();
        duperr.close();
    };
    {
        // The child opened three console handles, so FreeConsole closes all of
        // them.
        Worker p;
        check(p, false, false, false);
    }
    {
        // The child inherited the handles, so FreeConsole closes none of them.
        Worker p;
        auto c = p.child({ true });
        check(c, true, true, true);
    }
    {
        // Duplicated console handles: still none of them are closed.
        Worker p;
        auto c = p.child({ false });
        check(c, true, true, true);
    }
    {
        // FreeConsole doesn't close the current stdhandles; it closes the
        // handles it opened at attach-time.
        Worker p;
        p.openConout().setStderr();
        check(p, false, false, true);
    }
    {
        // With UseStdHandles, handles aren't closed.
        Worker p;
        auto c = p.child({ true, 0, stdHandles(p) });
        check(c, true, true, true);
    }
    {
        // Using StdHandles, AllocConsole sometimes only opens a few handles.
        // Only the handles it opens are closed.
        Worker p({ false, DETACHED_PROCESS });
        auto pipe = newPipe(p, true);
        auto c = p.child({ true, DETACHED_PROCESS, {
            std::get<0>(pipe),
            std::get<1>(pipe),
            std::get<1>(pipe),
        }});
        Handle::invent(0ull, c).setStderr();
        c.alloc();
        CHECK(c.getStdin().value() == std::get<0>(pipe).value());
        CHECK(c.getStdout().value() == std::get<1>(pipe).value());
        CHECK(c.getStderr().tryScreenBufferInfo());
        check(c, true, true, false);
    }
}
