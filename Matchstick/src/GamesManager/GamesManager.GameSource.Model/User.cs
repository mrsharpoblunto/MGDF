using System;
using System.Collections.Generic;
using System.Linq;
using System.Security;
using System.Security.Cryptography;
using System.Text;
using MGDF.GamesManager.Common.Extensions;

namespace MGDF.GamesManager.GameSource.Model
{
    public partial class User
    {
        public static User Create(string username, string custom)
        {
            User user = new User();
            user.Id = Guid.NewGuid();
            user.Name = username;
            user.Custom = custom;

            return user;
        }

        public static User Create(string username, string realm,string password)
        {
            User user = new User();
            user.Id = Guid.NewGuid();
            user.Name = username;
            user.PasswordHash = GeneratePasswordDigest(user.Name,realm, password);

            return user;
        }

        public bool Authenticate(string realm,string password)
        {
            return !string.IsNullOrEmpty(PasswordHash) && GeneratePasswordDigest(Name,realm,password) == PasswordHash;
        }

        public static string GeneratePasswordDigest(string username,string realm,string newPassword)
        {
            return (username + ":" + realm + ":" + newPassword).ComputeMD5();
        }
    }
}
