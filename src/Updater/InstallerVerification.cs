using System;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;

namespace MGDF.Updater
{
  public static class InstallerVerification
  {
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct TrustFile
    {
      public uint Size;
      [MarshalAs(UnmanagedType.LPWStr)] public string Path;
      public IntPtr File;
      public IntPtr Subject;
    }
    [StructLayout(LayoutKind.Sequential)]
    private struct TrustData
    {
      public uint Size;
      public IntPtr PolicyCallback, SipClient;
      public uint Ui, Revocation, Choice;
      public IntPtr File;
      public uint StateAction;
      public IntPtr State, Url;
      public uint Flags, Context;
    }
    [DllImport("wintrust.dll", ExactSpelling = true, CharSet = CharSet.Unicode)]
    private static extern int WinVerifyTrust(IntPtr window, [In] ref Guid action, ref TrustData data);

    private static string Signer(string path)
    {
      var file = new TrustFile { Size = (uint)Marshal.SizeOf(typeof(TrustFile)), Path = path };
      var pointer = Marshal.AllocHGlobal(Marshal.SizeOf(file));
      try
      {
        Marshal.StructureToPtr(file, pointer, false);
        var data = new TrustData { Size = (uint)Marshal.SizeOf(typeof(TrustData)), Ui = 2,
          Choice = 1, File = pointer, Flags = 0x1000 };
        var action = new Guid("00AAC56B-CD44-11d0-8CC2-00C04FC295EE");
        if (WinVerifyTrust(new IntPtr(-1), ref action, ref data) != 0) return null;
        using (var certificate = new X509Certificate2(X509Certificate.CreateFromSignedFile(path)))
          return certificate.Thumbprint;
      }
      finally { Marshal.DestroyStructure<TrustFile>(pointer); Marshal.FreeHGlobal(pointer); }
    }

    public static FileStream Open(string installer, string hash, string updater)
    {
      var file = new FileStream(installer, FileMode.Open, FileAccess.Read, FileShare.Read);
      try
      {
        using (var algorithm = SHA256.Create())
        {
          var actual = BitConverter.ToString(algorithm.ComputeHash(file)).Replace("-", "");
          if (!string.Equals(hash, actual, StringComparison.OrdinalIgnoreCase))
            throw new InvalidDataException("The installer hash does not match.");
        }
        var signer = Signer(updater);
        if (signer != null && !string.Equals(signer, Signer(installer), StringComparison.OrdinalIgnoreCase))
          throw new InvalidDataException("The installer signer does not match.");
        return file;
      }
      catch { file.Dispose(); throw; }
    }
  }
}
