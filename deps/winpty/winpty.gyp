{
    'targets' : [
        {
            'target_name' : 'winpty-agent',
            'type' : 'executable',
            'include_dirs' : [
                'include',
            ],
            'defines' : [
                'UNICODE',
                '_UNICODE',
                '_WIN32_WINNT=0x0501',
                'NOMINMAX',
            ],
            'libraries' : [
                '-luser32.lib',
            ],
            'sources' : [
                'agent/Agent.cc',
                'agent/ConsoleFont.cc',
                'agent/ConsoleInput.cc',
                'agent/ConsoleLine.cc',
                'agent/Coord.cc',
                'agent/EventLoop.cc',
                'agent/LargeConsoleRead.cc',
                'agent/NamedPipe.cc',
                'agent/SmallRect.cc',
                'agent/Terminal.cc',
                'agent/Win32Console.cc',
                'agent/main.cc',
                'shared/DebugClient.cc',
                'shared/WinptyAssert.cc',
                'shared/winpty_wcsnlen.cc',
            ],
        },
        {
            'target_name' : 'winpty',
            'type' : 'shared_library',
            'include_dirs' : [
                'include',
            ],
            'defines' : [
                'UNICODE',
                '_UNICODE',
                '_WIN32_WINNT=0x0501',
                'NOMINMAX',
            ],
            'libraries' : [
                '-luser32.lib',
            ],
            'sources' : [
                'include/winpty.h',
                'libwinpty/winpty.cc',
                'shared/AgentMsg.h',
                'shared/Buffer.h',
                'shared/DebugClient.h',
                'shared/DebugClient.cc',
                'shared/c99_snprintf.h',
            ],
        },
        {
            'target_name' : 'winpty-debugserver',
            'type' : 'executable',
            'defines' : [
                'UNICODE',
                '_UNICODE',
                '_WIN32_WINNT=0x0501',
                'NOMINMAX',
            ],
            'sources' : [
                'debugserver/DebugServer.cc',
            ],
        }
    ],
}
