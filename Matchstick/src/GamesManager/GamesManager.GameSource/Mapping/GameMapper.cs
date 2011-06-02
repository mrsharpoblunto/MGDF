using System;
using System.Collections.Generic;
using System.Globalization;
using System.Web;
using System.Linq;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using MGDF.GamesManager.GameSource.Model;
using MGDF.GamesManager.GameSource.Model.Configuration;
using MGDF.GamesManager.ServerCommon;
using Constants=MGDF.GamesManager.GameSource.Model.Constants;
using Developer=MGDF.GamesManager.GameSource.Contracts.Entities.Developer;
using Game=MGDF.GamesManager.GameSource.Model.Game;
using GameVersion=MGDF.GamesManager.GameSource.Contracts.Entities.GameVersion;
using UserGame=MGDF.GamesManager.GameSource.Contracts.Entities.UserGame;

namespace MGDF.GamesManager.GameSource.Mapping
{
    public static class GameMapper
    {
        public static Model.GameVersion GetLatestVersion(Game game)
        {
            return GetLatestVersion(game, GameSourceRepository.Current);
        }

        public static Model.GameVersion GetLatestVersion(Game game, IRepository repository)
        {
            List<Model.GameVersion> versions = (from gv in repository.Get<Model.GameVersion>()
                                                where gv.GameId == game.Id && !gv.IsUpdate
                                                select gv).ToList();
            versions.Sort((a, b) => b.Version.CompareTo(a.Version));

            return versions.FirstOrDefault();
        }

        public static List<UserGame> MapToContractEntities(Model.Developer authenticatedDeveloper,IEnumerable<Model.Game> games)
        {
            List<UserGame> contractEntities = new List<UserGame>();
            foreach (Model.Game model in games)
            {
                UserGame contract = new UserGame();
                MapToContractEntity(contract,model);

                Model.GameVersion latestVersion = GetLatestVersion(model);

                contract.LatestVersion = new UserGameVersion();
                if (MapToContractEntity(contract.LatestVersion, latestVersion))
                {
                    contract.LatestVersion.AuthenticatedDownloadURL = Constants.GameVersionDownload(Config.Current.BaseUrl,latestVersion.Id);

                    string timestamp = Cryptography.GenerateTimestamp(TimeService.Current.Now);
                    contract.LatestVersion.AuthenticatedDownloadURL += "?" + Common.Constants.QueryString.Hmac + "=" + Cryptography.GenerateHMac(authenticatedDeveloper.SecretKey, latestVersion.Id.ToString().ToLowerInvariant(), timestamp) + "&" + Common.Constants.QueryString.Timestamp + "=" + timestamp;

                }

                contractEntities.Add(contract);
            }

            return contractEntities;
        }

        public static List<Contracts.Entities.Game> MapToContractEntities(IEnumerable<Model.Game> games)
        {
            List<Contracts.Entities.Game> contractEntities = new List<Contracts.Entities.Game>();
            foreach (Model.Game model in games)
            {
                Contracts.Entities.Game contract = new Contracts.Entities.Game();
                MapToContractEntity(contract, model);

                Model.GameVersion latestVersion = GetLatestVersion(model);

                contract.LatestVersion = new GameVersion();
                if (MapToContractEntity(contract.LatestVersion, latestVersion))
                {
                    contract.LatestVersion.DownloadURL = Constants.GameVersionDownload(Config.Current.BaseUrl, latestVersion.Id);
                }

                contractEntities.Add(contract);
            }

            return contractEntities;
        }

        public static List<GameVersion> MapToContractEntities(IEnumerable<Model.GameVersion> gameVersions)
        {
            List<GameVersion> contractEntities = new List<GameVersion>();
            foreach (Model.GameVersion model in gameVersions)
            {
                GameVersion contract = new GameVersion();
                if (MapToContractEntity(contract, model))
                {
                    contract.IsComplete = !string.IsNullOrEmpty(model.GameDataId);
                    contract.DownloadURL = Constants.GameVersionDownload(Config.Current.BaseUrl, model.Id);
                }

                contractEntities.Add(contract);
            }

            return contractEntities;
        }

        public static GameVersionUpdate MapToContractEntity(Model.Game game, Model.GameVersion gameVersion)
        {
            if (gameVersion == null)
            {
                return null;
            }

            GameVersionUpdate contractEntity = new GameVersionUpdate();
            if (MapToContractEntity(contractEntity, gameVersion))
            {
                contractEntity.DownloadURL = Constants.GameVersionDownload(Config.Current.BaseUrl, gameVersion.Id);
                contractEntity.Game = new GameUpdate();
            }
            MapToContractEntity(contractEntity.Game, game);

            return contractEntity;
        }

        private static bool MapToContractEntity(GameVersionBase contract,Model.GameVersion model)
        {
            if (model == null)
            {
                return false;
            }

            contract.Id = model.Id;
            contract.Description = model.Description;
            contract.IsUpdate = model.IsUpdate;
            contract.Published = model.Published;
            contract.Version = model.Version;
            contract.UpdateMinVersion = model.UpdateMinVersion;
            contract.UpdateMaxVersion = model.UpdateMaxVersion;
            contract.Md5Hash = model.Md5Hash;

            return true;
        }

        private static bool MapToContractEntity(GameInfoBase contract,Model.Game model)
        {
            if (model == null)
            {
                return false;
            }

            contract.Description = model.Description;
            contract.Developer = MapToContractEntity(GameSourceRepository.Current.Get<Model.Developer>().Single(d => d.Id == model.DeveloperId));
            contract.Homepage = model.Homepage;
            contract.Name = model.Name;
            contract.Uid = model.Uid;
            contract.InterfaceVersion = model.InterfaceVersion;
            contract.RequiresAuthentication = model.RequiresAuthentication;

            return true;
        }

        public static Developer MapToContractEntity(Model.Developer developer)
        {
            Developer contractEntity = new Developer
                                           {
                                               Homepage = developer.Homepage,
                                               Name = developer.Name,
                                               Uid = developer.Uid
                                           };
            return contractEntity;
        }

        public static Model.Game MapToDomainEntity(EditGame game)
        {
            if (game == null)
            {
                return null;
            }


            Model.Game domainEntity = new Model.Game
                                             {
                                                 Description = game.Description,
                                                 Homepage = game.Homepage,
                                                 Name = game.Name,
                                                 Uid = game.Uid,
                                                 InterfaceVersion = game.InterfaceVersion
                                             };
            if (game.RequiresAuthenticationSpecified) domainEntity.RequiresAuthentication = game.RequiresAuthentication;

            return domainEntity;
        }
    }
}
