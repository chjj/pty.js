#include <TestCommon.h>

//
// Test CreateProcess, using PROC_THREAD_ATTRIBUTE_HANDLE_LIST to restrict the
// inherited handles.
//
// Ordinarily, standard handles are copied as-is.
//
// On Windows 8 and later, if a PROC_THREAD_ATTRIBUTE_HANDLE_LIST list is used,
// then the standard handles are duplicated instead.
//

REGISTER(Test_CreateProcess_InheritList, isAtLeastVista);
static void Test_CreateProcess_InheritList() {
    // Specifically test inherit lists.

    SpawnFailure failure;

    auto testSetup = [&](Worker &proc,
                         SpawnParams sp,
                         std::initializer_list<HANDLE> inheritList) {
        sp.dwCreationFlags |= EXTENDED_STARTUPINFO_PRESENT;
        sp.sui.cb = sizeof(STARTUPINFOEXW);
        sp.inheritCount = inheritList.size();
        std::copy(inheritList.begin(), inheritList.end(),
                  sp.inheritList.begin());
        return proc.tryChild(sp, &failure);
    };

    Worker p;
    auto pipe1 = newPipe(p, true);
    auto ph1 = std::get<0>(pipe1);
    auto ph2 = std::get<1>(pipe1);

    auto pipe2 = newPipe(p, true);
    auto ph3 = std::get<0>(pipe2);
    auto ph4 = std::get<1>(pipe2);

    auto phNI = ph1.dup(false);

    // Add an extra console handle so we can verify that a child's console
    // handles didn't revert to the original default, but were inherited.
    p.openConout(true);

    auto testSetupStdHandles = [&](SpawnParams sp) {
        const auto in = sp.sui.hStdInput;
        const auto out = sp.sui.hStdOutput;
        const auto err = sp.sui.hStdError;
        sp.dwCreationFlags |= EXTENDED_STARTUPINFO_PRESENT;
        sp.sui.cb = sizeof(STARTUPINFOEXW);
        // This test case isn't interested in what
        // PROC_THREAD_ATTRIBUTE_HANDLE_LIST does when there are duplicate
        // handles in its list.
        ASSERT(in != out && out != err && in != err);
        sp.inheritCount = 3;
        sp.inheritList = { in, out, err };
        return p.tryChild(sp, &failure);
    };

    auto ch1 = [&](RemoteWorker &c) { return Handle::invent(ph1.value(), c); };
    auto ch2 = [&](RemoteWorker &c) { return Handle::invent(ph2.value(), c); };
    auto ch3 = [&](RemoteWorker &c) { return Handle::invent(ph3.value(), c); };
    auto ch4 = [&](RemoteWorker &c) { return Handle::invent(ph4.value(), c); };

    {
        // Use PROC_THREAD_ATTRIBUTE_HANDLE_LIST correctly.
        auto c = testSetup(p, {true}, {ph1.value()});
        CHECK(c.valid());
        // i.e. ph1 was inherited, because ch1 identifies the same thing.
        // ph2 was not inherited, because it wasn't listed.
        ObjectSnap snap;
        CHECK(snap.eq(ph1, ch1(c)));
        CHECK(!snap.eq(ph2, ch2(c)));

        if (!isAtLeastWin8()) {
            // The traditional console handles were all inherited, but they're
            // also the standard handles, so maybe that's an exception.  We'll
            // test more aggressively below.
            CHECK(handleValues(c.scanForConsoleHandles()) ==
                  handleValues(p.scanForConsoleHandles()));
        }
    }
    {
        // UpdateProcThreadAttribute fails if the buffer size is zero.
        auto c = testSetup(p, {true}, {});
        CHECK(!c.valid());
        CHECK_EQ(failure.kind, SpawnFailure::UpdateProcThreadAttribute);
        CHECK_EQ(failure.errCode, (DWORD)ERROR_BAD_LENGTH);
    }
    {
        // Attempting to inherit the GetCurrentProcess pseudo-handle also
        // fails.  (The MSDN docs point out that using GetCurrentProcess here
        // will fail.)
        auto c = testSetup(p, {true}, {GetCurrentProcess()});
        CHECK(!c.valid());
        CHECK_EQ(failure.kind, SpawnFailure::CreateProcess);
        CHECK_EQ(failure.errCode, (DWORD)ERROR_INVALID_PARAMETER);
    }
    {
        // CreateProcess fails if the inherit list has a non-inheritable handle
        // in it.  (STARTF_USESTDHANDLES not set.)
        auto c1 = testSetup(p, {true}, {phNI.value()});
        CHECK(!c1.valid());
        CHECK_EQ(failure.kind, SpawnFailure::CreateProcess);
        CHECK_EQ(failure.errCode, (DWORD)ERROR_INVALID_PARAMETER);
    }
    {
        // CreateProcess fails if the inherit list has a non-inheritable handle
        // in it.  (STARTF_USESTDHANDLES set.)
        auto c = testSetup(p, {true, 0, {phNI, phNI, phNI}}, {phNI.value()});
        CHECK(!c.valid());
        CHECK_EQ(failure.kind, SpawnFailure::CreateProcess);
        CHECK_EQ(failure.errCode, (DWORD)ERROR_INVALID_PARAMETER);
    }
    {
        // If bInheritHandles=FALSE and PROC_THREAD_ATTRIBUTE_HANDLE_LIST are
        // combined, the API call fails.  (STARTF_USESTDHANDLES not set.)
        auto c = testSetup(p, {false}, {ph1.value()});
        CHECK(!c.valid());
        CHECK_EQ(failure.kind, SpawnFailure::CreateProcess);
        CHECK_EQ(failure.errCode, (DWORD)ERROR_INVALID_PARAMETER);
    }
    {
        // If bInheritHandles=FALSE and PROC_THREAD_ATTRIBUTE_HANDLE_LIST are
        // combined, the API call fails.  (STARTF_USESTDHANDLES set.)
        auto c = testSetupStdHandles({false, 0, {ph1, ph2, ph4}});
        CHECK(!c.valid());
        CHECK_EQ(failure.kind, SpawnFailure::CreateProcess);
        CHECK_EQ(failure.errCode, (DWORD)ERROR_INVALID_PARAMETER);
    }

    if (!isAtLeastWin8()) {
        // Attempt to restrict inheritance to just one of the three open
        // traditional console handles.
        auto c = testSetupStdHandles({true, 0, {ph1, ph2, p.getStderr()}});
        if (isWin7()) {
            // On Windows 7, the CreateProcess call fails with a strange
            // error.
            CHECK(!c.valid());
            CHECK_EQ(failure.kind, SpawnFailure::CreateProcess);
            CHECK_EQ(failure.errCode, (DWORD)ERROR_NO_SYSTEM_RESOURCES);
        } else {
            // On Vista, the CreateProcess call succeeds, but handle
            // inheritance is broken.  All of the console handles are
            // inherited, not just the error screen buffer that was listed.
            // None of the pipe handles were inherited, even though two were
            // listed.
            c.dumpConsoleHandles();
            CHECK(handleValues(c.scanForConsoleHandles()) ==
                  handleValues(p.scanForConsoleHandles()));
            {
                ObjectSnap snap;
                CHECK(!snap.eq(ph1, ch1(c)));
                CHECK(!snap.eq(ph2, ch2(c)));
                CHECK(!snap.eq(ph3, ch3(c)));
                CHECK(!snap.eq(ph4, ch4(c)));
            }
        }
    }

    if (!isAtLeastWin8()) {
        // Make a final valiant effort to find a
        // PROC_THREAD_ATTRIBUTE_HANDLE_LIST and console handle interaction.
        // We'll set all the standard handles to pipes.  Nevertheless, all
        // console handles are inherited.
        auto c = testSetupStdHandles({true, 0, {ph1, ph2, ph4}});
        CHECK(c.valid());
        CHECK(handleValues(c.scanForConsoleHandles()) ==
              handleValues(p.scanForConsoleHandles()));
    }

    //
    // What does it mean if the inherit list has a NULL handle in it?
    //

    {
        // CreateProcess apparently succeeds if the inherit list has a single
        // NULL in it.  Inheritable handles unrelated to standard handles are
        // not inherited.
        auto c = testSetup(p, {true}, {NULL});
        CHECK(c.valid());
        // None of the inheritable handles were inherited.
        ObjectSnap snap;
        CHECK(!snap.eq(ph1, ch1(c)));
        CHECK(!snap.eq(ph2, ch2(c)));
    }
    {
        // {NULL, a handle} ==> nothing is inherited.
        auto c = testSetup(p, {true}, {NULL, ph2.value()});
        CHECK(c.valid());
        ObjectSnap snap;
        CHECK(!snap.eq(ph1, ch1(c)));
        CHECK(!snap.eq(ph2, ch2(c)));
    }
    {
        // {a handle, NULL} ==> nothing is inherited.  (Apparently a NULL
        // anywhere in the list means "inherit nothing"?  The attribute is not
        // ignored.)
        auto c = testSetup(p, {true}, {ph1.value(), NULL});
        CHECK(c.valid());
        ObjectSnap snap;
        CHECK(!snap.eq(ph1, ch1(c)));
        CHECK(!snap.eq(ph2, ch2(c)));
    }
    {
        // bInheritHandles=FALSE still fails.
        auto c = testSetup(p, {false}, {NULL});
        CHECK(!c.valid());
        CHECK_EQ(failure.kind, SpawnFailure::CreateProcess);
        CHECK_EQ(failure.errCode, (DWORD)ERROR_INVALID_PARAMETER);
    }
    {
        // Test whether inheritList={NULL} has an unexpected effect on the
        // standard handles.  Everything seems consistent.
        auto q = testSetup(p, {true}, {ph1.value(), ph2.value()});
        ch1(q).setStdin();
        ch2(q).setStdout().setStderr();
        auto c = testSetup(q, {true}, {NULL});
        ObjectSnap snap;
        if (isAtLeastWin8()) {
            // In Windows 8, standard handles are duplicated if an inherit
            // list is specified.
            CHECK(snap.eq({c.getStdin(), q.getStdin(), ch1(q)}));
            CHECK(snap.eq({c.getStdout(), q.getStdout(), ch2(q)}));
            CHECK(snap.eq({c.getStderr(), q.getStderr(), ch2(q)}));
            CHECK(c.getStdout().value() != c.getStderr().value());
            CHECK(c.getStdin().tryFlags() && c.getStdin().inheritable());
            CHECK(c.getStdout().tryFlags() && c.getStdout().inheritable());
            CHECK(c.getStderr().tryFlags() && c.getStderr().inheritable());
        } else {
            // The standard handles were not successfully inherited.
            CHECK(handleValues(stdHandles(c)) == handleValues(stdHandles(q)));
            CHECK(!snap.eq(ch1(c), ch1(q)));
            CHECK(!snap.eq(ch2(c), ch2(q)));
        }
    }
}

