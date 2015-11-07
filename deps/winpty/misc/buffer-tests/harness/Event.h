#pragma once

#include <windows.h>

#include <string>
#include <utility>

class Event {
public:
    explicit Event(const std::string &name);
    ~Event();
    void set();
    void reset();
    void wait();

    // no copying
    Event(const Event &other) = delete;
    Event &operator=(const Event &other) = delete;

    // moving is okay
    Event(Event &&other) { *this = std::move(other); }
    Event &operator=(Event &&other) {
        m_handle = other.m_handle;
        other.m_handle = NULL;
        return *this;
    }

private:
    HANDLE m_handle;
};
