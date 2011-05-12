using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Controls;

namespace MGDF.GamesManager.FrameworkUpdater
{
    public partial class UpdatingForm : ViewBase
    {
        private readonly string _file;
        private readonly CommandLineRepeater _repeater;
        private bool overrideClose = true;

        public UpdatingForm(CommandLineRepeater line, string file)
        {
            InitializeComponent();
            _file = file;
            _repeater = line;

            Shown += UpdatingForm_Shown;
        }

        void UpdatingForm_Shown(object sender, EventArgs e)
        {
            Thread updaterThread = new Thread(DoUpdate);
            updaterThread.Start();
        }

        private void DoUpdate()
        {
            Process p = Process.Start(_file,"/S");
            p.WaitForExit();

            try
            {
                FileSystem.Current.GetFile(_file).DeleteWithTimeout();
            }
            catch (Exception)
            {
                //file still in use, don't bother deleting.
            }

            Invoke(() =>
                       {
                           Process.Start(Constants.GamesManagerExecutable, _repeater.ToString());
                           overrideClose = false;
                           Close();
                       });
        }

        private void UpdatingForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            e.Cancel = overrideClose;
        }
    }
}