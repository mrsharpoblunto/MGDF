using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Text;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class Error
    {
        public const string AuthorizationFailed = "AuthorizationFailed";
        public const string AccessDenied = "AccessDenied";
        public const string InvalidArguments = "InvalidArguments";
        public const string UnknownError = "UnknownError";

        [DataMember]
        public string Message { get; set; }

        [DataMember]
        public string Code { get; set; }
    }

    [DataContract]
    public class ResponseBase
    {
        private List<Error> _errors  =new List<Error>();

        [DataMember]
        public List<Error> Errors
        {
            get { return _errors; }
            set { _errors = value; }
        }
    }
}
