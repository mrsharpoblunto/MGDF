using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Common
{
    public static class Cryptography
    {
        public static string GenerateTimestamp(DateTime messageTimestamp)
        {
            return messageTimestamp.ToString("yyyy-MM-ddTHH:mm:ssZ", CultureInfo.InvariantCulture);
        }

        public static DateTime ParseTimestamp(string messageTimestamp)
        {
            return DateTime.ParseExact(messageTimestamp, "yyyy-MM-ddTHH:mm:ssZ", CultureInfo.InvariantCulture, DateTimeStyles.AssumeUniversal).ToUniversalTime();
        }

        public static string GenerateHMac(string secretKey, string contentMd5, DateTime messageTimestamp)
        {
            return GenerateHMac(secretKey, contentMd5, GenerateTimestamp(messageTimestamp));
        }

        public static string GenerateHMac(string secretKey, DateTime messageTimestamp)
        {
            return GenerateHMac(secretKey, string.Empty, GenerateTimestamp(messageTimestamp));
        }

        public static string GenerateHMac(string secretKey, string cnonce, string messageTimestamp)
        {
            HMACSHA256 hmacsha256 = new HMACSHA256(Convert.FromBase64String(secretKey));
            byte[] computedHash = hmacsha256.ComputeHash(UTF8Encoding.UTF8.GetBytes(messageTimestamp + cnonce));
            return Convert.ToBase64String(computedHash);
        }

        public static bool Authenticate(string hmac, string messageTimestamp, string secretKey, int maxTimeDifference)
        {
            return Authenticate(hmac, messageTimestamp, string.Empty,secretKey, maxTimeDifference);
        }

        public static bool Authenticate(string hmac, string messageTimestamp, string cnonce, string secretKey,int maxTimeDifference)
        {
            try
            {
                string computedHMac = GenerateHMac(secretKey, cnonce, messageTimestamp);

                //does the hmac agree with the server generated version.
                if (computedHMac.Equals(hmac))
                {
                    DateTime timeStamp = ParseTimestamp(messageTimestamp);
                    DateTime now = TimeService.Current.Now;

                    //is the timestamp within the last x minutes server time (prevents reuse of timestamps)
                    TimeSpan timeDiff = new TimeSpan(now.Ticks - timeStamp.Ticks);
                    if (Math.Abs(timeDiff.TotalMinutes) <= maxTimeDifference)
                    {
                        return true;
                    }
                }
            }
            catch (Exception)
            {
            }
            return false;
        }
    }
}
