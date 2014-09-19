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
                _errorCollection.Add("Unable to load data from "+filename+" - "+ex);
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

        public static string ReadRequiredValue(this JObject json, string name)
        {
            if (json[name] == null) throw new Exception("Required attribute '" + name + "' missing");
            return json[name].Value<string>();
        }

        public static string ReadOptionalValue(this JObject json, string name)
        {
            return json[name] == null ? null : json[name].Value<string>();
        }

        public static string ReadRequiredValue(this JToken json, string name)
        {
            string result = json[name].Value<string>();
            if (string.IsNullOrEmpty(result)) throw new Exception("Required attribute '" + name + "' missing");
            return result;
        }
    }
}