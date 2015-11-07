#include <TestCommon.h>

const int SC_CONSOLE_MARK = 0xFFF2;
const int SC_CONSOLE_SELECT_ALL = 0xFFF5;

int main() {
    SpawnParams sp;
    sp.bInheritHandles = TRUE;

    trace("----------------------------------");
    Worker p;
    p.getStdout().write("<-- origBuffer -->");

    auto c = p.child();
    auto cb = c.newBuffer();
    cb.activate();
    cb.write("<-- cb -->");

    // This is what the winpty-agent would want to do:
    //  - It tries to "freeze" the console with "Select All", which blocks
    //    WriteConsole but little else.  Closing a screen buffer is not
    //    blocked.
    //  - Then, winpty wants to get the buffer info, then read screen content.
    //  - If the child process closes its special screen buffer during the
    //    scraping, then on Windows 7, conhost can start reading freed memory
    //    and crash.  In this test case, `info2` is frequently garbage.
    // Somehow winpty-agent needs to avoid this situation, but options seem
    // scarce:
    //  - The Windows 7 bug only happens with `CloseHandle` AFAICT.  If a
    //    buffer handle goes away implicitly from `FreeConsole` or process
    //    exit, then the buffer is reference counted properly.  If app
    //    developers avoid closing their buffer handle, winpty can work.
    //  - Be really careful about when to scrape.  Pay close attention to
    //    the kinds of WinEvents a full-screen app generates just before it
    //    exits, and try to fast-path everything such that no scraping is
    //    necessary.
    //  - Start interfering with the user processes attached to the console.
    //     - e.g. inject a DLL inside the processes and open CONOUT$, or
    //       override APIs, etc.
    //     - Attach to the right console process before opening CONOUT$.  If
    //       that console's buffer handle is inheritable, then opening CONOUT$
    //       will then produce a safe handle.
    //  - Accept a certain amount of unreliability.
    SendMessage(p.consoleWindow(), WM_SYSCOMMAND, SC_CONSOLE_SELECT_ALL, 0);
    auto scrape = p.openConout();
    auto info1 = scrape.screenBufferInfo();
    cb.close();
    Sleep(200); // Helps the test fail more often.
    auto info2 = scrape.screenBufferInfo();
    SendMessage(p.consoleWindow(), WM_CHAR, 27, 0x00010001);

    trace("%d %d %d %d", info1.srWindow.Left, info1.srWindow.Top, info1.srWindow.Right, info1.srWindow.Bottom);
    trace("%d %d %d %d", info2.srWindow.Left, info2.srWindow.Top, info2.srWindow.Right, info2.srWindow.Bottom);

    Sleep(300000);
}
