using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.Common.Extensions
{
  public static class FunctionalExtensions
  {
    public static List<T> Map<T, TU>(this IEnumerable<TU> input, Func<TU, T> mapFunc)
    {
      var output = new List<T>();
      foreach (var i in input)
      {
        output.Add(mapFunc(i));
      }
      return output;
    }
  }
}
