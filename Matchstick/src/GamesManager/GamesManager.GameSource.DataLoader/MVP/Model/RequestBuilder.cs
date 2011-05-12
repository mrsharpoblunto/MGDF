using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Security.Cryptography;
using System.ServiceModel;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Contracts;
using MGDF.GamesManager.GameSource.Contracts.Messages;

namespace MGDF.GamesManager.GameSource.DataLoader.MVP.Model
{
    public static class RequestBuilder
    {
        public static T Build<T>() where T : AuthenticatedRequestBase, new()
        {
            var request = new T { Timestamp = Cryptography.GenerateTimestamp(TimeService.Current.Now) };
            request.CNonce = Guid.NewGuid();
            request.HMAC = Cryptography.GenerateHMac(Settings.Instance.SecretKey, request.CNonce.ToString(), request.Timestamp);
            request.DeveloperKey = Settings.Instance.DeveloperKey;
            return request;
        }
    }
}
