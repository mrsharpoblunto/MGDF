using System;
using System.Collections.Generic;
using Newtonsoft.Json.Linq;

namespace MGDF.GamesManager.Model.Entities
{
    public class Update: JsonEntity
    {
        public Version UpdateMinVersion { get; private set; }
        public Version UpdateMaxVersion { get; private set; }

        private readonly List<string> _removeFiles = new List<string>();
        public List<string> RemoveFiles { get { return _removeFiles; } }

        public Game UpdateTarget
        {
            get { return _updateTarget; }
        }

        private readonly Game _updateTarget;

        public Update(Game upgadeTarget, IArchiveFile updateFile)
            : base(updateFile)
        {
            _updateTarget = upgadeTarget;

            if (UpdateMinVersion >= upgadeTarget.Version || UpdateMaxVersion >= upgadeTarget.Version)
            {
                ErrorCollection.Add("UpdateMinVersion and UpdateMaxVersion must be less than the upgraded game version");
            }
            else if (UpdateMinVersion > UpdateMaxVersion)
            {
                ErrorCollection.Add("UpdateMinVersion must be less than or equal to UpdateMaxVersion");                
            }
        }

        protected override void Load(JObject json)
        {
            UpdateMinVersion = new Version(json.ReadRequiredValue("updateMinVersion"));
            UpdateMaxVersion = new Version(json.ReadRequiredValue("updateMaxVersion"));
             
            foreach (var child in json["removeFiles"].Values<string>())
            {
                RemoveFiles.Add(child);
            }
        }
    }
}