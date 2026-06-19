using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

namespace MGDF.GamesManager.Common
{
  public class CommandLineParser
  {
    // prefix identifying which environment variables are parameters,
    // e.g. MGDF_GAMEDIROVERRIDE maps to the "gamediroverride" parameter
    private const string EnvPrefix = "MGDF_";

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

      MergeEnvironmentVariables();
    }

    // Fold MGDF_ prefixed environment variables in as parameters. Anything
    // already present on the command line wins, so this only supplies defaults.
    private void MergeEnvironmentVariables()
    {
      foreach (DictionaryEntry envar in Environment.GetEnvironmentVariables())
      {
        string name = envar.Key.ToString();
        if (!name.StartsWith(EnvPrefix, StringComparison.OrdinalIgnoreCase)) continue;

        string key = name.Substring(EnvPrefix.Length).ToLowerInvariant();
        if (key.Length == 0 || _parameters.ContainsKey(key)) continue;

        _parameters.Add(key, envar.Value == null ? "true" : envar.Value.ToString());
      }
    }

    // Retrieve a parameter value if it exists
    public string this[string param]
    {
      get { return (_parameters[param]); }
    }
  }
}
