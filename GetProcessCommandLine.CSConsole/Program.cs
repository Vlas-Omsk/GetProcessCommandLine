using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Diagnostics;

namespace GetProcessCommandLine.CSConsole
{
    class Program
    {
        static void Main(string[] args)
        {
            ProcessCommandLine.AdjustDebugPriviliges();

            long status = 0;
            string str = "";

            if (args.Length == 0)
            {
                var list = Process.GetProcesses();

                foreach (var item in list)
                    Console.WriteLine($"PID:          {item.Id}\r\n" +
                                      $"EXE:          {item.ProcessName}\r\n" +
                                      $"COMMAND LINE: {(!string.IsNullOrEmpty(str = ProcessCommandLine.GetProcessCommandLineByPid(item.Id, out status)) ? str : "NULL")}\r\n");
            }
            else
            {

                try
                {
                    if (!string.IsNullOrEmpty(str = ProcessCommandLine.GetProcessCommandLineByPid(int.Parse(args[0]), out status)))
                        if (status == 0)
                            Console.WriteLine(str);
                        else
                            Console.WriteLine("Error code: " + status);
                    else
                        Console.WriteLine("Error code: " + status);
                }
                catch (Exception ex)
                {
                    Console.WriteLine("Error code: " + ex.HResult);
                }
            }

#if DEBUG
                Console.ReadLine();
#endif
        }


        //static void Test()
        //{
        //    Console.WriteLine(ProcessCommandLine.AdjustDebugPriviliges());

        //    long status = 0;

        //    Console.WriteLine(ProcessCommandLine.GetProcessCommandLine(Process.GetCurrentProcess().Handle, out status) + ", Status=" + status);

        //    Console.WriteLine(ProcessCommandLine.GetProcessCommandLineByPid(Process.GetCurrentProcess().Id, out status) + ", Status=" + status);

        //    Console.WriteLine("WindowsVersion=" + ProcessCommandLine.InitializeWindowsVersion());

        //    Console.WriteLine("Status=", ProcessCommandLine.GetProcessHandle(Process.GetCurrentProcess().Id, out IntPtr processhadle) + ", ProcessHadle=" + processhadle);

        //    Console.WriteLine(ProcessCommandLine.GetProcessCommandLine(processhadle, out status) + ", Status=" + status);
        //}
    }
}
