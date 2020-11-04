#include "ProcessCommandLine.h"

PVOID PhHeapHandle = NULL;
BOOL DebugPriviliges = FALSE;
RTL_OSVERSIONINFOEXW PhOsVersion = { 0 };

#pragma region Ph
VOID __stdcall InitializeWindowsVersion(
    VOID
)
{
    RTL_OSVERSIONINFOEXW versionInfo;
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG buildVersion;

    memset(&versionInfo, 0, sizeof(RTL_OSVERSIONINFOEXW));
    versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

    if (!NT_SUCCESS(RtlGetVersion(&versionInfo)))
    {
        WindowsVersion = WINDOWS_NEW;
        return;
    }

    memcpy(&PhOsVersion, &versionInfo, sizeof(RTL_OSVERSIONINFOEXW));
    majorVersion = versionInfo.dwMajorVersion;
    minorVersion = versionInfo.dwMinorVersion;
    buildVersion = versionInfo.dwBuildNumber;

    // Windows 7, Windows Server 2008 R2
    if (majorVersion == 6 && minorVersion == 1)
    {
        WindowsVersion = WINDOWS_7;
    }
    // Windows 8, Windows Server 2012
    else if (majorVersion == 6 && minorVersion == 2)
    {
        WindowsVersion = WINDOWS_8;
    }
    // Windows 8.1, Windows Server 2012 R2
    else if (majorVersion == 6 && minorVersion == 3)
    {
        WindowsVersion = WINDOWS_8_1;
    }
    // Windows 10, Windows Server 2016
    else if (majorVersion == 10 && minorVersion == 0)
    {
        if (buildVersion >= 19042)
        {
            WindowsVersion = WINDOWS_10_20H2;
        }
        else if (buildVersion >= 19041)
        {
            WindowsVersion = WINDOWS_10_20H1;
        }
        else if (buildVersion >= 18363)
        {
            WindowsVersion = WINDOWS_10_19H2;
        }
        else if (buildVersion >= 18362)
        {
            WindowsVersion = WINDOWS_10_19H1;
        }
        else if (buildVersion >= 17763)
        {
            WindowsVersion = WINDOWS_10_RS5;
        }
        else if (buildVersion >= 17134)
        {
            WindowsVersion = WINDOWS_10_RS4;
        }
        else if (buildVersion >= 16299)
        {
            WindowsVersion = WINDOWS_10_RS3;
        }
        else if (buildVersion >= 15063)
        {
            WindowsVersion = WINDOWS_10_RS2;
        }
        else if (buildVersion >= 14393)
        {
            WindowsVersion = WINDOWS_10_RS1;
        }
        else if (buildVersion >= 10586)
        {
            WindowsVersion = WINDOWS_10_TH2;
        }
        else if (buildVersion >= 10240)
        {
            WindowsVersion = WINDOWS_10;
        }
        else
        {
            WindowsVersion = WINDOWS_10;
        }
    }
    else
    {
        WindowsVersion = WINDOWS_NEW;
    }
}

BOOLEAN PhHeapInitialization(
    _In_opt_ SIZE_T HeapReserveSize,
    _In_opt_ SIZE_T HeapCommitSize
)
{
    if (WindowsVersion >= WINDOWS_8)
    {
        PhHeapHandle = RtlCreateHeap(
            HEAP_GROWABLE | HEAP_CREATE_SEGMENT_HEAP | HEAP_CLASS_1,
            NULL,
            0,
            0,
            NULL,
            NULL
        );
    }

    if (!PhHeapHandle)
    {
        PhHeapHandle = RtlCreateHeap(
            HEAP_GROWABLE | HEAP_CLASS_1,
            NULL,
            HeapReserveSize ? HeapReserveSize : 2 * 1024 * 1024, // 2 MB
            HeapCommitSize ? HeapCommitSize : 1024 * 1024, // 1 MB
            NULL,
            NULL
        );

        if (!PhHeapHandle)
            return FALSE;

        if (WindowsVersion >= WINDOWS_VISTA)
        {
            ULONG uTmp = 2UL;
            RtlSetHeapInformation(
                PhHeapHandle,
                HeapCompatibilityInformation,
                &uTmp,
                sizeof(ULONG)
            );
        }
    }

    return TRUE;
}

_May_raise_
_Post_writable_byte_size_(Size)
PVOID PhAllocate(
    _In_ SIZE_T Size
)
{
    assert(Size);
    return RtlAllocateHeap(PhHeapHandle, HEAP_GENERATE_EXCEPTIONS, Size);
}

VOID PhFree(
    _Frees_ptr_opt_ PVOID Memory
)
{
    RtlFreeHeap(PhHeapHandle, 0, Memory);
}

NTSTATUS PhpQueryProcessVariableSize(
    _In_ HANDLE ProcessHandle,
    _In_ _PROCESSINFOCLASS ProcessInformationClass,
    _Out_ PVOID* Buffer
)
{
    NTSTATUS status;
    PVOID buffer;
    ULONG returnLength = 0;

    status = NtQueryInformationProcessEx(
        ProcessHandle,
        ProcessInformationClass,
        NULL,
        0,
        &returnLength
    );

    if (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL && status != STATUS_INFO_LENGTH_MISMATCH)
        return status;

    buffer = PhAllocate(returnLength);
    status = NtQueryInformationProcessEx(
        ProcessHandle,
        ProcessInformationClass,
        buffer,
        returnLength,
        &returnLength
    );

    if (NT_SUCCESS(status))
    {
        *Buffer = buffer;
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}
#pragma endregion

PVOID GetPebAddress(HANDLE ProcessHandle)
{
    PROCESS_BASIC_INFORMATION pbi;

    NtQueryInformationProcessEx(ProcessHandle, 0, &pbi, sizeof(pbi), NULL);

    return pbi.PebBaseAddress;
}

BOOL __stdcall AdjustDebugPriviliges(void)
{
    BOOL bRes = FALSE;

    HANDLE hTok;
    if (DebugPriviliges == FALSE && OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hTok))
    {
        TOKEN_PRIVILEGES tp;
        LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid);
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        bRes = AdjustTokenPrivileges(hTok, FALSE, &tp, NULL, NULL, NULL);

        CloseHandle(hTok);
    }

    return DebugPriviliges = bRes;
}

char* GetProcessCommandLineNew(
    _In_ HANDLE ProcessHandle,
    _Out_ NTSTATUS* status
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
    _Out_ NTSTATUS* status
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
    wcstombs(chTemp, commandLineContents, size);

    *status = 0;
    return chTemp;
}

char* __stdcall GetProcessCommandLine(
    _In_ HANDLE ProcessHandle,
    _Out_ NTSTATUS* status
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
    _Out_ NTSTATUS* status
)
{
    AdjustDebugPriviliges();

    HANDLE ProcessHandle;

    if ((ProcessHandle = OpenProcess(
        PROCESS_QUERY_INFORMATION | /* required for NtQueryInformationProcess */
        PROCESS_VM_READ, /* required for ReadProcessMemory */
        FALSE, pid)) == 0)
    {
        *status = GetLastError();
        return NULL;
    }

    return GetProcessCommandLine(ProcessHandle, status);
}