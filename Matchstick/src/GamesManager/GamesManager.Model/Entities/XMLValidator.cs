using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Text;
using System.Xml;
using System.Xml.Schema;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Model.Entities
{
    public class XmlValidator
    {
        private List<string> _errors = new List<string>();
        private string _fileName;

        public bool IsXmlFileValid(IArchiveFile file,string xmlSchema,List<string> errorCollection)
        {
            _errors = errorCollection;
            _fileName = file.Name;

            try
            {
                using (Stream stream = file.OpenStream())
                {
                    XmlReaderSettings settings = GetSchemaSettings(xmlSchema);
                    XmlReader reader = XmlReader.Create(stream, settings);

                    while (reader.Read())
                    {
                    }
                }
            }
            catch (Exception e)
            {
                _errors.Add("In file " + _fileName+": "+ e.Message);
            }

            return _errors.Count == 0;
        }

        private XmlReaderSettings GetSchemaSettings(string xmlSchema)
        {

            // Create the XmlSchemaSet class.
            XmlSchemaSet sc = new XmlSchemaSet();

            Stream xsdStream = Assembly.GetExecutingAssembly().GetManifestResourceStream("MGDF.GamesManager.Model.Schemas." + xmlSchema);
            if (xsdStream == null) FileSystem.Current.GetFile(FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, Resources.SchemaDir, xmlSchema)).OpenStream(FileMode.Open);

            using (xsdStream)
            {
                sc.Add(null, XmlReader.Create(xsdStream));
            }

            // Set the validation settings.
            XmlReaderSettings settings = new XmlReaderSettings { ValidationType = ValidationType.Schema, Schemas = sc };
            settings.ValidationEventHandler += Validation;
            return settings;
        }

        private void Validation(object sender, ValidationEventArgs args)
        {
            _errors.Add("In file " + _fileName + ": " + args.Message);
        }
    }
}