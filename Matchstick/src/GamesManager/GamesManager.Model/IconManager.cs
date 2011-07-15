using System;
using System.Collections.Generic;
using System.Drawing.IconLib;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;

namespace MGDF.GamesManager.Model
{
    public interface IIconManager
    {
        void CreateIcon(string name, string source, string destination);
    }

    public class IconManager:IIconManager
    {
        public static IIconManager Current
        {
            get
            {
                return ServiceLocator.Current.Get<IIconManager>();
            }
            set
            {
                ServiceLocator.Current.Register(value);
            }
        }

        public void CreateIcon(string name,string source, string destination)
        {
            MultiIcon icon = new MultiIcon();
            SingleIcon single = icon.Add(name);
            single.CreateFrom(source, IconOutputFormat.FromWinXP);
            icon.SelectedIndex = 0;
            icon.Save(destination, MultiIconFormat.ICO);
        }
    }
}