using System;
using System.Collections.Generic;
using System.Text;

namespace MGDF.GamesManager.Common.Framework
{
  public interface IIdentityGenerator
  {
    Guid GenerateIdentity();
  }

  public class IdentityGenerator : IIdentityGenerator
  {
    public static IIdentityGenerator Current
    {
      get
      {
        return ServiceLocator.Current.Get<IIdentityGenerator>();
      }
      set
      {
        ServiceLocator.Current.Register(value);
      }
    }

    public Guid GenerateIdentity()
    {
      return Guid.NewGuid();
    }
  }
}
