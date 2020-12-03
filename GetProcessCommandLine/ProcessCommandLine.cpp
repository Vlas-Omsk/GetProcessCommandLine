#include "Main.h"
#include "ProcessCommandLine.h"

PVOID GetPebAddress(HANDLE ProcessHandle)
{
    PROCESS_BASIC_INFORMATION pbi;

    NtQueryInformationProcessEx(ProcessHandle, 0, &pbi, sizeof(pbi), NULL);

    return pbi.PebBaseAddress;
}

char* GetProcessCommandLineNew(
    _In_ HANDLE ProcessHandle,
    _Out_ long* status
)
{
    PUNICODE_STRING commandLine;

    if (!NT_SUCCESS(*status = PhpQueryProcessVariableSize(
        ProcessHandle,
        (PROCESSINFOCLASS)60,
        (PVOID*)&commandLine
    )))
        return NULL;

    auto size = commandLine->Length;
    char* chTemp = (char*)malloc(size);
    WideCharToMultiByte(CP_ACP, 0, commandLine->Buffer, -1, chTemp, size, 0, 0);

    return chTemp;
}

char* GetProcessCommandLineOld(
    _In_ HANDLE ProcessHandle,
    _Out_ long* status
)
{
    PVOID pebAddress = GetPebAddress(ProcessHandle);
    PVOID rtlUserProcParamsAddress;
    UNICODE_STRING commandLine;

    if (!ReadProcessMemory(ProcessHandle, (PCHAR)pebAddress + 0x10,
        &rtlUserProcParamsAddress, sizeof(PVOID), NULL))
    {
        *status = GetLastError();
        return NULL;
    }

    if (!ReadProcessMemory(ProcessHandle, (PCHAR)rtlUserProcParamsAddress + 0x40,
        &commandLine, sizeof(commandLine), NULL))
    {
        *status = GetLastError();
        return NULL;
    }

    WCHAR* commandLineContents = (WCHAR*)malloc(commandLine.Length);

    if (!ReadProcessMemory(ProcessHandle, commandLine.Buffer,
        commandLineContents, commandLine.Length, NULL))
    {
        *status = GetLastError();
        return NULL;
    }

    CloseHandle(ProcessHandle);

    auto size = commandLine.Length;
    char* chTemp = (char*)malloc(size);
    //wcstombs(chTemp, commandLineContents, size);
    WideCharToMultiByte(CP_ACP, 0, commandLineContents, -1, chTemp, size, 0, 0);

    *status = 0;
    return chTemp;
}

char* __stdcall GetProcessCommandLine(
    _In_ HANDLE ProcessHandle,
    _Out_ long* status
)
{
    AdjustDebugPriviliges();

    if (PhHeapHandle == NULL) {
        InitializeWindowsVersion();
        if (!PhHeapInitialization(0, 0)) {
            *status = -1;
            return NULL;
        }
    }

    if (WindowsVersion >= WINDOWS_8_1)
        return GetProcessCommandLineNew(ProcessHandle, status);

    return GetProcessCommandLineOld(ProcessHandle, status);
}

char* __stdcall GetProcessCommandLineByPid(
    _In_ int pid,
    _Out_ long* status
)
{
    HANDLE ProcessHandle;
    if (GetProcessHandle(pid, &ProcessHandle)) {
        *status = GetLastError();
        return NULL;
    }

    return GetProcessCommandLine(ProcessHandle, status);
}