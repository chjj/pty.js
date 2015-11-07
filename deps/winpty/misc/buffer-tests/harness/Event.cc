#include "Event.h"

#include "UnicodeConversions.h"
#include <WinptyAssert.h>

Event::Event(const std::string &name) {
    // Create manual-reset, not signaled initially.
    m_handle = CreateEventW(NULL, TRUE, FALSE, widenString(name).c_str());
    ASSERT(m_handle != NULL);
}

Event::~Event() {
    if (m_handle != NULL) {
        CloseHandle(m_handle);
    }
}

void Event::set() {
    BOOL ret = SetEvent(m_handle);
    ASSERT(ret && "SetEvent failed");
}

void Event::reset() {
    BOOL ret = ResetEvent(m_handle);
    ASSERT(ret && "ResetEvent failed");
}

void Event::wait() {
    DWORD ret = WaitForSingleObject(m_handle, INFINITE);
    ASSERT(ret == WAIT_OBJECT_0 && "WaitForSingleObject failed");
}
