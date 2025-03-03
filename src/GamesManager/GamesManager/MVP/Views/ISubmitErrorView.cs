using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Controls;

namespace MGDF.GamesManager.MVP.Views
{
  public interface ISubmitErrorView : IView
  {
    event EventHandler CopyLogOutput;
    event EventHandler SendLogOutput;

    string Message { set; }
    string SupportType { get; set; }
    string SupportUrl { get; set; }
  }

  public interface ISubmitErrorS3View : IView
  {
    event EventHandler SendLogOutput;
    bool Sending { set; }
  }
}