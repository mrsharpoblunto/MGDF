using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Management;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using MGDF.GamesManager.Common;

namespace MGDF.GamesManager.Common.Framework
{
    public interface IEnvironmentSettings
    {
        IDictionary GetEnvironmentVariables();
        string SubstituteEnvironmentVariables(string path);

        string OSName { get; }
        Version OSVersion { get; }
        int OSArchitecture { get; }
        ulong TotalMemory { get; }

        string AppDirectory { get; }

        string UserDirectory { get; }

        string CommonDesktopDirectory { get;}
        string CommonStartMenuDirectory { get; }
    }

    public class EnvironmentSettings : IEnvironmentSettings
    {
        public static IEnvironmentSettings Current
        {
            get
            {
                return ServiceLocator.Current.Get<IEnvironmentSettings>();
            }
            set
            {
                ServiceLocator.Current.Register(value);
            }
        }

        private int _osArchitecture = -1;

        [return: MarshalAs(UnmanagedType.Bool)]
        [DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        static extern bool GlobalMemoryStatusEx([In, Out] MEMORYSTATUSEX lpBuffer);

        [return: MarshalAs(UnmanagedType.Bool)]
        [DllImport("kernel32.dll", SetLastError = true, CallingConvention = CallingConvention.Winapi)]
        public static extern bool IsWow64Process([In] IntPtr hProcess, [Out] out bool lpSystemInfo);

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
        private class MEMORYSTATUSEX
        {
            public uint dwLength;
            public uint dwMemoryLoad;
            public ulong ulTotalPhys;
            public ulong ulAvailPhys;
            public ulong ulTotalPageFile;
            public ulong ulAvailPageFile;
            public ulong ulTotalVirtual;
            public ulong ulAvailVirtual;
            public ulong ulAvailExtendedVirtual;
            public MEMORYSTATUSEX()
            {
                dwLength = (uint)Marshal.SizeOf(typeof(MEMORYSTATUSEX));
            }
        }

        public IDictionary GetEnvironmentVariables()
        {
            return Environment.GetEnvironmentVariables();
        }

        public string SubstituteEnvironmentVariables(string path)
        {
            foreach (DictionaryEntry envar in GetEnvironmentVariables())
            {
                if (path.Contains("%" + envar.Key + "%"))
                {
                    path = path.Replace("%" + envar.Key + "%", envar.Value.ToString());
                }
            }

            return path;
        }

        public string OSName
        {
            get { return Environment.OSVersion.ToString(); }
        }

        public Version OSVersion
        {
            get { return Environment.OSVersion.Version; }
        }

        public int OSArchitecture
        {
            get
            {
                if (_osArchitecture == -1)
                {
                    _osArchitecture = IntPtr.Size * 8;

                    //double check thet its not a 32 bit app running under wow64
                    if (_osArchitecture == 32)
                    {
                        Logger.Current.Write(LogInfoLevel.Info, "32 bit architecture detected, checking if process is Wow64...");
                        try
                        {
                            Logger.Current.Write(LogInfoLevel.Info, "Getting current process handle...");
                            Process p = Process.GetCurrentProcess();
                            IntPtr handle = p.Handle;
                            bool isWow64;
                            Logger.Current.Write(LogInfoLevel.Info, "Invoking IsWow64Process Check...");
                            bool success = IsWow64Process(handle, out isWow64);
                            if (success)
                            {
                                if (isWow64)
                                {
                                    Logger.Current.Write(LogInfoLevel.Info, "Process is Wow64, OS architecture is 64 bit");
                                    _osArchitecture = 64;
                                }
                                else
                                {
                                    Logger.Current.Write(LogInfoLevel.Info, "Process is not Wow64, OS architecture is 32 bit");
                                }
                            }
                            else
                            {
                                Logger.Current.Write(LogInfoLevel.Info, "IsWow64Process API Call not supported, assuming 32 bit OS Architecture");
                            }
                        }
                        catch (Exception ex)
                        {
                            Logger.Current.Write(LogInfoLevel.Error, "Unable to check if IsWow64Process, assuming 32 bit OS Architecture: " + ex.ToString());
                        }
                    }

                }
                return _osArchitecture;
            }
        }

        public ulong TotalMemory
        {
            get
            {
                MEMORYSTATUSEX memStatus = new MEMORYSTATUSEX();
                if (GlobalMemoryStatusEx(memStatus))
                {
                    return memStatus.ulTotalPhys / 1048576;
                }
                return 0;
            }
        }

        public string AppDirectory
        {
            get
            {
                return Path.GetDirectoryName(Assembly.GetExecutingAssembly().GetModules()[0].FullyQualifiedName);
            }
        }

        public string UserDirectory
        {
            get
            {
                string localAppData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);

                if (!System.IO.Directory.Exists(Path.Combine(localAppData, "MGDF")))
                {
                    System.IO.Directory.CreateDirectory(Path.Combine(localAppData, "MGDF"));
                }
                if (!System.IO.Directory.Exists(Path.Combine(localAppData,"MGDF\\"+Constants.InterfaceVersion)))
                {
                    System.IO.Directory.CreateDirectory(Path.Combine(localAppData, "MGDF\\"+Constants.InterfaceVersion));
                }
                return Path.Combine(localAppData, "MGDF\\" + Constants.InterfaceVersion);
            }
        }

        [DllImport("shell32.dll")]
        private static extern bool SHGetSpecialFolderPath(IntPtr hwndOwner,[Out] StringBuilder lpszPath, int nFolder, bool fCreate);


        private const int COMMON_DESKTOPDIRECTORY = 0x0019;
        private const int COMMON_PROGRAMS = 0x0017;

        public string CommonDesktopDirectory
        {
            get
            {
                var path = new StringBuilder(260);
                SHGetSpecialFolderPath(IntPtr.Zero, path, COMMON_DESKTOPDIRECTORY, false);
                return path.ToString();
            }
        }

        public string CommonStartMenuDirectory
        {
            get
            {
                var path = new StringBuilder(260);
                SHGetSpecialFolderPath(IntPtr.Zero, path, COMMON_PROGRAMS, false);
                return path.ToString();
            }
        }
    }
}