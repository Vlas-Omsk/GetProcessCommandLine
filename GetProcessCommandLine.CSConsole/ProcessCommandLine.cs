using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace GetProcessCommandLine.CSConsole
{
    public class ProcessCommandLine
    {
        const string DLLPath = "ProcessCommandLine.dll";

        [DllImport(DLLPath, EntryPoint = "GetProcessCommandLine")]
        private static extern IntPtr _GetProcessCommandLine(IntPtr handle, out long status);
        public static string GetProcessCommandLine(IntPtr handle, out long status) =>
            Marshal.PtrToStringAnsi(_GetProcessCommandLine(handle, out status));

        [DllImport(DLLPath, EntryPoint = "GetProcessCommandLineByPid")]
        private static extern IntPtr _GetProcessCommandLineByPid(int pid, out long status);
        public static string GetProcessCommandLineByPid(int pid, out long status) =>
            Marshal.PtrToStringAnsi(_GetProcessCommandLineByPid(pid, out status));

        [DllImport(DLLPath)]
        public static extern bool AdjustDebugPriviliges();

        [DllImport(DLLPath)]
        public static extern WindowsVersion InitializeWindowsVersion();

        [DllImport(DLLPath)]
        public static extern long GetProcessHandle(int pid, out IntPtr processHandle);
    }

    public enum WindowsVersion : ulong
    {
        WINDOWS_ANCIENT = 0,
        WINDOWS_XP = 51,
        WINDOWS_VISTA = 60,
        WINDOWS_7 = 61,
        WINDOWS_8 = 62,
        WINDOWS_8_1 = 63,
        WINDOWS_10 = 100, // TH1
        WINDOWS_10_TH2 = 101,
        WINDOWS_10_RS1 = 102,
        WINDOWS_10_RS2 = 103,
        WINDOWS_10_RS3 = 104,
        WINDOWS_10_RS4 = 105,
        WINDOWS_10_RS5 = 106,
        WINDOWS_10_19H1 = 107,
        WINDOWS_10_19H2 = 108,
        WINDOWS_10_20H1 = 109,
        WINDOWS_10_20H2 = 110,
        WINDOWS_NEW = ulong.MaxValue
    }
}
