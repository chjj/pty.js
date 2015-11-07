#include <TestCommon.h>

// Test for the Windows 7 win7_conout_crash bug.
//
// See console-handle.md, #win7_conout_crash, for theory.  Basically, if a
// process does not have a handle for a screen buffer, and it opens and closes
// CONOUT$, then the buffer is destroyed, even though another process is still
// using it.  Closing the *other* handles crashes conhost.exe.
//
// The bug affects Windows 7 SP1, but does not affect
// Windows Server 2008 R2 SP1, the server version of the OS.
//

REGISTER(Win7_RefCount_Bug, always);
static void Win7_RefCount_Bug() {
    {
        // Simplest demonstration:
        //
        // We will have two screen buffers in this test, O and N.  The parent opens
        // CONOUT$ to access N, but when it closes its handle, N is freed,
        // restoring O as the active buffer.
        //
        Worker p;
        p.getStdout().setFirstChar('O');
        auto c = p.child();
        c.newBuffer(false, 'N').activate();
        auto conout = p.openConout();
        CHECK_EQ(conout.firstChar(), 'N');
        conout.close();
        // At this point, Win7 is broken.  Test for it and hope we don't crash.
        conout = p.openConout();
        if (isWin7() && isWorkstation()) {
            CHECK_EQ(conout.firstChar(), 'O');
        } else {
            CHECK_EQ(conout.firstChar(), 'N');
        }
    }
    {
        // We can still "close" the handle by first importing it to another
        // process, then detaching that process from its console.
        Worker p;
        Worker assistant({ false, DETACHED_PROCESS });
        p.getStdout().setFirstChar('O');
        auto c = p.child();
        c.newBuffer(false, 'N').activate();

        // Do the read a few times for good measure.
        for (int i = 0; i < 5; ++i) {
            auto conout = p.openConout(true); // Must be inheritable!
            CHECK_EQ(conout.firstChar(), 'N');
            assistant.attach(p); // The attach imports the CONOUT$ handle
            conout.close();
            assistant.detach(); // Exiting would also work.
        }
    }
    {
        // If the child detaches, the screen buffer is still allocated.  This
        // demonstrates that the CONOUT$ handle *did* increment a refcount on
        // the buffer.
        Worker p;
        p.getStdout().setFirstChar('O');
        Worker c = p.child();
        c.newBuffer(false, 'N').activate();
        auto conout = p.openConout();
        c.detach(); // The child must exit/detach *without* closing the handle.
        CHECK_EQ(conout.firstChar(), 'N');
        auto conout2 = p.openConout();
        CHECK_EQ(conout2.firstChar(), 'N');
        // It is now safe to close the handles.  There is no other "console
        // object" referencing the screen buffer.
        conout.close();
        conout2.close();
    }
    {
        // If there are multiple console objects, closing any of them frees
        // the screen buffer.
        Worker p;
        auto c1 = p.child();
        auto c2 = p.child();
        p.getStdout().setFirstChar('O');
        p.newBuffer(false, 'N').activate();
        auto ch1 = c1.openConout();
        auto ch2 = c2.openConout();
        CHECK_EQ(ch1.firstChar(), 'N');
        CHECK_EQ(ch2.firstChar(), 'N');
        ch1.close();
        // At this point, Win7 is broken.  Test for it and hope we don't crash.
        auto testHandle = c1.openConout();
        if (isWin7() && isWorkstation()) {
            CHECK_EQ(testHandle.firstChar(), 'O');
        } else {
            CHECK_EQ(testHandle.firstChar(), 'N');
        }
    }

    if (isTraditionalConio()) {
        // Two processes can share a console object; in that case, CloseHandle
        // does not immediately fail.
        for (int i = 0; i < 2; ++i) {
            Worker p1;
            Worker p2 = p1.child();
            Worker p3({false, DETACHED_PROCESS});
            p1.getStdout().setFirstChar('O');
            Worker observer = p1.child();
            p1.newBuffer(false, 'N').activate();
            auto objref1 = p2.openConout(true);
            p3.attach(p2);
            auto objref2 = Handle::invent(objref1.value(), p3);
            if (i == 0) {
                objref1.close();
            } else {
                objref2.close();
            }
            CHECK_EQ(observer.openConout().firstChar(), 'N');
        }
    }
}
