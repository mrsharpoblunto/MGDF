using System;
using System.Collections.Generic;
using System.Text;
using MGDF.GamesManager.Common;

namespace MGDF.GamesManager.Common.Framework
{
    public interface ITimeService
    {
        DateTime Now { get; }
    }

    public class TimeService: ITimeService
    {
        public static ITimeService Current
        {
            get
            {
                return ServiceLocator.Current.Get<ITimeService>();
            }
            set
            {
                ServiceLocator.Current.Register(value);
            }
        }

        public DateTime Now
        {
            get { return DateTime.UtcNow; }
        }
    }
}
