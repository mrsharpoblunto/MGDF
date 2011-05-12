using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Xml;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Helpers;
using MGDF.GamesManager.Model.Services;

namespace MGDF.GamesManager.Model.Entities.XmlEntities.FragmentEntities
{
    class PendingGameUninstall: PendingOperationBase
    {
        private string _uid;
        private string _developerName;
        private string _gameName;

        public PendingGameUninstall(XmlReader reader, ReaderWriterLockSlim readerWriterLock)
            : base(reader, readerWriterLock)
        {
            Name = "Uninstalling";
        }

        public PendingGameUninstall(IGame game, ReaderWriterLockSlim readerWriterLock)
            : base(readerWriterLock)
        {
            _uid = game.Uid;
            _developerName = game.DeveloperName;
            _gameName = game.Name;
            Name = "Uninstalling";
        }

        protected override void Load(XmlReader reader)
        {
            while (reader.Read())
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element:
                        if (!LoadBase(reader))
                        {
                            switch (reader.Name)
                            {
                                case "uid":
                                    _uid = reader.ReadString();
                                    break;
                                case "developerName":
                                    _developerName = reader.ReadString();
                                    break;
                                case "gameName":
                                    _gameName = reader.ReadString();
                                    break;
                            }
                        }
                        break;
                }
            }
        }

        public override void Save(XmlWriter writer)
        {
            writer.WriteStartElement("pendinggameuninstall");
            SaveBase(writer);
            writer.WriteElementString("uid", _uid);
            writer.WriteElementString("developerName", _developerName);
            writer.WriteElementString("gameName", _gameName);
            writer.WriteEndElement();
        }

        public override IPendingOperation NewOperationOnFinish(ReaderWriterLockSlim readerWriterLock, out bool reloadGame)
        {
            reloadGame = true;
            return null;
        }

        protected override void Error()
        {
        }

        protected override void Paused()
        {
        }

        protected override void Finished()
        {
        }

        protected override void Work()
        {
            UpdateTotal(5);
            UpdateProgress(0);
            if (!PausePending && GameExplorer.Current.IsInstalled(Constants.GameDefinitionFileBin(_uid)))
            {
                GameExplorer.Current.UninstallGame(Constants.GameDefinitionFileBin(_uid));
            }

            UpdateProgress(1);

            string desktopShortcutPath = Path.Combine(EnvironmentSettings.Current.CommonDesktopDirectory, _gameName + ".lnk");
            if (!PausePending && ShortcutManager.Current.HasShortcut(desktopShortcutPath))
            {
                ShortcutManager.Current.DeleteShortcut(desktopShortcutPath);
            }

            UpdateProgress(2);

            var startMenuFolder = FileSystem.Current.GetDirectory(Path.Combine(EnvironmentSettings.Current.CommonStartMenuDirectory, _developerName));
            string shortcutPath = Path.Combine(startMenuFolder.FullName, _gameName + ".lnk");
            if (!PausePending && ShortcutManager.Current.HasShortcut(shortcutPath))
            {
                ShortcutManager.Current.DeleteShortcut(shortcutPath);
                //also remvoe the parent folder if it has no contents.
                if (startMenuFolder.GetFiles("*.*").Count==0 && startMenuFolder.GetSubDirectories("*").Count==0) startMenuFolder.DeleteWithTimeout();
            }

            UpdateProgress(3);

            if (!PausePending && FileSystem.Current.DirectoryExists(Constants.GameDir(_uid)))
            {
                FileSystem.Current.GetDirectory(Constants.GameDir(_uid)).DeleteWithTimeout(); //remove the game contents
            }

            UpdateProgress(4);

            if (!PausePending)
            {
                InstalledProgramsHelper.RemoveFromInstalledPrograms(_uid);
            }

            UpdateProgress(5);
        }
    }
}
