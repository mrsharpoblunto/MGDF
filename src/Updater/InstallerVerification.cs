using System;
using System.IO;
using System.Security.Cryptography;

namespace MGDF.Updater
{
  public static class InstallerVerification
  {
    public static FileStream Open(string installer, string hash)
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
        return file;
      }
      catch { file.Dispose(); throw; }
    }
  }
}
