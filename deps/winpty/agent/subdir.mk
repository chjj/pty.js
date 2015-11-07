# Copyright (c) 2011-2012 Ryan Prichard
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

ALL_TARGETS += build/winpty-agent.exe

AGENT_OBJECTS = \
	build/mingw/agent/Agent.o \
	build/mingw/agent/ConsoleFont.o \
	build/mingw/agent/ConsoleInput.o \
	build/mingw/agent/ConsoleLine.o \
	build/mingw/agent/Coord.o \
	build/mingw/agent/EventLoop.o \
	build/mingw/agent/LargeConsoleRead.o \
	build/mingw/agent/NamedPipe.o \
	build/mingw/agent/SmallRect.o \
	build/mingw/agent/Terminal.o \
	build/mingw/agent/Win32Console.o \
	build/mingw/agent/main.o \
	build/mingw/shared/DebugClient.o \
	build/mingw/shared/WinptyAssert.o \
	build/mingw/shared/winpty_wcsnlen.o

build/winpty-agent.exe : $(AGENT_OBJECTS)
	@echo Linking $@
	@$(MINGW_CXX) $(MINGW_LDFLAGS) -o $@ $^

-include $(AGENT_OBJECTS:.o=.d)
