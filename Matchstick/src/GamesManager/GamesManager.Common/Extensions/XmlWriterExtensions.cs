using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;

namespace MGDF.GamesManager.Common.Extensions
{
    public static class XmlWriterExtensions
    {
        public static void AddNonDefaultElementString(this XmlWriter writer, string localName, string value)
        {
            if (!string.IsNullOrEmpty(value))
            {
                writer.WriteElementString(localName, value);
            }
        }
    }
}
