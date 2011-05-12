using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.Globalization;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.GameSource.Model
{
    public class DeveloperMetadata
    {
        [StringLength(255)]
        [Required(ErrorMessage = "Name is required")]
        public string Name { get; set; }

        [StringLength(255)]
        [Required(ErrorMessage = "Uid is required")]
        [RegularExpression("^([A-Za-z0-9]|\\-)*$",ErrorMessage ="Uid's can only contain alphanumeric characters and the '-' symbol")]
        public string Uid { get; set; }

        [StringLength(255)]
        public string Homepage { get; set; }
    }

    [MetadataType(typeof(DeveloperMetadata))]
    public partial class Developer
    {
        public static Developer Create(string developerName,string uid,string developerHomepage)
        {
            Developer developer = new Developer();
            developer.Name = developerName;
            developer.Uid = uid;
            developer.Homepage = developerHomepage;
            developer.GenerateDeveloperKeySecretKeyPair();

            return developer;
        }

        public void GenerateDeveloperKeySecretKeyPair()
        {
            //create the developer key from a base64 encoded guid
            DeveloperKey = Convert.ToBase64String(Guid.NewGuid().ToByteArray());

            //create the secret key from 4 combined guids (16 bytes each = 64 byte sha256 key) base64 encoded into a string
            byte[] secretKeyBytes = new byte[64];
            for (int i=0;i<4;++i)
            {
                byte[] guidBytes = Guid.NewGuid().ToByteArray();
                guidBytes.CopyTo(secretKeyBytes,i*16);
            }

            SecretKey = Convert.ToBase64String(secretKeyBytes);
        }
    }
}