REGISTER(Test_CreateProcess_InheritList_StdHandles, isAtLeastVista);
static void Test_CreateProcess_InheritList_StdHandles() {
    // List one of the standard handles in the inherit list, and see what
    // happens to the standard list.

    auto check = [](Worker &p, RemoteHandle rh, RemoteHandle wh) {
        ASSERT(!rh.isTraditionalConsole());
        ASSERT(!wh.isTraditionalConsole());
        {
            // Test bInheritHandles=TRUE, STARTF_USESTDHANDLES, and the
            // PROC_THREAD_ATTRIBUTE_HANDLE_LIST attribute.  Verify that the
            // standard handles are set to handles whose inheritability was
            // suppressed.
            SpawnParams sp { true, EXTENDED_STARTUPINFO_PRESENT, {rh, wh, wh} };
            sp.sui.cb = sizeof(STARTUPINFOEXW);
            sp.inheritCount = 1;
            sp.inheritList = { wh.value() };
            auto c = p.child(sp);
            ObjectSnap snap;
            CHECK(handleValues(stdHandles(c)) ==
                handleValues(std::vector<RemoteHandle> {rh, wh, wh}));
            CHECK(!snap.eq(rh, c.getStdin()));
            CHECK(snap.eq(wh, c.getStdout()));
            CHECK(snap.eq(wh, c.getStderr()));
        }

        {
            // Same as above, but use a single NULL in the inherit list.  Now
            // none of the handles are inherited, but the standard values are
            // unchanged.
            SpawnParams sp { true, EXTENDED_STARTUPINFO_PRESENT, {rh, wh, wh} };
            sp.sui.cb = sizeof(STARTUPINFOEXW);
            sp.inheritCount = 1;
            sp.inheritList = { NULL };
            auto c = p.child(sp);
            ObjectSnap snap;
            CHECK(handleValues(stdHandles(c)) ==
                handleValues(std::vector<RemoteHandle> {rh, wh, wh}));
            CHECK(!snap.eq(rh, c.getStdin()));
            CHECK(!snap.eq(wh, c.getStdout()));
            CHECK(!snap.eq(wh, c.getStderr()));
        }

        if (!isAtLeastWin8()) {
            // Same as above, but avoid STARTF_USESTDHANDLES this time.  The
            // behavior changed with Windows 8, which now appears to duplicate
            // handles in this case.
            rh.setStdin();
            wh.setStdout().setStderr();
            SpawnParams sp { true, EXTENDED_STARTUPINFO_PRESENT };
            sp.sui.cb = sizeof(STARTUPINFOEXW);
            sp.inheritCount = 1;
            sp.inheritList = { wh.value() };
            auto c = p.child(sp);
            ObjectSnap snap;
            CHECK(handleValues(stdHandles(p)) == handleValues(stdHandles(c)));
            CHECK(!snap.eq(p.getStdin(), c.getStdin()));
            CHECK(snap.eq(p.getStdout(), c.getStdout()));
        }
    };

    {
        Worker p;
        auto pipe = newPipe(p, true);
        check(p, std::get<0>(pipe), std::get<1>(pipe));
    }

    if (isModernConio()) {
        Worker p;
        check(p, p.openConin(true), p.openConout(true));
    }
}

