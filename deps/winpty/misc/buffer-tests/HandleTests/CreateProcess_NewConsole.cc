#include <TestCommon.h>

//
// Test CreateProcess when called with these parameters:
//  - STARTF_USESTDHANDLES is not specified
//  - bInheritHandles=FALSE or bInheritHandles=TRUE
//  - CreationConsoleMode=NewConsole
//

REGISTER(Test_CreateProcess_NewConsole, always);
static void Test_CreateProcess_NewConsole() {
    auto check = [](Worker &p, bool inheritHandles) {
        auto c = p.child({ inheritHandles, Worker::defaultCreationFlags() });
        if (isTraditionalConio()) {
            checkInitConsoleHandleSet(c);
            CHECK(handleInts(stdHandles(c)) ==
                (std::vector<uint64_t> {0x3, 0x7, 0xb}));
        } else {
            checkModernConsoleHandleInit(c, true, true, true);
        }
        return c;
    };
    {
        Worker p;
        check(p, true);
        check(p, false);
    }
    {
        Worker p;
        p.openConin(false).setStdin();
        p.newBuffer(false).setStdout().dup(true).setStderr();
        check(p, true);
        check(p, false);
    }

    if (isModernConio()) {
        // The default Unbound console handles should be inheritable, so with
        // bInheritHandles=TRUE, the child process should have six console
        // handles, all usable.
        Worker p;
        auto c = check(p, true);

        std::vector<uint64_t> expected;
        extendVector(expected, handleInts(stdHandles(p)));
        extendVector(expected, handleInts(stdHandles(c)));
        std::sort(expected.begin(), expected.end());

        auto correct = handleInts(c.scanForConsoleHandles());
        std::sort(correct.begin(), correct.end());

        CHECK(expected == correct);
    }
}
