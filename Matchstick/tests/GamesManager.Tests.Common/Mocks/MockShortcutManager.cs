using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Services;

namespace MGDF.GamesManager.Tests.Common.Mocks
{
    public class MockShortcutManager: IShortcutManager
    {
        public void CreateShortcut(string path, string filename, string args, string icon)
        {
            FileSystem.Current.GetFile(path).WriteText("SHORTCUT");
        }

        public void DeleteShortcut(string path)
        {
            var shortcut = FileSystem.Current.GetFile(path);
            shortcut.Delete();
        }

        public bool HasShortcut(string path)
        {
            return FileSystem.Current.FileExists(path);
        }
    }
}