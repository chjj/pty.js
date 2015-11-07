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

# Use make -n to see the actual command-lines make would run.

default : all

PREFIX ?= /usr/local
UNIX_ADAPTER_EXE ?= console.exe

# Include config.mk but complain if it hasn't been created yet.
ifeq "$(wildcard config.mk)" ""
    $(error config.mk does not exist.  Please run ./configure)
endif
include config.mk

UNIX_CXXFLAGS += -MMD -Wall

MINGW_CXXFLAGS += \
	-MMD -Wall \
	-DUNICODE \
	-D_UNICODE \
	-D_WIN32_WINNT=0x0501 \
	-fno-exceptions \
	-fno-rtti \
	-O2

MINGW_LDFLAGS += -static -static-libgcc -static-libstdc++
UNIX_LDFLAGS += $(UNIX_LDFLAGS_STATIC)

include agent/subdir.mk
include debugserver/subdir.mk
include libwinpty/subdir.mk
include tests/subdir.mk
include unix-adapter/subdir.mk

all : $(ALL_TARGETS)

tests : $(TEST_PROGRAMS)

install : all
	mkdir -p $(PREFIX)/bin
	install -m 755 -p -s build/$(UNIX_ADAPTER_EXE) $(PREFIX)/bin
	install -m 755 -p -s build/winpty.dll $(PREFIX)/bin
	install -m 755 -p -s build/winpty-agent.exe $(PREFIX)/bin
	install -m 755 -p -s build/winpty-debugserver.exe $(PREFIX)/bin

clean :
	rm -fr build

distclean : clean
	rm -f config.mk

.PHONY : default all tests install clean distclean

build/mingw/%.o : %.cc
	@echo Compiling $<
	@mkdir -p $$(dirname $@)
	@$(MINGW_CXX) $(MINGW_CXXFLAGS) -I include -c -o $@ $<

build/unix/%.o : %.cc
	@echo Compiling $<
	@mkdir -p $$(dirname $@)
	@$(UNIX_CXX) $(UNIX_CXXFLAGS) -I include -c -o $@ $<
