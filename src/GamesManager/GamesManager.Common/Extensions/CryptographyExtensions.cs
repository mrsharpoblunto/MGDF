using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Common.Extensions
{
  public static class CryptographyExtensions
  {
    public static string ConvertToBase16(this byte[] input)
    {
      StringBuilder result = new StringBuilder();
      foreach (byte b in input)
      {
        result.Append(b.ToString("x2"));
      }
      return result.ToString();
    }

    public static byte[] ConvertFromBase16(this string input)
    {
      List<byte> output = new List<byte>();
      for (int i = 0; i < input.Length; i += 2)
      {
        output.Add(byte.Parse(input.Substring(i, 2), NumberStyles.HexNumber));
      }
      return output.ToArray();
    }

    public static string ComputeMD5(this string input)
    {
      using (HashAlgorithm hashAlg = MD5.Create())
      {
        return ComputeHash(input, hashAlg);
      }
    }

    public static string ComputeHash(this string input, HashAlgorithm hashAlg)
    {
      byte[] hash = hashAlg.ComputeHash(Encoding.UTF8.GetBytes(input));
      return ConvertToBase16(hash);
    }

    public static string ComputeMD5(this IFile file)
    {
      using (Stream stream = file.OpenStream(FileMode.Open, FileAccess.Read, FileShare.Read))
      {
        return stream.ComputeMD5();
      }
    }

    public static string ComputeMD5(this Stream input)
    {
      using (HashAlgorithm hashAlg = MD5.Create())
      {
        return ComputeHash(input, hashAlg);
      }
    }

    public static string ComputeMD5(this byte[] input)
    {
      using (HashAlgorithm hashAlg = MD5.Create())
      {
        return ComputeHash(input, hashAlg);
      }
    }

    public static string ComputeMD5(this byte[] input, int count)
    {
      using (HashAlgorithm hashAlg = MD5.Create())
      {
        return ComputeHash(input, count, hashAlg);
      }
    }

    public static string ComputeHash(this Stream input, HashAlgorithm hashAlg)
    {
      byte[] hash = hashAlg.ComputeHash(input);
      return ConvertToBase16(hash);
    }

    public static string ComputeHash(this byte[] input, HashAlgorithm hashAlg)
    {
      byte[] hash = hashAlg.ComputeHash(input);
      return ConvertToBase16(hash);
    }

    public static string ComputeHash(this byte[] input, int count, HashAlgorithm hashAlg)
    {
      byte[] hash = hashAlg.ComputeHash(input, 0, count);
      return ConvertToBase16(hash);
    }
  }
}