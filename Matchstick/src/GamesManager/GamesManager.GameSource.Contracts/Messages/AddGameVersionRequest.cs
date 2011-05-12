using System;
using System.Runtime.Serialization;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using MGDF.GamesManager.GameSource.Contracts.Messages;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class AddGameVersionRequest: AuthenticatedRequestBase
    {
        [DataMember]
        public GameVersionBase NewGameVersion { get; set; }

        [DataMember]
        public bool PublishOnUploadComplete { get; set; }

        [DataMember]
        public string GameUid { get; set; }
    }
}