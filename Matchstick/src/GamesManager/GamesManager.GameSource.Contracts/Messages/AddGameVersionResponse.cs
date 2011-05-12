using System;
using System.Runtime.Serialization;
using MGDF.GamesManager.GameSource.Contracts.Messages;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class AddGameVersionResponse: ResponseBase
    {
        [DataMember]
        public Guid GameFragmentId { get; set; }

        [DataMember]
        public string UploadHandler { get; set; }

        [DataMember]
        public int MaxUploadPartSize { get; set; }
    }
}