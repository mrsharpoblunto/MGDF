using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Xml;
using System.Xml.Schema;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Contracts.Entities;
using IArchiveFile=MGDF.GamesManager.Model.Factories.IArchiveFile;

namespace MGDF.GamesManager.Model.Entities.XmlEntities
{
    /// <summary>
    /// provides a boase class for entities loaded from an xml file (either on a physical or virtual filesystem) and validated with an 
    /// xsd schema
    /// </summary>
    public abstract class XmlEntity: IEntity 
    {
        protected List<string> _errorCollection = new List<string>();
        protected XmlReader _xmlReader;

        public List<string> ErrorCollection
        {
            get { return _errorCollection; }
        }

        public bool IsValid
        {
            get { return _errorCollection.Count == 0; }
        }

        protected XmlEntity()
        {  
        }

        protected XmlEntity(XmlReader reader)
        {
            try
            {
                Load(reader);
            }
            catch (Exception e)
            {
                _errorCollection.Add(e.Message);
            }
        }

        protected XmlEntity(string filename,string schema)
        {
            Load(filename, schema);
        }

        protected void Load(string filename, string schema)
        {
            using (var stream = FileSystem.Current.GetFile(filename).OpenStreamWithTimeout(FileMode.Open, FileAccess.Read, FileShare.Read))
            {
                using (var reader = !string.IsNullOrEmpty(schema) ? XmlReader.Create(stream, CreateSchemaSettings(schema)) : XmlReader.Create(stream))
                {
                    DoLoad(reader);
                }
            }
        }

        protected XmlEntity(IArchiveFile file, string schema)
        {
            using (var stream = file.OpenStream())
            {
                using (var reader = XmlReader.Create(stream, CreateSchemaSettings(schema)))
                {
                    DoLoad(reader);
                }
            }
        }

        protected XmlReaderSettings CreateSchemaSettings(string schema)
        {
            // Create the XmlSchemaSet class.
            XmlSchemaSet sc = new XmlSchemaSet();

            Stream xsdStream = Assembly.GetExecutingAssembly().GetManifestResourceStream("MGDF.GamesManager.Model.Schemas." + schema);
            if (xsdStream == null) FileSystem.Current.GetFile(FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, Constants.SchemaDir, schema)).OpenStream(FileMode.Open);

            using (xsdStream)
            {
                sc.Add(null, XmlReader.Create(xsdStream));
            }
 
            // Set the validation settings.
            XmlReaderSettings settings = new XmlReaderSettings {ValidationType = ValidationType.Schema, Schemas = sc};
            settings.ValidationEventHandler += Validation;
            return settings;
        }

        private void Validation(object sender, ValidationEventArgs args)
        {
            _errorCollection.Add(args.Message);
        }

        protected void DoLoad(XmlReader reader)
        {
            try
            {
                Load(reader);
            }
            catch (Exception e)
            {
                _errorCollection.Add(e.Message);
            }          
        }

        protected abstract void Load(XmlReader reader);
    }
}
