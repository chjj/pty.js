// Test GetConsoleTitleW.
//
// Each of these OS sets implements different semantics for the system call:
//  * Windows XP
//  * Vista and Windows 7
//  * Windows 8 and up (at least to Windows 10)
//

#include <TestCommon.h>

static void checkBuf(const std::array<wchar_t, 1024> &actual,
                     const std::array<wchar_t, 1024> &expected,
                     const char *filename,
                     int line) {
    if (actual != expected) {
        for (size_t i = 0; i < actual.size(); ++i) {
            if (actual[i] != expected[i]) {
                std::cout << filename << ":" << line << ": "
                          << "char mismatch: [" << i << "]: "
                          << actual[i] << " != " << expected[i]
                          << " ('" << static_cast<char>(actual[i]) << "' != '"
                                   << static_cast<char>(expected[i]) << "')"
                          << std::endl;
            }
        }
    }
}

#define CHECK_BUF(actual, ...) (checkBuf((actual), __VA_ARGS__, __FILE__, __LINE__))

int main() {
    Worker w;

    std::array<wchar_t, 1024> readBuf;
    const std::wstring kNul = std::wstring(L"", 1);
    const std::array<wchar_t, 1024> kJunk = {
        '1', '2', '3', '4', '5', '6', '7', '8',
        '9', '0', 'A', 'B', 'C', 'D', 'E', 'F',
    };

    for (auto inputStr : {
        std::wstring(L""),
        std::wstring(L"a"),
        std::wstring(L"ab"),
        std::wstring(L"abc"),
        std::wstring(L"abcd"),
        std::wstring(L"abcde"),
    }) {
        for (size_t readLen = 0; readLen < 12; ++readLen) {
            std::cout << "Testing \"" << narrowString(inputStr) << "\", "
                      << "reading " << readLen << " chars" << std::endl;

            // Set the title and read it back.
            w.setTitle(narrowString(inputStr));
            readBuf = kJunk;
            const DWORD retVal = w.titleInternal(readBuf, readLen);

            if (readLen == 0) {
                // When passing a buffer size 0, the API returns 0 and leaves
                // the buffer untouched.  Every OS version does the same thing.
                CHECK_EQ(retVal, 0u);
                CHECK_BUF(readBuf, kJunk);
                continue;
            }

            std::wstring expectedWrite;

            if (isAtLeastWin8()) {
                expectedWrite = inputStr.substr(0, readLen - 1) + kNul;

                // The call returns the untruncated length.
                CHECK_EQ(retVal, inputStr.size());
            }
            else if (isAtLeastVista()) {
                // Vista and Windows 7 have a bug where the title is instead
                // truncated to half the correct number of characters.  (i.e.
                // The `readlen` is seemingly interpreted as a byte count
                // rather than a character count.)  The bug isn't present on XP
                // or Windows 8.
                if (readLen == 1) {
                    // There is not even room for a NUL terminator, so it's
                    // just left off.  The call still succeeds, though.
                    expectedWrite = std::wstring();
                } else {
                    expectedWrite =
                        inputStr.substr(0, (readLen / 2) - 1) + kNul;
                }

                // The call returns the untruncated length.
                CHECK_EQ(retVal, inputStr.size());
            }
            else {
                // Unlike later OSs, XP returns a truncated title length.
                // Moreover, whenever it would return 0, either because:
                //  * the title is blank, and/or
                //  * the read length is 1
                // then XP does not NUL-terminate the buffer.
                const size_t truncatedLen = std::min(inputStr.size(), readLen - 1);
                if (truncatedLen == 0) {
                    expectedWrite = std::wstring();
                } else {
                    expectedWrite = inputStr.substr(0, truncatedLen) + kNul;
                }
                CHECK_EQ(retVal, truncatedLen);
            }

            // I will assume that remaining characters have undefined values,
            // but I suspect they're actually unchanged.  On the other hand,
            // the API must never modify the bytes beyond `readLen`.
            auto expected = kJunk;
            std::copy(&readBuf[0], &readBuf[readLen], expected.begin());
            std::copy(expectedWrite.begin(), expectedWrite.end(), expected.begin());

            CHECK_BUF(readBuf, expected);
        }
    }

    return 0;
}
