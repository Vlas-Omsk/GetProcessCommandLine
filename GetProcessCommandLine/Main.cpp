#include <Windows.h>
#include <winternl.h>
#include <assert.h>
#include <stdio.h>
#include <atlstr.h>
#include "Main.h"

PVOID PhHeapHandle = NULL;
ULONG WindowsVersion = WINDOWS_NEW;
BOOL DebugPriviliges = FALSE;
RTL_OSVERSIONINFOEXW PhOsVersion = { 0 };

_RtlAllocateHeap RtlAllocateHeap =
(_RtlAllocateHeap)GetProcAddress(
    GetModuleHandleA("ntdll.dll"), "RtlAllocateHeap");
_RtlFreeHeap RtlFreeHeap =
(_RtlFreeHeap)GetProcAddress(
    GetModuleHandleA("ntdll.dll"), "RtlFreeHeap");
_NtQueryInformationProcess NtQueryInformationProcessEx =
(_NtQueryInformationProcess)GetProcAddress(
    GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
_RtlCreateHeap RtlCreateHeap =
(_RtlCreateHeap)GetProcAddress(
    GetModuleHandleA("ntdll.dll"), "RtlCreateHeap");
_RtlGetVersion RtlGetVersion =
(_RtlGetVersion)GetProcAddress(
    GetModuleHandleA("ntdll.dll"), "RtlGetVersion");
_RtlSetHeapInformation RtlSetHeapInformation =
(_RtlSetHeapInformation)GetProcAddress(
    GetModuleHandleA("ntdll.dll"), "RtlSetHeapInformation");

ULONG __stdcall InitializeWindowsVersion(void)
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
        return WindowsVersion;
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

    return WindowsVersion;
}

ULONG GetWindowsVersion(void) {
    return WindowsVersion;
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

long __stdcall GetProcessHandle(
    _In_ int pid,
    _Out_ HANDLE* ProcessHandle
) {
    AdjustDebugPriviliges();

    if ((*ProcessHandle = OpenProcess(
        PROCESS_QUERY_INFORMATION | /* required for NtQueryInformationProcess */
        PROCESS_VM_READ, /* required for ReadProcessMemory */
        FALSE, pid)) == 0)
    {
        return GetLastError();
    }

    return 0;
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