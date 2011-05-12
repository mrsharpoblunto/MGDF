using System;
using System.Data;
using System.Configuration;
using System.Web;
using System.Web.Security;
using System.Web.UI;
using System.Web.UI.HtmlControls;
using System.Web.UI.WebControls;
using System.Web.UI.WebControls.WebParts;
using MGDF.GamesManager.Common.Extensions;

namespace MGDF.GamesManager.GameSource.Model
{
    public static class Headers
    {
        public const string Location = "Location";
        public const string AcceptRanges = "Accept-Ranges";
        public const string AcceptRangesBytes = "bytes";
        public const string ContentType = "Content-Type";
        public const string ContentRange = "Content-Range";
        public const string ContentLength = "Content-Length";
        public const string ContentDisposition = "Content-Disposition";
        public const string EntityTag = "ETag";
        public const string LastModified = "Last-Modified";
        public const string Range = "Range";
        public const string IfRange = "If-Range";
        public const string IfMatch = "If-Match";
        public const string IfNoneMatch = "If-None-Match";
        public const string IfModifiedSince = "If-Modified-Since";
        public const string IfUnmodifiedSince = "If-Unmodified-Since";
        public const string UnlessModifiedSince = "Unless-Modified-Since";
        public const string WWWAuthenticate = "WWW-Authenticate";
    }

    public static class Constants
    {
        public const int WriteBufferSize = 16384;

        public static string GameFolder
        {
            get { return "/games"; }
        }

        public static string GameVersionDownload(string baseUrl, Guid gameVersionId)
        {
            return string.Format("{0}{1}/{2}.mza", baseUrl, GameFolder, gameVersionId.Encode());
        }
    }
}