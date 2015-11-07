#include <TestCommon.h>

//
// Test CreateProcess when called with these parameters:
//  - STARTF_USESTDHANDLES is specified
//  - bInheritHandles=FALSE or bInheritHandles=TRUE
//  - CreationConsoleMode=NewConsole or CreationConsoleMode=Inherit
//
// Verify:
//  - The resulting traditional ConsoleHandleSet is correct.
//  - Windows 8 creates Unbound handles when appropriate.
//  - Standard handles are set correctly.
//
// Before Windows 8, the child process has the standard handles specified
// in STARTUPINFO, without exception.  Starting with Windows 8, the STARTUPINFO
// handles are ignored with bInheritHandles=FALSE, and even with
// bInheritHandles=TRUE, a NULL hStd{Input,Output,Error} field is translated to
// a new open handle if a new console is being created.

template <typename T>
void checkVariousInputs(T check) {
    {
        // Specify the original std values.  With CreationConsoleMode==Inherit
        // and bInheritHandles=FALSE, this code used to work (i.e. produce
        // valid standard handles in the child).  As of Windows 8, the standard
        // handles are now NULL instead.
        Worker p;
        check(p, stdHandles(p));
    }
    {
        Worker p;
        check(p, {
            p.getStdin().dup(),
            p.getStdout().dup(),
            p.getStderr().dup(),
        });
    }
    {
        Worker p;
        check(p, {
            p.getStdin().dup(true),
            p.getStdout().dup(true),
            p.getStderr().dup(true),
        });
    }
    {
        Worker p;
        check(p, {
            p.openConin(),
            p.openConout(),
            p.openConout(),
        });
    }
    {
        Worker p;
        check(p, {
            p.openConin(true),
            p.openConout(true),
            p.openConout(true),
        });
    }
    {
        // Invalid handles.
        Worker p;
        check(p, {
            Handle::invent(nullptr, p),
            Handle::invent(0x10000ull, p),
            Handle::invent(0xdeadbeecull, p),
        });
        check(p, {
            Handle::invent(INVALID_HANDLE_VALUE, p),
            Handle::invent(nullptr, p),
            Handle::invent(nullptr, p),
        });
        check(p, {
            Handle::invent(nullptr, p),
            Handle::invent(nullptr, p),
            Handle::invent(nullptr, p),
        });
    }
    {
        // Try a non-inheritable pipe.
        Worker p;
        auto pipe = newPipe(p, false);
        check(p, {
            std::get<0>(pipe),
            std::get<1>(pipe),
            std::get<1>(pipe),
        });
    }
    {
        // Try an inheritable pipe.
        Worker p;
        auto pipe = newPipe(p, true);
        check(p, {
            std::get<0>(pipe),
            std::get<1>(pipe),
            std::get<1>(pipe),
        });
    }
}

REGISTER(Test_CreateProcess_UseStdHandles, always);
static void Test_CreateProcess_UseStdHandles() {
    checkVariousInputs([](Worker &p, std::vector<Handle> newHandles) {
        ASSERT(newHandles.size() == 3);
        auto check = [&](Worker &c, bool inheritHandles, bool newConsole) {
            trace("Test_CreateProcess_UseStdHandles: "
                  "inheritHandles=%d newConsole=%d",
                  inheritHandles, newConsole);
            auto childHandles = stdHandles(c);
            if (isTraditionalConio()) {
                CHECK(handleValues(stdHandles(c)) == handleValues(newHandles));
                if (newConsole) {
                    checkInitConsoleHandleSet(c);
                } else {
                    checkInitConsoleHandleSet(c, p);
                }
                // The child handles have the same values as the parent.
                // Verify that the child standard handles point to the right
                // kernel objects.
                ObjectSnap snap;
                for (int i = 0; i < 3; ++i) {
                    if (newHandles[i].value() == nullptr ||
                            newHandles[i].value() == INVALID_HANDLE_VALUE) {
                        // Nothing to check.
                    } else if (newHandles[i].isTraditionalConsole()) {
                        // Everything interesting was already checked in
                        // checkInitConsoleHandleSet.
                    } else if (newHandles[i].tryFlags()) {
                        // A handle is not inherited simply because it is
                        // listed in STARTUPINFO.  The new child standard
                        // handle is valid iff:
                        //  - the parent handle was valid, AND
                        //  - the parent handle was inheritable, AND
                        //  - bInheritHandles is TRUE
                        //
                        // The logic below is not obviously true for all
                        // possible handle values, but it will work for all
                        // values we test for.  (i.e. There could be some
                        // handle H to object O that isn't inherited, but by
                        // sheer conincidence, the child gets a handle H that
                        // also refers to O.  (e.g. Windows internal objects.)
                        // This test case works because we know that Windows
                        // won't create a reference to our test objects.)
                        CHECK(snap.eq(newHandles[i], childHandles[i]) ==
                            (inheritHandles && newHandles[i].inheritable()));
                    }
                }
            } else {
                ObjectSnap snap;
                bool consoleOpened[3] = {false, false, false};
                for (int i = 0; i < 3; ++i) {
                    if (inheritHandles && newHandles[i].value() != nullptr) {
                        // The parent's standard handle is used, without
                        // validation or duplication.  It is not inherited
                        // simply because it is listed in STARTUPINFO.
                        CHECK(childHandles[i].value() ==
                            newHandles[i].value());
                        if (newHandles[i].value() == INVALID_HANDLE_VALUE) {
                            // The test below does not work on the current
                            // process pseudo-handle (aka
                            // INVALID_HANDLE_VALUE).
                        } else if (newHandles[i].tryFlags()) {
                            CHECK(snap.eq(newHandles[i], childHandles[i]) ==
                                newHandles[i].inheritable());
                        }
                    } else if (newConsole) {
                        consoleOpened[i] = true;
                    } else {
                        CHECK(childHandles[i].value() == nullptr);
                    }
                }
                checkModernConsoleHandleInit(c,
                    consoleOpened[0],
                    consoleOpened[1],
                    consoleOpened[2]);
            }
        };

        for (int inheritInt = 0; inheritInt <= 1; ++inheritInt) {
            const bool inherit = inheritInt != 0;
            auto c1 = p.child({inherit, 0, newHandles});
            check(c1, inherit, false);
            auto c2 = p.child(
                {inherit, Worker::defaultCreationFlags(), newHandles});
            check(c2, inherit, true);
        }
    });
}
