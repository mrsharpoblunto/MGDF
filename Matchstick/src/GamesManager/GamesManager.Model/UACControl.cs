using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Security.Principal;
using System.Windows.Forms;
using Microsoft.Win32;

namespace MGDF.GamesManager.Model
{
  public class UACControl
  {
    public enum TOKEN_INFORMATION_CLASS
    {
      TokenUser = 1,
      TokenGroups,
      TokenPrivileges,
      TokenOwner,
      TokenPrimaryGroup,
      TokenDefaultDacl,
      TokenSource,
      TokenType,
      TokenImpersonationLevel,
      TokenStatistics,
      TokenRestrictedSids,
      TokenSessionId,
      TokenGroupsAndPrivileges,
      TokenSessionReference,
      TokenSandBoxInert,
      TokenAuditPolicy,
      TokenOrigin,
      TokenElevationType,
      TokenLinkedToken,
      TokenElevation,
      TokenHasRestrictions,
      TokenAccessInformation,
      TokenVirtualizationAllowed,
      TokenVirtualizationEnabled,
      TokenIntegrityLevel,
      TokenUIAccess,
      TokenMandatoryPolicy,
      TokenLogonSid,
      MaxTokenInfoClass
    }

    public enum TOKEN_ELEVATION_TYPE
    {
      TokenElevationTypeDefault = 1,
      TokenElevationTypeFull,
      TokenElevationTypeLimited
    }

    [DllImport("user32")]
    private static extern UInt32 SendMessage(IntPtr hWnd, UInt32 msg, UInt32 wParam, UInt32 lParam);

    [DllImport("advapi32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    static extern bool OpenProcessToken(IntPtr ProcessHandle, UInt32 DesiredAccess, out IntPtr TokenHandle);

    [DllImport("advapi32.dll", SetLastError = true)]
    public static extern bool GetTokenInformation(IntPtr TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, IntPtr TokenInformation, uint TokenInformationLength, out uint ReturnLength);

    private const int BCM_FIRST = 0x1600;
    private const int BCM_SETSHIELD = (BCM_FIRST + 0x000C);
    private const string uacRegistryKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System";
    private const string uacRegistryValue = "EnableLUA";
    private static uint STANDARD_RIGHTS_READ = 0x00020000;
    private static uint TOKEN_QUERY = 0x0008;
    private static uint TOKEN_READ = (STANDARD_RIGHTS_READ | TOKEN_QUERY);

    public static bool IsVistaOrHigher()
    {
      return Environment.OSVersion.Version.Major >= 6;
    }

    public static bool IsUacEnabled()
    {
      RegistryKey uacKey = Registry.LocalMachine.OpenSubKey(uacRegistryKey, false);
      bool result = uacKey.GetValue(uacRegistryValue).Equals(1);
      return result;
    }


    /// <summary>
    /// Checks if the process is elevated
    /// </summary>
    /// <returns>If is elevated</returns>
    public static bool IsAdmin()
    {
      if (IsVistaOrHigher() && IsUacEnabled())
      {
        IntPtr tokenHandle;
        if (!OpenProcessToken(Process.GetCurrentProcess().Handle, TOKEN_READ, out tokenHandle))
        {
          throw new ApplicationException("Could not get process token.  Win32 Error Code: " + Marshal.GetLastWin32Error());
        }

        TOKEN_ELEVATION_TYPE elevationResult = TOKEN_ELEVATION_TYPE.TokenElevationTypeDefault;

        int elevationResultSize = Marshal.SizeOf((int)elevationResult);
        uint returnedSize = 0;
        IntPtr elevationTypePtr = Marshal.AllocHGlobal(elevationResultSize);

        bool success = GetTokenInformation(tokenHandle, TOKEN_INFORMATION_CLASS.TokenElevationType, elevationTypePtr, (uint)elevationResultSize, out returnedSize);
        if (success)
        {
          elevationResult = (TOKEN_ELEVATION_TYPE)Marshal.ReadInt32(elevationTypePtr);
          bool isProcessAdmin = elevationResult == TOKEN_ELEVATION_TYPE.TokenElevationTypeFull;
          return isProcessAdmin;
        }
        else
        {
          throw new ApplicationException("Unable to determine the current elevation.");
        }
      }
      else
      {
        WindowsIdentity identity = WindowsIdentity.GetCurrent();
        WindowsPrincipal principal = new WindowsPrincipal(identity);
        bool result = principal.IsInRole(WindowsBuiltInRole.Administrator);
        return result;
      }
    }

    /// <summary>
    /// Add a shield icon to a button
    /// </summary>
    /// <param name="b">The button</param>
    public static void AddShieldToButton(Button b)
    {
      b.FlatStyle = FlatStyle.System;
      SendMessage(b.Handle, BCM_SETSHIELD, 0, 0xFFFFFFFF);
    }

    /// <summary>
    /// Restart the current process with administrator credentials
    /// </summary>
    public static void RestartElevated(string arguments)
    {
      ProcessStartInfo startInfo = new ProcessStartInfo();
      startInfo.UseShellExecute = true;
      startInfo.WorkingDirectory = Environment.CurrentDirectory;
      startInfo.FileName = Application.ExecutablePath;
      startInfo.Arguments = arguments;
      if (IsVistaOrHigher()) startInfo.Verb = "runas";
      try
      {
        Process p = Process.Start(startInfo);
      }
      catch (System.ComponentModel.Win32Exception)
      {
        return; //If cancelled, do nothing
      }

      Application.Exit();
    }
  }
}