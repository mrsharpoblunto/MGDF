using System.Xml;

namespace MGDF.GamesManager.Model.Entities.XmlEntities.FragmentEntities
{
    internal abstract class XmlFragmentEntity
    {
        protected XmlFragmentEntity()
        {
            
        }
        
        protected XmlFragmentEntity(XmlReader reader)
        {
            Load(reader);
        }

        protected abstract void Load(XmlReader reader);
    }
}
