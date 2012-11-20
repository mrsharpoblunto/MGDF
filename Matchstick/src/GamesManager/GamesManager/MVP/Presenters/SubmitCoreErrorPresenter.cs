using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Management;
using System.Reflection;
using System.Text;
using System.Windows.Forms;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model.Entities;
using MGDF.GamesManager.MVP.Views;

namespace MGDF.GamesManager.MVP.Presenters
{
    class SubmitCoreErrorPresenter: PresenterBase<ISubmitErrorView>
    {
        private readonly string _detail;
        private readonly Game _game;

        public SubmitCoreErrorPresenter(Game game, string message, string detail)
        {
            _game = game;
            _detail = detail;
            View.Message = message;
            View.SupportEmail = string.IsNullOrEmpty(game.SupportEmail) ? Resources.SupportEmail : game.SupportEmail;
            View.CopyLogOutput += View_CopyLogOutput;
            View.EmailLogOutput += View_EmailLogOutput;
        }

        void View_EmailLogOutput(object sender, EventArgs e)
        {
            try
            {
                Process.Start("mailto:" + (string.IsNullOrEmpty(_game.SupportEmail) ? Resources.SupportEmail : _game.SupportEmail) + "?subject=Core Error Report (" + _game.Uid + ")");
            }
            catch (Exception ex)
            {
                Message.Show("No email client installed");
                Logger.Current.Write(ex, "No program configured to open mailto: links");
            }
        }

        void View_CopyLogOutput(object sender, EventArgs e)
        {
            StringBuilder sb = new StringBuilder();

            sb.AppendLine("IMPORTANT: In order to help us find out the source of this problem, please attach the following file to this email before sending.");
            sb.AppendLine();
            sb.AppendLine(Resources.GameUserDir +  "\\minidump.dmp");
            sb.AppendLine();
            sb.AppendLine("This file contains important debugging information to allow us to better understand what caused the problem you experienced");
            sb.AppendLine();
            sb.AppendLine();
            sb.AppendLine();

            sb.AppendLine("System Information");
            sb.AppendLine("==================");
            sb.AppendLine("MGDF Version: " + Assembly.GetExecutingAssembly().GetName().Version);
            sb.AppendLine("OS: " + EnvironmentSettings.Current.OSName);
            sb.AppendLine("OS Architecture: " + EnvironmentSettings.Current.OSArchitecture + " bit");
            sb.AppendLine("RAM: " + EnvironmentSettings.Current.TotalMemory);
            sb.AppendLine("Processor Count: " + EnvironmentSettings.Current.ProcessorCount);
            sb.AppendLine();

            sb.AppendLine("Video Controllers");
            sb.AppendLine("=================");
            //iterate through all the video controllers and output all the relevant info
            //including driver versions etc.
            ManagementObjectSearcher searcher = new ManagementObjectSearcher("select * from Win32_VideoController");
            int controller = 1;
            foreach (ManagementObject obj in searcher.Get())
            {
                sb.AppendLine(String.Format("Video Controller {0}:", controller));
                foreach (PropertyData property in obj.Properties)
                {
                    if (property.Value != null)
                    {
                        sb.AppendLine(property.Name + ": " + property.Value.ToString());
                    }
                }
                sb.AppendLine();
                controller++;
            }

            sb.AppendLine("Details");
            sb.AppendLine("=======");
            sb.AppendLine("Game: "+_game.Uid+" "+_game.Version);
            sb.AppendLine(_detail);
            sb.AppendLine();

            sb.AppendLine("Log output");
            sb.AppendLine("==========");
            IFile coreLog = FileSystem.Current.GetFile(Path.Combine(Resources.GameUserDir, "corelog.txt"));
            if (!coreLog.Exists)
            {
                coreLog = FileSystem.Current.GetFile(Path.Combine(EnvironmentSettings.Current.UserDirectory, "corelog.txt"));
            }
            if (coreLog.Exists)
            {
                using (var stream = coreLog.OpenStream(FileMode.Open, FileAccess.Read, FileShare.Read))
                {
                    using (TextReader reader = new StreamReader(stream))
                    {
                        sb.Append(reader.ReadToEnd());
                    }
                }
            }
            else
            {
                sb.Append("No Log output");
            }


            var runner = new CrossThreadRunner();
            runner.RunInSTA(() => Clipboard.SetText(sb.ToString()));
        }
    }
}