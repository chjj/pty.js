TESTS += \
    build/Test_GetConsoleTitleW.exe \
    build/Win7Bug_InheritHandles.exe \
    build/Win7Bug_RaceCondition.exe

# To add tests that aren't checked-in, create an mk file of this name:
-include local_tests.mk
