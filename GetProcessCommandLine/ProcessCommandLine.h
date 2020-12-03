#pragma once

#include "Main.h"

char* __stdcall GetProcessCommandLine(
    _In_ HANDLE ProcessHandle,
    _Out_ long* status
);

char* __stdcall GetProcessCommandLineByPid(
    _In_ int pid,
    _Out_ long* status
);