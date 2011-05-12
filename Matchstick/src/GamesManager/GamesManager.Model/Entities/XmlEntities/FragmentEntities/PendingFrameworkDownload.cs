using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Xml;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Helpers;

namespace MGDF.GamesManager.Model.Entities.XmlEntities.FragmentEntities
{
    class PendingFrameworkDownload: PendingDownloadBase
    {
        public Version Version { get; private set;}
        public string DestinationFileName { get { return _destinationFileName; } }

        public PendingFrameworkDownload(XmlReader reader, ReaderWriterLockSlim syncLock)
            : base(reader,syncLock)
        {
            Name = "Downloading Framework Update";
        }

        public PendingFrameworkDownload(AvailableVersion version, ReaderWriterLockSlim syncLock)
            : base(syncLock)
        {
            _destinationFileName = Path.Combine(Constants.FrameworkUpdatesDir, version.Version + ".part");
            _sourceUrl = version.Url.ToString();
            Version = version.Version;
            Name = "Downloading Framework Update";
        }

        public override IPendingOperation NewOperationOnFinish(ReaderWriterLockSlim readerWriterLock, out bool reloadGame)
        {
            reloadGame = false;
            return null;
        }

        protected override void Work()
        {
            base.Work();

            if (!PausePending && !ErrorPending)
            {
                //rename the finished file to .msi
                IFile destinationFile = FileSystem.Current.GetFile(_destinationFileName);
                destinationFile.MoveTo(_destinationFileName.Replace(".part", ".exe"));
            }
        }

        protected override void Load(XmlReader reader)
        {
            while (reader.Read())
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.EndElement:
                        if (reader.Name.Equals("pendingframeworkdownload"))
                        {
                            return;
                        }
                        break;

                    case XmlNodeType.Element:
                        if (!LoadBase(reader))
                        {
                            switch (reader.Name)
                            {
                                case "destinationfilename":
                                    _destinationFileName = reader.ReadString();
                                    break;

                                case "sourceurl":
                                    _sourceUrl = reader.ReadString();
                                    break;

                                case "version":
                                    Version = new Version(reader.ReadString());
                                    break;
                            }
                        }
                        break;
                }
            }
        }

        public override void Save(XmlWriter writer)
        {
            writer.WriteStartElement("pendingframeworkdownload");
            SaveBase(writer);
            writer.WriteElementString("destinationfilename", _destinationFileName);
            writer.WriteElementString("sourceurl", _sourceUrl);
            writer.WriteElementString("version", Version.ToString());
            writer.WriteEndElement();
        }


        protected override void Finished()
        {
        }

        protected override void Error()
        {
        }

        protected override void Paused()
        {
        }
    }
}
