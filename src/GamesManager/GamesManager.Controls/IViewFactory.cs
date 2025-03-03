using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.Controls
{
  public interface IViewFactory
  {
    IView LatestView { get; }
    TINTERFACETYPE CreateView<TINTERFACETYPE>() where TINTERFACETYPE : IView;
    void RegisterView<TINTERFACETYPE, TCONCRETETYPE>()
        where TINTERFACETYPE : IView
        where TCONCRETETYPE : IView;
  }
}
