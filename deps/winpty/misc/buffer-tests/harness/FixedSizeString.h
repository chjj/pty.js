#pragma once

#include <cstdlib>
#include <cstring>
#include <string>

#include <WinptyAssert.h>

template <size_t N>
struct FixedSizeString {
public:
    std::string str() const {
        ASSERT(strnlen(data, N) < N);
        return std::string(data);
    }

    const char *c_str() const {
        ASSERT(strnlen(data, N) < N);
        return data;
    }

    FixedSizeString &operator=(const char *from) {
        ASSERT(strlen(from) < N);
        strcpy(data, from);
        return *this;
    }

    FixedSizeString &operator=(const std::string &from) {
        ASSERT(from.size() < N);
        ASSERT(from.size() == strlen(from.c_str()));
        strcpy(data, from.c_str());
        return *this;
    }

private:
    char data[N];
};
