using System.Xml;
using MGDF.GamesManager.Model.Contracts.Entities;

namespace MGDF.GamesManager.Model.Entities.XmlEntities.FragmentEntities
{
    internal class Preference : XmlFragmentEntity, IPreference
    {
        public string Name { get; private set; }
        public string Value { get; set; }

        public Preference(XmlReader reader) : base(reader)
        {
        }

        protected override void Load(XmlReader reader)
        {
            while (reader.Read())
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.EndElement:
                        if (reader.Name.Equals("preference"))
                        {
                            return;
                        }
                        break;

                    case XmlNodeType.Element:
                        switch (reader.Name)
                        {
                            case "name":
                                Name = reader.ReadString();
                                break;

                            case "value":
                                Value = reader.ReadString();
                                break;
                        }
                        break;
                }
            }
        }
    }
}
