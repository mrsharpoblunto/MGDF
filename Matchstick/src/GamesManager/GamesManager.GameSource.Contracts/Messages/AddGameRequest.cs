using System;
using System.Runtime.Serialization;
using MGDF.GamesManager.GameSource.Contracts.Entities;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class AddGameRequest: AuthenticatedRequestBase
    {
        [DataMember]
        public EditGame NewGame { get; set; }
    }
}