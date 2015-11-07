#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <cwchar>
#include <iostream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "NtHandleQuery.h"
#include "OsVersion.h"
#include "RemoteHandle.h"
#include "RemoteWorker.h"
#include "TestUtil.h"
#include "UnicodeConversions.h"
#include "Util.h"

#include <DebugClient.h>
#include <WinptyAssert.h>
#include <winpty_wcsnlen.h>

using Handle = RemoteHandle;
using Worker = RemoteWorker;
