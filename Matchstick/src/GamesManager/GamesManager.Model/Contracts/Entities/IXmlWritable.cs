using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Model.Contracts.Entities
{
    public interface IXmlWritable
    {
        void Save(string filename);
    }
}