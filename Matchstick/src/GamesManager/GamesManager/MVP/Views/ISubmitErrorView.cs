using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Controls;

namespace MGDF.GamesManager.MVP.Views
{
    public interface ISubmitErrorEmailView : IView
    {
        event EventHandler CopyLogOutput;
        event EventHandler EmailLogOutput;

        string Message { set; }
        string SupportEmail { set; }
    }

	public interface ISubmitErrorS3View : IView
	{
		event EventHandler SendLogOutput;
		bool Sending { set; }
	}
}