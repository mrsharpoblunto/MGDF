using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Tests.Common.Mocks
{
    public class MockEnvironmentSettings: IEnvironmentSettings
    {
        private readonly Dictionary<string,string> _table;

        public MockEnvironmentSettings()
        {
            _table = new Dictionary<string, string>();
            OSArchitecture = 64;
        }

        public IDictionary GetEnvironmentVariables()
        {
            return _table;
        }

        public string UserTempDirectory
        {
            get { return "c:\\temp"; }
        }

        public string AppDirectory
        {
            get { return "c:\\program files\\MGDF"; }
        }

        public string CommonDirectory
        {
            get { return @"C:\Documents and Settings\All Users\Application Data"; }
        }

        public string UserDirectory
        {
            get { return @"C:\Documents and Settings\user\Local Settings\Application Data"; }
        }

        public string CommonDesktopDirectory
        {
            get { return "c:\\Documents and Settings\\user\\desktop"; }
        }

        public string CommonStartMenuDirectory
        {
            get { return "c:\\Documents and Settings\\user\\start menu"; }
        }

        public string SubstituteEnvironmentVariables(string path)
        {
            foreach (DictionaryEntry envar in GetEnvironmentVariables())
            {
                if (path.Contains("%" + envar.Key + "%"))
                {
                    path = path.Replace("%" + envar.Key + "%", envar.Value.ToString());
                }
            }

            return path;
        }

        public string OSName
        {
            get { return "Windows XP"; }
        }

        Version IEnvironmentSettings.OSVersion
        {
            get { return new Version(5, 0, 0, 0); }
        }

        public int OSArchitecture
        { 
            get;
            set;
        }

        public ulong TotalMemory
        {
            get { return 2048; }
        }

        public void AddEnvironmentVariables(string key,string value)
        {
            _table.Add(key,value);
        }
    }
}