using System;
using System.Collections.Generic;
using System.Xml;
using MGDF.GamesManager.Model.Contracts.Entities;
using IArchiveFile=MGDF.GamesManager.Model.Factories.IArchiveFile;

namespace MGDF.GamesManager.Model.Entities.XmlEntities
{
    internal class Update: XmlEntity, IUpdate 
    {
        public Version UpdateMinVersion { get; private set; }
        public Version UpdateMaxVersion { get; private set; }

        private readonly List<string> _removeFiles = new List<string>();
        public List<string> RemoveFiles { get { return _removeFiles; } }
        public string UpdateDescription { get; private set; }

        public IGame UpdateTarget
        {
            get { return _updateTarget; }
        }

        private readonly IGame _updateTarget;

        public Update(IGame upgadeTarget, IArchiveFile updateFile)
            : base(updateFile, "update.xsd")
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

        protected override void Load(XmlReader reader)
        {
            while (reader.Read())
            {
                // parse based on NodeType
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element:
                        switch (reader.Name)
                        {
                            case "updatedescription":
                                UpdateDescription = reader.ReadString();
                                break;

                            case "updateminversion":
                                UpdateMinVersion = new Version(reader.ReadString());
                                break;

                            case "updatemaxversion":
                                UpdateMaxVersion = new Version(reader.ReadString());
                                break;

                            case "file":
                                RemoveFiles.Add(reader.ReadString());
                                break;
                        }
                        break;
                }
            }
        }
    }
}
