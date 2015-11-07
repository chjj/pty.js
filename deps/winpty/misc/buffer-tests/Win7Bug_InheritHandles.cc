#include <TestCommon.h>

int main() {
    trace("----------------------------------");
    Worker p;
    p.getStdout().write("<-- origBuffer -->");

    auto c = p.child();
    auto cb = c.newBuffer(FALSE);
    cb.activate();
    cb.write("<-- cb -->");
    c.dumpConsoleHandles(TRUE);

    // Proposed fix: the agent somehow decides it should attach to this
    // particular child process.  Does that fix the problem?
    //
    // No, because the child's new buffer was not marked inheritable.  If it
    // were inheritable, then the parent would "inherit" the handle during
    // attach, and both processes would use the same refcount for
    // `CloseHandle`.
    p.detach();
    p.attach(c);
    p.dumpConsoleHandles(TRUE);
    auto pb = p.openConout();

    cb.close();

    // Demonstrate that pb is an invalid handle.
    pb.close();

    Sleep(300000);
}
