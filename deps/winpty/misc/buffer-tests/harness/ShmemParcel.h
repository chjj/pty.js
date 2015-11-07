#pragma once

#include <windows.h>

#include <cstdlib>
#include <string>
#include <utility>

class ShmemParcel {
public:
    enum CreationDisposition {
        CreateNew,
        OpenExisting,
    };

public:
    ShmemParcel(
        const std::string &name,
        CreationDisposition disposition,
        size_t size);

    ~ShmemParcel();

    // no copying
    ShmemParcel(const ShmemParcel &other) = delete;
    ShmemParcel &operator=(const ShmemParcel &other) = delete;

    // moving is okay
    ShmemParcel(ShmemParcel &&other) {
        *this = std::move(other);
    }
    ShmemParcel &operator=(ShmemParcel &&other) {
        m_hfile = other.m_hfile;
        m_view = other.m_view;
        other.m_hfile = NULL;
        other.m_view = NULL;
        return *this;
    }

    void *view() { return m_view; }

private:
    HANDLE m_hfile;
    void *m_view;
};

template <typename T>
class ShmemParcelTyped {
public:
    ShmemParcelTyped(
        const std::string &name,
        ShmemParcel::CreationDisposition disposition) :
        m_parcel(name, disposition, sizeof(T))
    {
    }

    T &value() { return *static_cast<T*>(m_parcel.view()); }

private:
    ShmemParcel m_parcel;
};
