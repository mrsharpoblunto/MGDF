using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json;

namespace MGDF.GamesManager.Model.Entities
{
  /// <summary>
  /// provides a base class for entities loaded from a json file (either on a physical or virtual filesystem)
  /// </summary>
  public abstract class JsonEntity
  {
    protected List<string> _errorCollection = new List<string>();

    public List<string> ErrorCollection
    {
      get { return _errorCollection; }
    }

    public bool IsValid
    {
      get { return _errorCollection.Count == 0; }
    }

    protected JsonEntity(string filename)
    {
      Load(filename);
    }

    protected JsonEntity()
    {
    }

    protected void Load(string filename)
    {
      try
      {
        using (var stream = FileSystem.Current.GetFile(filename).OpenStreamWithTimeout(FileMode.Open, FileAccess.Read, FileShare.Read))
        {
          using (var reader = new StreamReader(stream))
          {
            var json = JObject.Parse(reader.ReadToEnd());
            DoLoad(json);
          }
        }
      }
      catch (Exception ex)
      {
        _errorCollection.Add("Unable to load data from " + filename + " - " + ex);
        Logger.Current.Write(ex, "Unable to load data from " + filename);
      }
    }

    protected JsonEntity(IArchiveFile file)
    {
      try
      {
        using (var stream = file.OpenStream())
        {
          using (var reader = new StreamReader(stream))
          {
            var json = JObject.Parse(reader.ReadToEnd());
            DoLoad(json);
          }
        }
      }
      catch (Exception ex)
      {
        _errorCollection.Add("Unable to load data from " + file.Name + " - " + ex);
        Logger.Current.Write(ex, "Unable to load data from " + file.Name);
      }
    }

    protected void DoLoad(JObject json)
    {
      try
      {
        Load(json);
      }
      catch (Exception e)
      {
        _errorCollection.Add(e.Message);
        Logger.Current.Write(e, "Unable to load JSON data " + e.Message);
      }
    }

    protected abstract void Load(JObject json);
  }

  public static class JsonExtensions
  {
    public static void WriteOptionalValue(this JsonWriter writer, string name, string value)
    {
      if (!string.IsNullOrEmpty(value))
      {
        writer.WritePropertyName(name);
        writer.WriteValue(value);
      }
    }

    public static void WriteRequiredValue(this JsonWriter writer, string name, string value)
    {
      writer.WritePropertyName(name);
      writer.WriteValue(value);
    }

    public static T ReadRequiredValue<T>(this JObject json, string name)
    {
      if (json[name] == null)
      {
        name = name.ToLowerInvariant();
      }
      if (json[name] == null)
      {
        throw new Exception("Required attribute '" + name + "' missing");
      }
      return json[name].Value<T>();
    }

    public static T ReadOptionalValue<T>(this JObject json, string name) where T : class
    {
      if (json[name] == null)
      {
        name = name.ToLowerInvariant();
      }
      return json[name]?.Value<T>();
    }

    public static T ReadRequiredValue<T>(this JToken json, string name)
    {
      if (json[name] == null)
      {
        name = name.ToLowerInvariant();
      }
      if (json[name] == null)
      {
        throw new Exception("Required attribute '" + name + "' missing");
      }
      return json[name].Value<T>();
    }

    public static bool ReadOptionalValue<T>(this JToken json, string name, ref T value)
    {
      if (json[name] == null)
      {
        name = name.ToLowerInvariant();
      }
      if (json[name] != null)
      {
        value = json[name].Value<T>();
        return true;
      }
      return false;
    }


    public static JToken ReadToken(this JToken json, string name)
    {
      if (json[name] == null)
      {
        name = name.ToLowerInvariant();
      }
      return json[name];
    }
  }
}