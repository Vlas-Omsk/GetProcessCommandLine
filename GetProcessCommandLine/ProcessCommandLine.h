#pragma once

#include <iostream>
#include <Windows.h>
#include <winternl.h>
#include <assert.h>
#include <stdio.h>
#include <atlstr.h>

#define STATUS_BUFFER_OVERFLOW           ((NTSTATUS)0x80000005L)
#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L)
#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)

#define WINDOWS_ANCIENT 0
#define WINDOWS_XP 51
#define WINDOWS_VISTA 60
#define WINDOWS_7 61
#define WINDOWS_8 62
#define WINDOWS_8_1 63
#define WINDOWS_10 100 // TH1
#define WINDOWS_10_TH2 101
#define WINDOWS_10_RS1 102
#define WINDOWS_10_RS2 103
#define WINDOWS_10_RS3 104
#define WINDOWS_10_RS4 105
#define WINDOWS_10_RS5 106
#define WINDOWS_10_19H1 107
#define WINDOWS_10_19H2 108
#define WINDOWS_10_20H1 109
#define WINDOWS_10_20H2 110
#define WINDOWS_NEW ULONG_MAX

#define HEAP_CLASS_1 0x00001000

#define _May_raise_

typedef struct _RTL_HEAP_PARAMETERS
{
    ULONG Length;
    SIZE_T SegmentReserve;
    SIZE_T SegmentCommit;
    SIZE_T DeCommitFreeBlockThreshold;
    SIZE_T DeCommitTotalFreeThreshold;
    SIZE_T MaximumAllocationSize;
    SIZE_T VirtualMemoryThreshold;
    SIZE_T InitialCommit;
    SIZE_T InitialReserve;
    HANDLE CommitRoutine;
    SIZE_T Reserved[2];
} RTL_HEAP_PARAMETERS, * PRTL_HEAP_PARAMETERS;

typedef PVOID(NTAPI* _RtlAllocateHeap)(
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _In_ SIZE_T Size
    );

typedef ULONG(NTAPI* _RtlFreeHeap)(
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _Frees_ptr_opt_ PVOID BaseAddress
    );

typedef PVOID(NTAPI* _RtlCreateHeap)(
    _In_ ULONG Flags,
    _In_opt_ PVOID HeapBase,
    _In_opt_ SIZE_T ReserveSize,
    _In_opt_ SIZE_T CommitSize,
    _In_opt_ PVOID Lock,
    _In_opt_ PRTL_HEAP_PARAMETERS Parameters
    );

typedef NTSTATUS(NTAPI* _NtQueryInformationProcess)(
    HANDLE ProcessHandle,
    DWORD ProcessInformationClass,
    PVOID ProcessInformation,
    DWORD ProcessInformationLength,
    PDWORD ReturnLength
    );

typedef NTSTATUS(NTAPI* _RtlGetVersion)(
    _Out_ PRTL_OSVERSIONINFOEXW VersionInformation // PRTL_OSVERSIONINFOW
    );

typedef NTSTATUS(NTAPI* _RtlSetHeapInformation)(
    _In_ PVOID HeapHandle,
    _In_ HEAP_INFORMATION_CLASS HeapInformationClass,
    _In_opt_ PVOID HeapInformation,
    _In_opt_ SIZE_T HeapInformationLength
);

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

ULONG WindowsVersion = WINDOWS_NEW;

VOID __stdcall InitializeWindowsVersion(
    VOID
);

char* __stdcall GetProcessCommandLine(
    _In_ HANDLE ProcessHandle,
    _Out_ NTSTATUS* status
);

char* __stdcall GetProcessCommandLineByPid(
    _In_ int pid,
    _Out_ NTSTATUS* status
);

BOOL __stdcall AdjustDebugPriviliges(void);