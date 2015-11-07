#include <TestCommon.h>

// With CreateProcess's default handle duplication behavior, the
// GetCurrentProcess() psuedo-handle (i.e. INVALID_HANDLE_VALUE) is translated
// to a real handle value for the child process.  It is a handle to the parent
// process.  Naturally, this was unintended behavior, and as of Windows 8.1,
// the handle is instead translated to NULL.  On some older operating systems,
// the WOW64 mode also translates it to NULL.

const std::string bugParentProc = "BUG(parent-proc)";
const std::string okInvalid = "OK(INVALID)";
const std::string okNull = "OK(NULL)";

static std::string determineChildStdout(Worker &c, Worker &p) {
    if (c.getStdout().value() == nullptr) {
        return okNull;
    } else if (c.getStdout().value() == INVALID_HANDLE_VALUE) {
        return okInvalid;
    } else {
        auto handleToPInP = Handle::dup(p.processHandle(), p);
        CHECK(compareObjectHandles(c.getStdout(), handleToPInP));
        return bugParentProc;
    }
}

REGISTER(Test_CreateProcess_Duplicate_PseudoHandleBug, always);
static void Test_CreateProcess_Duplicate_PseudoHandleBug() {
    Worker p;
    Handle::invent(GetCurrentProcess(), p).setStdout();
    auto c = p.child({ false });

    const std::string expect =
        (isAtLeastWin8_1() || (isAtLeastVista() && isWow64()))
            ? okNull
            : bugParentProc;

    const std::string actual = determineChildStdout(c, p);

    trace("%s: actual: %s", __FUNCTION__, actual.c_str());
    std::cout << __FUNCTION__ << ": expect: " << expect << std::endl;
    std::cout << __FUNCTION__ << ": actual: " << actual << std::endl;
    CHECK_EQ(actual, expect);
}

REGISTER(Test_CreateProcess_Duplicate_PseudoHandleBug_IL, isAtLeastVista);
static void Test_CreateProcess_Duplicate_PseudoHandleBug_IL() {
    // As above, but use an inherit list.  With an inherit list, standard
    // handles are duplicated, but only with Windows 8 and up.
    for (int useDummyPipe = 0; useDummyPipe <= 1; ++useDummyPipe) {
        Worker p;
        Handle::invent(INVALID_HANDLE_VALUE, p).setStdout();
        auto c = childWithDummyInheritList(p, {}, useDummyPipe != 0);

        // Figure out what we expect to see.
        std::string expect;
        if (isAtLeastWin8_1()) {
            // Windows 8.1 turns INVALID_HANDLE_VALUE into NULL.
            expect = okNull;
        } else if (isAtLeastWin8()) {
            // Windows 8 tries to duplicate the handle.  WOW64 seems to be
            // OK, though.
            if (isWow64()) {
                expect = okNull;
            } else {
                expect = bugParentProc;
            }
        } else {
            // Prior to Windows 8, duplication doesn't occur in this case, so
            // the bug isn't relevant.  We run the test anyway, but it's less
            // interesting.
            expect = okInvalid;
        }

        const std::string actual = determineChildStdout(c, p);

        trace("%s: actual: %s", __FUNCTION__, actual.c_str());
        std::cout << __FUNCTION__ << ": expect: " << expect << std::endl;
        std::cout << __FUNCTION__ << ": actual: " << actual << std::endl;
        CHECK_EQ(actual, expect);
    }
}
