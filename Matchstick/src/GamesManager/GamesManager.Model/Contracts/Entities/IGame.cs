using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Text;
using MGDF.GamesManager.Model.Contracts;

namespace MGDF.GamesManager.Model.Contracts.Entities
{
    public interface IGame : IEntity, IXmlWritable
    {
        string Uid { get; }
        string Name { get; }
        Version Version { get; }
        int InterfaceVersion { get; }
        string Description { get; }
        string DeveloperUid { get; }
        string DeveloperName { get; }
        string Homepage { get; }
        string SupportEmail { get; }
        string GameSourceService { get; }
        string StatisticsService { get; }
        string StatisticsPrivacyPolicy { get; }
        byte[] GameIconData { get; }
    }
}