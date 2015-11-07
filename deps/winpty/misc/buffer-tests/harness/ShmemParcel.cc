#include "ShmemParcel.h"

#include "UnicodeConversions.h"
#include <DebugClient.h>
#include <WinptyAssert.h>

ShmemParcel::ShmemParcel(
    const std::string &name,
    CreationDisposition disposition,
    size_t size)
{
    if (disposition == CreateNew) {
        SetLastError(0);
        m_hfile = CreateFileMappingW(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            size,
            widenString(name).c_str());
        ASSERT(m_hfile != NULL && GetLastError() == 0 &&
            "Failed to create shared memory");
    } else if (disposition == OpenExisting) {
        m_hfile = OpenFileMappingW(
            FILE_MAP_ALL_ACCESS,
            FALSE,
            widenString(name).c_str());
        ASSERT(m_hfile != NULL && "Failed to open shared memory");
    } else {
        ASSERT(false && "Invalid disposition value");
    }
    m_view = MapViewOfFile(m_hfile, FILE_MAP_ALL_ACCESS, 0, 0, size);
    ASSERT(m_view != NULL && "Failed to map view of shared memory to create it");
}

ShmemParcel::~ShmemParcel()
{
    if (m_hfile != NULL) {
        UnmapViewOfFile(m_view);
        CloseHandle(m_hfile);
    }
}
