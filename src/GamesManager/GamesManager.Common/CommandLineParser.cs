using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

namespace MGDF.GamesManager.Common
{
  public class CommandLineParser
  {
    // Variables
    private readonly StringDictionary _parameters;

    // Constructor
    public CommandLineParser(string[] args, bool trimFirst = true)
    {
      _parameters = new StringDictionary();
      Regex remover = new Regex(@"^['""]?(.*?)['""]?$", RegexOptions.IgnoreCase | RegexOptions.Compiled);
      string parameter = null;

      // Valid parameters forms:
      // {-}param{ }((",')value(",'))
      // Examples: -param1 value1 -param2
      for (int i = trimFirst ? 1 : 0; i < args.Length; ++i)
      {
        string arg = args[i];

        if (arg.StartsWith("-"))
        {
          // The last parameter is still waiting. With no value, set it to true.
          if (parameter != null)
          {
            if (!_parameters.ContainsKey(parameter)) _parameters.Add(parameter, "true");
          }
          parameter = arg.Substring(1);
        }
        else if (parameter != null)
        {
          if (!_parameters.ContainsKey(parameter))
          {
            _parameters.Add(parameter, remover.Replace(arg, "$1"));
          }
          parameter = null;
        }
      }
      // In case a parameter is still waiting
      if (parameter != null)
      {
        if (!_parameters.ContainsKey(parameter)) _parameters.Add(parameter, "true");
      }
    }

    // Retrieve a parameter value if it exists
    public string this[string param]
    {
      get { return (_parameters[param]); }
    }
  }
}
