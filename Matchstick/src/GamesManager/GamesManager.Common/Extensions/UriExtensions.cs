using System;
using System.Collections.Generic;
using System.Text;

namespace MGDF.GamesManager.Common.Extensions
{
    public static class UriExtensions
    {
        public static string AbsoluteUri(this string testString, string baseUriString)
        {
            Uri testUri = new Uri(testString, UriKind.RelativeOrAbsolute);
            if (!testUri.IsAbsoluteUri)
            {
                if (!baseUriString.StartsWith("http", StringComparison.InvariantCultureIgnoreCase))
                {
                    baseUriString = "http://" + baseUriString;
                }
                Uri baseUri = new Uri(baseUriString, UriKind.Absolute);
                testUri = new Uri(baseUri, testUri);
            }
            return testUri.ToString();
        }
    }
}
