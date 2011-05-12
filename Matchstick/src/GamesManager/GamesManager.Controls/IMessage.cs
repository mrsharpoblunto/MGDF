using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace MGDF.GamesManager.Controls
{
    public interface IMessage: IView
    {
        void Show(string text);
        void Show(string text, string caption);
        DialogResult Show(string text, string caption, MessageBoxButtons buttons);
    }
}
