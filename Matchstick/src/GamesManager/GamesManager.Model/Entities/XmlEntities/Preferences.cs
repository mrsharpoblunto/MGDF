using System;
using System.Collections.Generic;
using System.IO;
using System.Xml;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Entities.XmlEntities.FragmentEntities;
using MGDF.GamesManager.Common.Framework;
using IArchiveFile=MGDF.GamesManager.Model.Factories.IArchiveFile;

namespace MGDF.GamesManager.Model.Entities.XmlEntities
{
    internal class Preferences: XmlEntity, IPreferences
    {
        private readonly List<IPreference> _preferenceCollection = new List<IPreference>();

        public Preferences(string filename) : base(filename, "preferences.xsd")
        {
        }

        public Preferences(IArchiveFile filename) : base(filename, "preferences.xsd")
        {
        }

        public void LoadAdditionalPreferences(string filename)
        {
            using (var stream = FileSystem.Current.GetFile(filename).OpenStream(FileMode.Open))
            {
                _xmlReader = XmlReader.Create(stream, CreateSchemaSettings("preferences.xsd"));
                try
                {
                    Load(_xmlReader);
                }
                catch (Exception e)
                {
                    _errorCollection.Add(e.Message);
                }
            }
        }

        public List<IPreference> PreferenceCollection
        {
            get { return _preferenceCollection; }
        }

        public void Save(string filename)
        {
            XmlWriterSettings settings = new XmlWriterSettings
                                             {
                                                 OmitXmlDeclaration = false,
                                                 Indent = true,
                                                 NewLineChars = "\r\n",
                                                 IndentChars = "\t"
                                             };

            using (var stream = FileSystem.Current.GetFile(filename).OpenStream(FileMode.Create))
            {
                XmlWriter writer = XmlWriter.Create(stream, settings);

                writer.WriteStartDocument();
                writer.WriteStartElement("mgdf", "preferences", "http://schemas.matchstickframework.org/2007/preferences");
                writer.WriteAttributeString("xmlns", "xsi", null, "http://www.w3.org/2001/XMLSchema-instance");
                foreach (IPreference preference in _preferenceCollection)
                {
                    writer.WriteStartElement("preference");
                    writer.WriteElementString("name", preference.Name);
                    writer.WriteElementString("value", preference.Value);
                    writer.WriteEndElement();
                }
                writer.WriteEndElement();
                writer.WriteEndDocument();
                writer.Close();
            }
        }

        protected override void Load(XmlReader reader)
        {
            while (reader.Read())
            {
                // parse based on NodeType
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element:
                        switch (reader.Name)
                        {
                            case "preference":
                                IPreference pref = new Preference(reader);

                                //replace any existing prefs with the newly defined one
                                if (_preferenceCollection.Exists(p => pref.Name == p.Name))
                                {
                                    _preferenceCollection.RemoveAll(p => pref.Name == p.Name);
                                }
                                _preferenceCollection.Add(pref);
                                break;
                        }
                        break;
                }
            }
        }


    }
}