REGISTER(Test_CreateProcess_InheritList_ModernDuplication, isAtLeastVista);
static void Test_CreateProcess_InheritList_ModernDuplication() {
    auto &hv = handleValues;

    for (int useDummyPipe = 0; useDummyPipe <= 1; ++useDummyPipe) {
        // Once we've specified an inherit list, non-inheritable standard
        // handles are duplicated.
        Worker p;
        auto pipe = newPipe(p);
        auto rh = std::get<0>(pipe).setStdin();
        auto wh = std::get<1>(pipe).setStdout().setStderr();
        auto c = childWithDummyInheritList(p, {}, useDummyPipe != 0);
        if (isModernConio()) {
            ObjectSnap snap;
            CHECK(snap.eq(rh, c.getStdin()));
            CHECK(snap.eq(wh, c.getStdout()));
            CHECK(snap.eq(wh, c.getStderr()));
            CHECK(c.getStdout().value() != c.getStderr().value());
            for (auto h : stdHandles(c)) {
                CHECK(!h.inheritable());
            }
        } else {
            CHECK(hv(stdHandles(c)) == hv(stdHandles(p)));
            CHECK(!c.getStdin().tryFlags());
            CHECK(!c.getStdout().tryFlags());
            CHECK(!c.getStderr().tryFlags());
        }
    }

    for (int useDummyPipe = 0; useDummyPipe <= 1; ++useDummyPipe) {
        // Invalid handles are translated to 0x0.  (For full details, see the
        // "duplicate" CreateProcess tests.)
        Worker p;
        Handle::invent(0x0ull, p).setStdin();
        Handle::invent(0xdeadbeefull, p).setStdout();
        auto c = childWithDummyInheritList(p, {}, useDummyPipe != 0);
        if (isModernConio()) {
            CHECK(c.getStdin().uvalue() == 0ull);
            CHECK(c.getStdout().uvalue() == 0ull);
        } else {
            CHECK(c.getStdin().uvalue() == 0ull);
            CHECK(c.getStdout().value() ==
                Handle::invent(0xdeadbeefull, c).value());
        }
    }
}

REGISTER(Test_CreateProcess_Duplicate_StdHandles, isModernConio);
static void Test_CreateProcess_Duplicate_StdHandles() {
    // The default Unbound console handles should be inheritable, so with
    // bInheritHandles=TRUE and standard handles listed in the inherit list,
    // the child process should have six console handles, all usable.
    Worker p;

    SpawnParams sp { true, EXTENDED_STARTUPINFO_PRESENT };
    sp.sui.cb = sizeof(STARTUPINFOEXW);
    sp.inheritCount = 3;
    sp.inheritList = {
        p.getStdin().value(),
        p.getStdout().value(),
        p.getStderr().value(),
    };
    auto c = p.child(sp);

    std::vector<uint64_t> expected;
    extendVector(expected, handleInts(stdHandles(p)));
    extendVector(expected, handleInts(stdHandles(c)));
    std::sort(expected.begin(), expected.end());

    auto correct = handleInts(c.scanForConsoleHandles());
    std::sort(correct.begin(), correct.end());

    p.dumpConsoleHandles();
    c.dumpConsoleHandles();

    CHECK(expected == correct);
}
