======
winpty
======

winpty is a Windows software package providing an interface similar to a Unix
pty-master for communicating with Windows console programs.  The package
consists of a library (libwinpty) and a tool for Cygwin and MSYS for running
Windows console programs in a Cygwin/MSYS pty.

The software works by starting the ``winpty-agent.exe`` process with a new,
hidden console window, which bridges between the console API and terminal
input/output escape codes.  It polls the hidden console's screen buffer for
changes and generates a corresponding stream of output.

The Unix adapter allows running Windows console programs (e.g. CMD, PowerShell,
IronPython, etc.) under ``mintty`` or Cygwin's ``sshd`` with
properly-functioning input (e.g. arrow and function keys) and output (e.g. line
buffering).  The library could be also useful for writing a non-Cygwin SSH
server.

Prerequisites
=============

You need the following to build winpty:

* A Cygwin or MSYS installation
* GNU make
* A MinGW g++ toolchain, v4 or later, to build ``winpty.dll`` and
  ``winpty-agent.exe``
* A g++ toolchain targeting Cygwin or MSYS, v3 or later, to build
  ``console.exe``

Winpty requires two g++ toolchains as it is split into two parts. The
binaries winpty.dll and winpty-agent.exe interface with the native Windows
command prompt window so they are compiled with the native MinGW toolchain.
The console.exe binary interfaces with the MSYS/Cygwin terminal so it is
compiled with the MSYS/Cygwin toolchain.

MinGW appears to be split into two distributions -- MinGW (creates 32-bit
binaries) and MinGW-w64 (creates both 32-bit and 64-bit binaries).  Either
one is acceptable, but the compiler must be v4 or later.

Cygwin packages
---------------

The default g++ compiler for Cygwin targets Cygwin itself, but Cygwin also
packages MinGW compilers from both the MinGW and MinGW-w64 projects.  As of
this writing, the necessary packages are:

* Either ``mingw-gcc-g++``, ``mingw64-i686-gcc-g++`` or
  ``mingw64-x86_64-gcc-g++``.  Select the appropriate compiler for your
  CPU architecture.
* ``gcc-g++``
* ``make``

MSYS packages
-------------

For the original MSYS, use the ``mingw-get`` tool (MinGW Installation Manager),
and select at least these components:

* ``mingw-developer-toolkit``
* ``mingw32-base``
* ``mingw32-gcc-g++``
* ``msys-base``
* ``msys-system-builder``

When running ``./configure``, make sure that ``mingw32-g++`` is in your
``PATH``.  It will be in the ``C:\MinGW\bin`` directory.

For MSYS2, use ``pacman`` and install at least these packages:

* ``msys/gcc``
* ``mingw32/mingw-w64-i686-gcc`` or ``mingw64/mingw-w64-x86_64-gcc``.  Select
  the appropriate compiler for your CPU architecture.
* ``make``

Build
=====

In the project directory, run ``./configure``, then ``make``.

This will produce three binaries:

* ``build/winpty.dll``
* ``build/winpty-agent.exe``
* ``build/console.exe``

Using the Unix adapter
======================

To run a Windows console program in ``mintty`` or Cygwin ``sshd``, prepend
``console.exe`` to the command-line::

    $ build/console.exe c:/Python27/python.exe
    Python 2.7.2 (default, Jun 12 2011, 15:08:59) [MSC v.1500 32 bit (Intel)] on win32
    Type "help", "copyright", "credits" or "license" for more information.
    >>> 10 + 20
    30
    >>> exit()
    $

Debugging winpty
================

winpty comes with a tool for collecting timestamped debugging output.  To use
it:

1. Run ``winpty-debugserver.exe`` on the same computer as winpty.
2. Set the ``WINPTY_DEBUG`` environment variable to 1 for the ``console.exe``
   process and/or the process using ``libwinpty.dll``.

winpty also recognizes a ``WINPTY_SHOW_CONSOLE`` environment variable.  Set it
to 1 to prevent winpty from hiding the console window.
