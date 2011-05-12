using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.StatisticsService.Model.Configuration
{
    public interface IConfig
    {
        string ConnectionString { get; }
    }
}