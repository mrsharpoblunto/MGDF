using System;
using System.Collections.Generic;
using System.Text;

namespace MGDF.GamesManager.Common.Extensions
{
  public static class GuidExtensions
  {
    public static string Encode(this Guid guid)
    {
      string encoded = Convert.ToBase64String(guid.ToByteArray());
      encoded = encoded
          .Replace("/", "_")
          .Replace("+", "-");
      return encoded.Substring(0, 22);
    }

    public static Guid Decode(this string shortGuid)
    {
      shortGuid = shortGuid
          .Replace("_", "/")
          .Replace("-", "+");
      byte[] buffer = Convert.FromBase64String(shortGuid + "==");
      return new Guid(buffer);
    }
  }
}
