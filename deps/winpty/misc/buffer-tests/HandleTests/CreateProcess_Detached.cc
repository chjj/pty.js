#include <TestCommon.h>

// Test CreateProcess called with dwCreationFlags containing DETACHED_PROCESS.

// This macro will output nicer line information than a function if it fails.
#define CHECK_NULL(proc) \
    do {                                        \
        CHECK(handleInts(stdHandles(proc)) ==   \
            (std::vector<uint64_t> {0,0,0}));   \
    } while(0)

REGISTER(Test_CreateProcess_Detached, always);
static void Test_CreateProcess_Detached() {
    {
        Worker p;
        auto c1 = p.child({ true, DETACHED_PROCESS });
        CHECK_NULL(c1);
        auto c2 = p.child({ false, DETACHED_PROCESS });
        CHECK_NULL(c2);
    }
    {
        Worker p;
        auto c = p.child({ true, DETACHED_PROCESS, {
            p.getStdin(),
            p.getStdout(),
            p.getStderr(),
        }});
        CHECK(handleValues(stdHandles(c)) == handleValues(stdHandles(p)));
    }
    {
        Worker p;
        auto c = p.child({ false, DETACHED_PROCESS, {
            p.getStdin(),
            p.getStdout(),
            p.getStderr(),
        }});
        if (isTraditionalConio()) {
            CHECK(handleValues(stdHandles(c)) == handleValues(stdHandles(p)));
        } else{
            CHECK_NULL(c);
        }
    }
    {
        Worker p({ false, DETACHED_PROCESS });
        auto pipe = newPipe(p, true);
        std::get<0>(pipe).setStdin();
        std::get<1>(pipe).setStdout().setStderr();

        {
            auto c1 = p.child({ true, DETACHED_PROCESS });
            CHECK_NULL(c1);
            auto c2 = p.child({ false, DETACHED_PROCESS });
            CHECK_NULL(c2);
        }
        {
            // The worker p2 was started with STARTF_USESTDHANDLES and with
            // standard handles referring to a pipe.  Nevertheless, its
            // children's standard handles are NULL.
            auto p2 = p.child({ true, DETACHED_PROCESS, stdHandles(p) });
            auto c1 = p2.child({ true, DETACHED_PROCESS });
            CHECK_NULL(c1);
            auto c2 = p2.child({ false, DETACHED_PROCESS });
            CHECK_NULL(c2);
        }
    }
}
