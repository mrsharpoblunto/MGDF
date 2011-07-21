using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Model;

namespace MGDF.GamesManager.Tests.Common.Mocks
{
    public class MockGameExplorer: IGameExplorer
    {
        private List<string> _installed = new List<string>();


        public bool IsInstalled(string gdfBinPath)
        {
            return _installed.Contains(gdfBinPath);
        }

        public void AddGameTask(Guid guid, TaskCount count, string name, string path, string arguments)
        {
        }

        public void UninstallGame(string gdfBinPath)
        {
            _installed.Remove(gdfBinPath);
        }

        public Guid InstallGame(string gdfBinPath, string gameInstallPath, string gameExecutable, string arguments)
        {
            _installed.Add(gdfBinPath);
            return Guid.NewGuid();
        }
    }
}