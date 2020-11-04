#include <iostream>
#include <vector>
#include <Windows.h>
#include <tlhelp32.h>
#include "../GetProcessCommandLine/ProcessCommandLine.h"

std::vector<PROCESSENTRY32W> GetProcesses() {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
        return (std::vector<PROCESSENTRY32W>)NULL;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    int i = 0;

    std::vector<PROCESSENTRY32W> ProcessEntries;

    while (Process32Next(hProcessSnap, &pe32))
        ProcessEntries.push_back(pe32);

    CloseHandle(hProcessSnap);
    return ProcessEntries;
}

int wmain(int argc, wchar_t* argv[], wchar_t* envp[])
{
    AdjustDebugPriviliges();

    NTSTATUS ntstatus;
    char* str;

    if (argc <= 1) {
        std::vector<PROCESSENTRY32W> list = GetProcesses();

        for (int i = 0; i < list.size(); i++)
            std::wcout << "PID:          " << list[i].th32ProcessID << std::endl
                       << "EXE:          " << list[i].szExeFile << std::endl
                       << "PARENT PID:   " << list[i].th32ParentProcessID << std::endl
                       << "COMMAND LINE: " << ((str = GetProcessCommandLineByPid(list[i].th32ProcessID, &ntstatus)) ? str : "NULL") << std::endl
                       << std::endl;

        return 0;
    }

    if ((str = GetProcessCommandLineByPid(_wtoi(argv[1]), &ntstatus)) && NT_SUCCESS(ntstatus))
        std::cout << str;
    else
        std::cout << "Error code: " << ntstatus;

    return ntstatus;
}