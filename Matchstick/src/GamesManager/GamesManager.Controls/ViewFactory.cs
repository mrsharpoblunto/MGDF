using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using MGDF.GamesManager.Common;

namespace MGDF.GamesManager.Controls
{
  public class ViewFactory : IViewFactory
  {
    private readonly Dictionary<Type, Type> _registeredViews = new Dictionary<Type, Type>();
    private IView _latestView;

    private ViewFactory()
    {
      RegisterView<IMessage, Message>();
    }

    private static readonly ViewFactory _current = new ViewFactory();
    public static IViewFactory Current
    {
      get
      {
        return _current;
      }
    }

    public IView LatestView
    {
      get { return _latestView; }
    }

    public TINTERFACETYPE CreateView<TINTERFACETYPE>() where TINTERFACETYPE : IView
    {
      Type concreteViewType = _registeredViews[typeof(TINTERFACETYPE)];
      if (concreteViewType != null)
      {
        var instance = (TINTERFACETYPE)Activator.CreateInstance(concreteViewType);
        _latestView = instance;
        return instance;
      }
      return default(TINTERFACETYPE);
    }

    public void RegisterView<TINTERFACETYPE, TCONCRETETYPE>()
        where TINTERFACETYPE : IView
        where TCONCRETETYPE : IView
    {
      if (!_registeredViews.ContainsKey(typeof(TINTERFACETYPE)))
      {
        _registeredViews.Add(typeof(TINTERFACETYPE), typeof(TCONCRETETYPE));
      }
      else
      {
        _registeredViews[typeof(TINTERFACETYPE)] = typeof(TCONCRETETYPE);
      }
    }
  }
}
