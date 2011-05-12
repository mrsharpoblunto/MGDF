using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.MVP.Views;

namespace MGDF.GamesManager.MVP.Presenters
{
    class SubmitCoreErrorPresenter: PresenterBase<ISubmitErrorView>
    {
        private readonly string _detail;
        private readonly IGame _game;

        public SubmitCoreErrorPresenter(IGame game, string message, string detail)
        {
            _game = game;
            _detail = detail;
            View.Message = message;
            View.SupportEmail = string.IsNullOrEmpty(game.SupportEmail) ? Constants.SupportEmail : game.SupportEmail;
            View.CopyLogOutput += View_CopyLogOutput;
            View.EmailLogOutput += View_EmailLogOutput;
        }

        void View_EmailLogOutput(object sender, EventArgs e)
        {
            try
            {
                Process.Start("mailto:" + (string.IsNullOrEmpty(_game.SupportEmail) ? Constants.SupportEmail : _game.SupportEmail) + "?subject=Core Error Report (" + _game.Uid + ")");
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
            sb.AppendLine(EnvironmentSettings.Current.UserDirectory + "\\minidump.dmp");
            sb.AppendLine();
            sb.AppendLine("This file contains important debugging information to allow us to better understand what caused the problem you have experienced");
            sb.AppendLine();
            sb.AppendLine();
            sb.AppendLine();
            sb.AppendLine("System Information");
            sb.AppendLine("==================");
            sb.AppendLine("OS: " + EnvironmentSettings.Current.OSName);
            sb.AppendLine("OS Architecture: " + EnvironmentSettings.Current.OSArchitecture + " bit");
            sb.AppendLine("RAM: " + EnvironmentSettings.Current.TotalMemory);
            sb.AppendLine();
            sb.AppendLine("Details");
            sb.AppendLine("=======");
            sb.AppendLine("Game: "+_game.Uid+" "+_game.Version);
            sb.AppendLine(_detail);
            sb.AppendLine();
            sb.AppendLine("Log output");
            sb.AppendLine("==========");

            IFile coreLog = FileSystem.Current.GetFile(Path.Combine(EnvironmentSettings.Current.UserDirectory, "corelog.txt"));
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


            var runner = new CrossThreadRunner();
            runner.RunInSTA(() => Clipboard.SetText(sb.ToString()));
        }
    }
}