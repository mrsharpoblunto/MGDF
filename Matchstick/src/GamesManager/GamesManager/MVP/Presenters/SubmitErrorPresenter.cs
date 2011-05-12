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
    class SubmitErrorPresenter: PresenterBase<ISubmitErrorView>
    {
        private readonly string _detail;

        public SubmitErrorPresenter(string message, string detail)
        {
            _detail = detail;
            View.Message = message;
            View.SupportEmail = Constants.SupportEmail;
            View.CopyLogOutput += View_CopyLogOutput;
            View.EmailLogOutput += View_EmailLogOutput;
        }

        void View_EmailLogOutput(object sender, EventArgs e)
        {
            try 
            {
                Process.Start("mailto:" + Constants.SupportEmail + "?subject=GamesManager Error Report");
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

            sb.AppendLine("IMPORTANT: In order to help us find out the source of this problem, please let us know what you were doing before the crash, or inform us of the steps to reproduce the problem.");
            sb.AppendLine();
            sb.AppendLine("System Information");
            sb.AppendLine("==================");
            sb.AppendLine("OS: " + EnvironmentSettings.Current.OSName);
            sb.AppendLine("OS Architecture: " + EnvironmentSettings.Current.OSArchitecture + " bit");
            sb.AppendLine("RAM: " + EnvironmentSettings.Current.TotalMemory);
            sb.AppendLine();
            sb.AppendLine("Details");
            sb.AppendLine("=======");
            sb.AppendLine(_detail);
            sb.AppendLine();
            sb.AppendLine("GamesManager Client Log output");
            sb.AppendLine("==============================");

            IFile gamesManagerLog = FileSystem.Current.GetFile(Path.Combine(EnvironmentSettings.Current.UserDirectory, "GamesManagerLog.txt"));
            if (gamesManagerLog.Exists)
            {
                using (var stream = gamesManagerLog.OpenStream(FileMode.Open, FileAccess.Read, FileShare.Read))
                {
                    using (TextReader reader = new StreamReader(stream))
                    {
                        sb.Append(reader.ReadToEnd());
                    }
                }
            }

            sb.AppendLine();
            sb.AppendLine("GamesManager Service Log output");
            sb.AppendLine("==============================");
            IFile gamesServiceLog = FileSystem.Current.GetFile(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "GamesManagerServiceLog.txt"));
            if (gamesServiceLog.Exists)
            {
                using (var stream = gamesServiceLog.OpenStream(FileMode.Open, FileAccess.Read, FileShare.Read))
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