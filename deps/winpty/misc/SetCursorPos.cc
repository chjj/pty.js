#include <windows.h>

#include "TestUtil.cc"
#include "../shared/DebugClient.cc"

int main(int argc, char *argv[]) {
    int col = atoi(argv[1]);
    int row = atoi(argv[2]);
    setCursorPos(col, row);
    return 0;
}
