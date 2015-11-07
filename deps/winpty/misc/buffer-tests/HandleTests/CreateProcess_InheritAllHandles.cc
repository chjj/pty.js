#include <TestCommon.h>

//
// Test CreateProcess when called with these parameters:
//  - STARTF_USESTDHANDLES is not specified
//  - bInheritHandles=TRUE
//  - CreationConsoleMode=Inherit
//

REGISTER(Test_CreateProcess_InheritAllHandles, always);
static void Test_CreateProcess_InheritAllHandles() {
    auto &hv = handleValues;

    {
        // Simple case: the standard handles are left as-is.
        Worker p;
        auto pipe = newPipe(p, true);
        std::get<0>(pipe).setStdin();
        std::get<1>(pipe).setStdout().setStderr();
        auto c = p.child({ true });
        CHECK(hv(stdHandles(c)) == hv(stdHandles(p)));
    }

    {
        // We can pass arbitrary values through.
        Worker p;
        Handle::invent(0x0ull, p).setStdin();
        Handle::invent(0x10000ull, p).setStdout();
        Handle::invent(INVALID_HANDLE_VALUE, p).setStderr();
        auto c = p.child({ true });
        CHECK(hv(stdHandles(c)) == hv(stdHandles(p)));
    }

    {
        // Passing through a non-inheritable handle produces an invalid child
        // handle.
        Worker p;
        p.openConin(false).setStdin();
        p.openConout(false).setStdout().setStderr();
        auto c = p.child({ true });
        CHECK(hv(stdHandles(c)) == hv(stdHandles(p)));
        if (isTraditionalConio()) {
            CHECK(!c.getStdin().tryFlags());
            CHECK(!c.getStdout().tryFlags());
            CHECK(!c.getStderr().tryFlags());
        } else {
            ObjectSnap snap;
            CHECK(!snap.eq(p.getStdin(), c.getStdin()));
            CHECK(!snap.eq(p.getStdout(), c.getStdout()));
            CHECK(!snap.eq(p.getStderr(), c.getStderr()));
        }
    }
}
