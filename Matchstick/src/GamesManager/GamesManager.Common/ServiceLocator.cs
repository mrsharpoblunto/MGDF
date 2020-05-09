using System;
using System.Collections.Generic;

namespace MGDF.GamesManager.Common
{
  public class ServiceLocator
  {
    private static readonly ServiceLocator _current = new ServiceLocator();
    private readonly Dictionary<Type, object> _services = new Dictionary<Type, object>();

    private ServiceLocator()
    {
    }

    public static ServiceLocator Current
    {
      get { return _current; }
    }

    public void Register<T>(T t)
    {
      if (_services.ContainsKey(typeof(T)))
      {
        _services.Remove(typeof(T));
      }
      _services.Add(typeof(T), t);
    }

    public T Get<T>()
    {
      if (_services.ContainsKey(typeof(T)))
      {
        return (T)_services[typeof(T)];
      }
      else
      {
        return default(T);
      }
    }

    public void Clear()
    {
      _services.Clear();
    }
  }
}