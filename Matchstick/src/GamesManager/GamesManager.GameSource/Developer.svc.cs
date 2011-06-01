using System;
using System.Collections.Generic;
using System.Data.Linq;
using System.IO;
using System.Linq;
using System.Net;
using System.Runtime.Serialization;
using System.ServiceModel;
using System.Text;
using System.Web;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Caching;
using MGDF.GamesManager.GameSource.Contracts;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using MGDF.GamesManager.GameSource.Contracts.Messages;
using MGDF.GamesManager.GameSource.Mapping;
using MGDF.GamesManager.GameSource.Model;
using MGDF.GamesManager.GameSource.Model.Configuration;
using MGDF.GamesManager.GameSource.Model.FileServers;
using MGDF.GamesManager.ServerCommon;
using Developer=MGDF.GamesManager.GameSource.Model.Developer;
using Game=MGDF.GamesManager.GameSource.Model.Game;
using GameVersion = MGDF.GamesManager.GameSource.Model.GameVersion;
using User=MGDF.GamesManager.GameSource.Model.User;
using UserGame=MGDF.GamesManager.GameSource.Model.UserGame;

namespace MGDF.GamesManager.GameSource
{
    public class ValidationException: Exception {
        public ValidationException(string message):  base(message)
        {
        }
    }

    public class DeveloperService : GamesService, IGameSourceDeveloperService
    {
        private delegate void AuthenticatedMethodHandler<REQUEST, RESPONSE>(Model.Developer authenticatedDeveloper, REQUEST request, RESPONSE response);
        private delegate bool ValidateRequestHandler<REQUEST>(Model.Developer authenticatedDeveloper, REQUEST request, List<Error> errors);

        private static RESPONSE InvokeAuthenticatedMethod<REQUEST, RESPONSE>(REQUEST request, AuthenticatedMethodHandler<REQUEST, RESPONSE> handler,ValidateRequestHandler<REQUEST> requestValidator)
            where REQUEST : AuthenticatedRequestBase
            where RESPONSE : ResponseBase, new()
        {
            RESPONSE response = new RESPONSE();

            try
            {
                Model.Developer authenticatedDeveloper = GameSourceRepository.Current.Get<Developer>().SingleOrDefault(u => u.DeveloperKey == request.DeveloperKey);
                if (!CNonceCache.Instance.IsUnique(request.CNonce, Common.Constants.AuthenticationTimeoutMinutes))
                {
                    response.Errors.Add(new Error { Code = Error.AuthorizationFailed, Message = "CNonce is not unique" });
                }
                else if (authenticatedDeveloper != null && Cryptography.Authenticate(request.HMAC, request.Timestamp, request.CNonce.ToString(), authenticatedDeveloper.SecretKey, Common.Constants.AuthenticationTimeoutMinutes))
                {
                    if (requestValidator(authenticatedDeveloper, request, response.Errors))
                    {
                        handler(authenticatedDeveloper, request, response);
                    }
                }
                else
                {
                    response.Errors.Add(new Error{Code = Error.AuthorizationFailed, Message = "Incorrect DeveloperKey/HMAC"});
                }
            }
            catch (ValidationException ex)
            {
                response.Errors.Add(new Error { Code = Error.InvalidArguments, Message = ex.Message });
            }
            catch (Exception ex)
            {
                response.Errors.Add(new Error { Code = Error.UnknownError, Message = "Unknown error" });
                Logger.Current.Write(ex, "Error processing " + typeof(RESPONSE).Name);
            }

            return response;
        }

        private delegate void AuthenticatedMethodHandler<REQUEST, RESPONSE,T>(Model.Developer authenticatedDeveloper, REQUEST request, RESPONSE response,PassThroughContext<T> context);
        private delegate bool ValidateRequestHandler<REQUEST,T>(Model.Developer authenticatedDeveloper, REQUEST request, PassThroughContext<T> context,List<Error> errors);

        private static RESPONSE InvokeAuthenticatedMethod<REQUEST, RESPONSE,T>(REQUEST request, AuthenticatedMethodHandler<REQUEST, RESPONSE,T> handler, ValidateRequestHandler<REQUEST,T> requestValidator)
            where REQUEST : AuthenticatedRequestBase
            where RESPONSE : ResponseBase, new()
        {
            RESPONSE response = new RESPONSE();

            try
            {
                Model.Developer authenticatedDeveloper = GameSourceRepository.Current.Get<Developer>().SingleOrDefault(u => u.DeveloperKey == request.DeveloperKey);
                if (!CNonceCache.Instance.IsUnique(request.CNonce, Common.Constants.AuthenticationTimeoutMinutes))
                {
                    response.Errors.Add(new Error { Code = Error.AuthorizationFailed, Message = "CNonce is not unique" });
                }
                else if (authenticatedDeveloper != null && Cryptography.Authenticate(request.HMAC, request.Timestamp, request.CNonce.ToString(), authenticatedDeveloper.SecretKey, Common.Constants.AuthenticationTimeoutMinutes))
                {
                    PassThroughContext<T> context = new PassThroughContext<T>();
                    if (requestValidator(authenticatedDeveloper, request, context,response.Errors))
                    {
                        handler(authenticatedDeveloper, request, response, context);
                    }
                }
                else 
                {
                    response.Errors.Add(new Error{Code = Error.AuthorizationFailed, Message = "Incorrect DeveloperKey/HMAC"});
                }
            }
            catch (ValidationException ex)
            {
                response.Errors.Add(new Error { Code = Error.InvalidArguments, Message = ex.Message });
            }
            catch (Exception ex)
            {
                response.Errors.Add(new Error { Code = Error.UnknownError, Message = "Unknown error" });
                Logger.Current.Write(ex, "Error processing " + typeof(RESPONSE).Name);
            }
            return response;
        }

        public CheckCredentialsResponse CheckCredentials(AuthenticatedRequestBase request)
        {
            return InvokeAuthenticatedMethod<AuthenticatedRequestBase, CheckCredentialsResponse>(request, CheckCredentialsHandler, (d, r, e) => true);
        }

        private static void CheckCredentialsHandler(Developer developer, AuthenticatedRequestBase request, CheckCredentialsResponse response)
        {
            response.Developer = GameMapper.MapToContractEntity(developer);
        }

        public CleanupIncompleteGameVersionResponse CleanupIncompleteGameVersion(CleanupIncompleteGameVersionRequest request)
        {
            return InvokeAuthenticatedMethod<CleanupIncompleteGameVersionRequest, CleanupIncompleteGameVersionResponse,GameFragment>(request, CleanupIncompleteGameVersionHandler, CleanupIncompleteGameVersionRequestValidator);
        }

        private static void CleanupIncompleteGameVersionHandler(Developer authenticateddeveloper, CleanupIncompleteGameVersionRequest request, CleanupIncompleteGameVersionResponse response,PassThroughContext<GameFragment> context)
        {
            GameVersion gameVersion = (from g in GameSourceRepository.Current.Get<GameVersion>() where g.Id == context.Context.GameVersionId select g).SingleOrDefault();

            DeleteGameFragment(context.Context);

            GameSourceRepository.Current.Delete(gameVersion);
            GameSourceRepository.Current.SubmitChanges();
            GameCache.Instance.Invalidate();
            GameVersionCache.Instance.Invalidate();
        }

        private static bool CleanupIncompleteGameVersionRequestValidator(Developer developer, CleanupIncompleteGameVersionRequest request, PassThroughContext<GameFragment> context,List<Error> errors)
        {
            context.Context = (from g in GameSourceRepository.Current.Get<GameFragment>() where g.Id == request.FragmentId select g).SingleOrDefault();
            if (context.Context == null)
            {
                errors.Add(new Error{Code = Error.InvalidArguments, Message = "No game fragment exists with the supplied Uid"});
                return false;
            }
            else if (context.Context.DeveloperId != developer.Id)
            {
                errors.Add(new Error { Code = Error.AccessDenied, Message = "You are not authorized to edit this game" });
                return false;
            }

            return true;
        }

        public AddGameVersionResponse AddGameVersion(AddGameVersionRequest versionRequest)
        {
            return InvokeAuthenticatedMethod<AddGameVersionRequest, AddGameVersionResponse,Game>(versionRequest, AddGameVersionHandler,AddGameVersionRequestValidator);
        }

        public GetGameVersionsResponse GetGameVersions(GetGameVersionsRequest request)
        {
            return InvokeAuthenticatedMethod<GetGameVersionsRequest, GetGameVersionsResponse,Game>(request, GetGameVersionsHandler, GetGameVersionsRequestValidator);

        }

        private static bool GetGameVersionsRequestValidator(Developer authenticateddeveloper, GetGameVersionsRequest request, PassThroughContext<Game> context, List<Error> errors)
        {
            if (string.IsNullOrEmpty(request.GameUid))
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message =  "No Game Uid supplied" });
                return false;
            }
            else
            {
                context.Context = (from g in GameSourceRepository.Current.Get<Game>() where g.Uid == request.GameUid select g).SingleOrDefault();
                if (context.Context == null)
                {
                    errors.Add(new Error { Code = Error.InvalidArguments, Message = "No game exists with the supplied Uid" });
                    return false;
                }
                else if (context.Context.DeveloperId != authenticateddeveloper.Id)
                {
                    errors.Add(new Error { Code = Error.AccessDenied, Message = "You are not authorized to view all versions of this game" });
                    return false;
                }
            }
            return true;
        }

        private static void GetGameVersionsHandler(Developer authenticateddeveloper, GetGameVersionsRequest request, GetGameVersionsResponse response, PassThroughContext<Game> context)
        {
            response.GameVersions = GameMapper.MapToContractEntities(from gv in GameSourceRepository.Current.Get<GameVersion>() where gv.GameId == context.Context.Id select gv);
        }

        private static bool AddGameVersionRequestValidator(Developer authenticateddeveloper, AddGameVersionRequest request, PassThroughContext<Game> context,List<Error> errors)
        {
            if (string.IsNullOrEmpty(request.GameUid))
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No Game Uid supplied" });
                return false;
            }
            else if (request.GameUid.Equals("core", StringComparison.InvariantCultureIgnoreCase) || request.GameUid.Equals("downloads", StringComparison.InvariantCultureIgnoreCase))
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "This Game Uid is reserved" });
                return false;
            }
            else if (request.NewGameVersion==null)
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No NewGameVersion supplied" });
                return false;    
            }
            else if (request.NewGameVersion.IsUpdate && string.IsNullOrEmpty(request.NewGameVersion.Description))
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No NewGameVersion Update Description supplied" });
                return false;  
            }                
            else if (string.IsNullOrEmpty(request.NewGameVersion.Md5Hash))
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No NewGameVersion Md5Hash supplied"});
                return false;  
            }
            else if (string.IsNullOrEmpty(request.NewGameVersion.Version))
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No NewGameVersion Version supplied" });
                return false;
            }
            else if (request.NewGameVersion.IsUpdate && string.IsNullOrEmpty(request.NewGameVersion.UpdateMaxVersion))
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No NewGameVersion UpdateMaxVersion supplied"});
                return false;
            }
            else if (request.NewGameVersion.IsUpdate && string.IsNullOrEmpty(request.NewGameVersion.UpdateMinVersion))
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No NewGameVersion UpdateMinVersion supplied" });
                return false;
            }
            else if (request.NewGameVersion.IsUpdate)
            {
                try
                {
                    Version version = new Version(request.NewGameVersion.Version);
                    Version minVersion = new Version(request.NewGameVersion.UpdateMinVersion);
                    Version maxVersion = new Version(request.NewGameVersion.UpdateMaxVersion);

                    if (minVersion >= version)
                    {
                        errors.Add(new Error { Code = Error.InvalidArguments, Message = "UpdateMinVersion must be less than the NewGameVersion version"});
                        return false;
                    }
                    else if (maxVersion >= version)
                    {
                        errors.Add(new Error { Code = Error.InvalidArguments, Message = "UpdateMaxVersion must be less than the NewGameVersion version" });
                        return false;
                    }
                    else if (minVersion>maxVersion)
                    {
                        errors.Add(new Error { Code = Error.InvalidArguments, Message = "No NewGameVersion UpdateMinVersion must be >= UpdateMaxVersion" });
                        return false;
                    }
                }
                catch (Exception)
                {
                    errors.Add(new Error { Code = Error.InvalidArguments, Message = "Version, UpdateMinVersion, UpdateMaxVersion formats invalid" });
                    return false;
                }
            }
            else
            {
                try
                {
                    Version version = new Version(request.NewGameVersion.Version);
                }
                catch (Exception)
                {
                    errors.Add(new Error { Code = Error.InvalidArguments, Message = "Version format invalid" });
                    return false;
                }
            }

            context.Context = (from g in GameSourceRepository.Current.Get<Game>() where g.Uid == request.GameUid select g).SingleOrDefault();
            if (context.Context == null)
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No game exists with the supplied Uid" });
                return false;
            }

            if (context.Context.DeveloperId != authenticateddeveloper.Id)
            {
                errors.Add(new Error { Code = Error.AccessDenied, Message = "You are not authorized to publish versions of this game" });
                return false;
            }

            string gameVersionUid = GenerateGameVersionUid(context.Context, request.NewGameVersion);
            GameVersion existingVersion = (from g in GameSourceRepository.Current.Get<GameVersion>() where g.Uid == gameVersionUid select g).SingleOrDefault();

            if (existingVersion != null)
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "A GameVersion with the same version already exists." });
                return false;
            }

            return true;
        }

        private static string GenerateGameVersionUid(Game context, GameVersionBase version)
        {
            return context.Id + (version.IsUpdate ? (version.Version + version.UpdateMinVersion + version.UpdateMaxVersion) : version.Version);
        }

        private static void AddGameVersionHandler(Developer developer, AddGameVersionRequest versionRequest, AddGameVersionResponse versionResponse,PassThroughContext<Game> context)
        {
             Model.GameVersion version = new Model.GameVersion
                                            {
                                                Description = versionRequest.NewGameVersion.Description,
                                                CreatedDate = TimeService.Current.Now,
                                                GameId = context.Context.Id,
                                                Id = Guid.NewGuid(),
                                                Uid = GenerateGameVersionUid(context.Context, versionRequest.NewGameVersion),
                                                IsUpdate = versionRequest.NewGameVersion.IsUpdate,
                                                Published = false,
                                                Version = versionRequest.NewGameVersion.Version
                                            };
            if (version.IsUpdate)
            {
                version.UpdateMaxVersion = versionRequest.NewGameVersion.UpdateMaxVersion;
                version.UpdateMinVersion = versionRequest.NewGameVersion.UpdateMinVersion;
            }


            var errors = version.GetValidationErrors();
            if (errors.Count() > 0)
            {
                throw new ValidationException(errors.First().ErrorMessage);
            }

            //ensure at the db level that the version is unique by committing before adding the fragment/data
            GameSourceRepository.Current.Insert(version);
            GameSourceRepository.Current.SubmitChanges();

            GameFragment fragment;
            try
            {
                //create game data before committing fragment to db in case the game data creation fails, this prevents invalid fragments entering the database
                fragment = new GameFragment
                {
                    Id = Guid.NewGuid(),
                    Md5Hash = versionRequest.NewGameVersion.Md5Hash,
                    Developer = developer,
                    PublishOnComplete = versionRequest.PublishOnUploadComplete,
                    GameVersionId = version.Id,
                    CreatedDate = TimeService.Current.Now,
                    GameDataId = FileServer.Current.CreateGameData(developer, context.Context, version, ServerContext.Current, GameSourceRepository.Current)
                };
                GameSourceRepository.Current.Insert(fragment);
                GameSourceRepository.Current.SubmitChanges();
            }
            catch (Exception ex)
            {
                //if there are any problems creating the fragment, delete the version so we aren't left with a version in the db with no associated fragment
                GameSourceRepository.Current.Delete(version);
                GameSourceRepository.Current.SubmitChanges();
                throw ex;
            }

            versionResponse.GameFragmentId = fragment.Id;
            versionResponse.UploadHandler = Config.Current.BaseUrl + Config.Current.FragmentUploadHandler;
            versionResponse.MaxUploadPartSize = Config.Current.MaxUploadPartSize;
            GameVersionCache.Instance.Invalidate();
        }

        public AddGameResponse AddGame(AddGameRequest request)
        {
            return InvokeAuthenticatedMethod<AddGameRequest, AddGameResponse>(request, AddGameHandler,AddGameRequestValidator);
        }

        private static bool AddGameRequestValidator(Developer authenticateddeveloper, AddGameRequest request, List<Error> errors)
        {
            if (request.NewGame==null)
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No Game supplied"});
                return false;
            }
            else if (string.IsNullOrEmpty(request.NewGame.Uid))
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No Game Uid supplied" });
                return false;
            }
            else if (string.IsNullOrEmpty(request.NewGame.Name))
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No Game Name supplied"});
                return false;
            }
            else if (string.IsNullOrEmpty(request.NewGame.Description))
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No Game Description supplied"});
                return false;
            }
            else if (request.NewGame.InterfaceVersion<=0)
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "Game InterfaceVersion must be greater than 0" });
                return false;
            }
            else
            {
                Game game = (from g in GameSourceRepository.Current.Get<Game>() where g.Uid == request.NewGame.Uid select g).SingleOrDefault();
                if (game != null)
                {
                    errors.Add(new Error { Code = Error.InvalidArguments, Message = "A game already exists with the supplied Uid" });
                    return false;
                }
            }

            if (!string.IsNullOrEmpty(request.NewGame.Homepage))
            {
                try
                {
                    Uri uri = new Uri(request.NewGame.Homepage);
                }
                catch (Exception)
                {
                    errors.Add(new Error { Code = Error.InvalidArguments, Message = "Homepage is not a valid URI" });
                    return false;
                }
            }

            return true;
        }

        private static void AddGameHandler(Developer developer, AddGameRequest request, AddGameResponse response)
        {
            Game domainEntity = GameMapper.MapToDomainEntity(request.NewGame);
            domainEntity.Developer = developer;

            var errors = domainEntity.GetValidationErrors();
            if (errors.Count() > 0)
            {
                throw new ValidationException(errors.First().ErrorMessage);
            }

            domainEntity.Id = Guid.NewGuid();

            GameSourceRepository.Current.Insert(domainEntity);
            GameSourceRepository.Current.SubmitChanges();

            GameCache.Instance.Invalidate();
        }

        public EditGameResponse EditGame(EditGameRequest request)
        {
            return InvokeAuthenticatedMethod<EditGameRequest, EditGameResponse,Game>(request, EditGameHandler,EditGameRequestValidator);
        }

        private static bool EditGameRequestValidator(Developer authenticateddeveloper, EditGameRequest request, PassThroughContext<Game> context,List<Error> errors)
        {
            if (request.EditGame==null)
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No EditGame supplied" });
                return false;
            }

            context.Context = GameSourceRepository.Current.Get<Game>().SingleOrDefault(p => p.Uid == request.EditGame.Uid);

            if (context.Context == null)
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No Game with a matching Uid/Version found" });
                return false;
            }
            else if (context.Context.DeveloperId != authenticateddeveloper.Id)
            {
                errors.Add(new Error { Code = Error.AccessDenied, Message = "You are not authorized to edit this game" });
                return false;
            }
            return true;
        }

        private static void EditGameHandler(Developer developer, EditGameRequest request, EditGameResponse response, PassThroughContext<Game> context)
        {
            context.Context.Description = !string.IsNullOrEmpty(request.EditGame.Description) ? request.EditGame.Description : context.Context.Description;
            context.Context.Name = !string.IsNullOrEmpty(request.EditGame.Name) ? request.EditGame.Name : context.Context.Name;
            context.Context.Homepage = !string.IsNullOrEmpty(request.EditGame.Homepage) ? request.EditGame.Homepage : context.Context.Homepage;
            context.Context.InterfaceVersion = request.EditGame.InterfaceVersion > 0 ? request.EditGame.InterfaceVersion : context.Context.InterfaceVersion;
            context.Context.RequiresAuthentication = request.EditGame.RequiresAuthenticationSpecified ? request.EditGame.RequiresAuthentication : context.Context.RequiresAuthentication;

            var errors = context.Context.GetValidationErrors();
            if (errors.Count()>0)
            {
                throw new ValidationException(errors.First().ErrorMessage);
            }

            GameSourceRepository.Current.SubmitChanges();
            GameCache.Instance.Invalidate();
        }


        public EditGameVersionResponse EditGameVersion(EditGameVersionRequest request)
        {
            return InvokeAuthenticatedMethod<EditGameVersionRequest, EditGameVersionResponse,GameVersion>(request, EditGameVersionHandler, EditGameVersionRequestValidator);
        }

        private static bool EditGameVersionRequestValidator(Developer authenticateddeveloper, EditGameVersionRequest request, PassThroughContext<GameVersion> context, List<Error> errors)
        {
            if (request.Id == Guid.Empty)
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No Game Version Id supplied" });
                return false;
            }

            context.Context = GameSourceRepository.Current.Get<GameVersion>().SingleOrDefault(p => p.Id == request.Id);
            if (context.Context == null)
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No matching Game version found"});
                return false;
            }

            Game game = GameSourceRepository.Current.Get<Game>().SingleOrDefault(g => g.Id == context.Context.GameId);
            if (game.DeveloperId != authenticateddeveloper.Id)
            {
                errors.Add(new Error { Code = Error.AccessDenied, Message = "You are not authorized to edit this GameVersion" });
                return false;
            }

            return true;
        }

        private static void EditGameVersionHandler(Developer developer, EditGameVersionRequest request, EditGameVersionResponse response, PassThroughContext<GameVersion> context)
        {
            context.Context.Published = request.Published;
            GameSourceRepository.Current.SubmitChanges();
            GameVersionCache.Instance.Invalidate();
        }

        public DeleteGameVersionResponse DeleteGameVersion(DeleteGameVersionRequest request)
        {
            return InvokeAuthenticatedMethod<DeleteGameVersionRequest, DeleteGameVersionResponse,GameVersion>(request, DeleteGameVersionHandler,DeleteGameVersionRequestValidator);
        }

        private static bool DeleteGameVersionRequestValidator(Developer authenticateddeveloper, DeleteGameVersionRequest request, PassThroughContext<GameVersion> context, List<Error> errors)
        {
            if (request.Id==Guid.Empty)
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No Game Version Id supplied"});
                return false;
            }

            context.Context = GameSourceRepository.Current.Get<GameVersion>().SingleOrDefault(p => p.Id == request.Id);
            if (context.Context == null)
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No matching Game version found" });
                return false;
            }

            Game game = GameSourceRepository.Current.Get<Game>().SingleOrDefault(g => g.Id == context.Context.GameId);
            if (game.DeveloperId != authenticateddeveloper.Id)
            {
                errors.Add(new Error { Code = Error.AccessDenied, Message = "You are not authorized to delete this GameVersion" });
                return false;
            }

            return true;
        }

        private static void DeleteGameVersionHandler(Developer developer, DeleteGameVersionRequest request, DeleteGameVersionResponse response, PassThroughContext<GameVersion> context)
        {
            var gameFragments = GameSourceRepository.Current.Get<GameFragment>().Where(gf => gf.GameVersionId == context.Context.Id);
            foreach (var fragment in gameFragments)
            {
                DeleteGameFragment(fragment);
            }
            DeleteGameVersion(context.Context);
            GameSourceRepository.Current.SubmitChanges();
            GameVersionCache.Instance.Invalidate();
        }

        public DeleteGameResponse DeleteGame(DeleteGameRequest request)
        {
            return InvokeAuthenticatedMethod<DeleteGameRequest, DeleteGameResponse,Game>(request, DeleteGameHandler,DeleteGameRequestValidator);
        }

        public GetOrCreateUserResponse GetOrCreateUser(GetOrCreateUserRequest request)
        {
            return InvokeAuthenticatedMethod<GetOrCreateUserRequest, GetOrCreateUserResponse, User>(request, GetOrCreateUserHandler, GetOrCreateUserValidator);
        }

        private static bool GetOrCreateUserValidator(Developer authenticateddeveloper, GetOrCreateUserRequest request, PassThroughContext<User> context, List<Error> errors)
        {
            if (string.IsNullOrEmpty(request.UserName))
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No username supplied"});
                return false; 
            }

            if (string.IsNullOrEmpty(request.PasswordHash))
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No password hash supplied" });
                return false; 
            }

            request.UserName = request.UserName.ToLowerInvariant();
            context.Context = GameSourceRepository.Current.Get<User>().SingleOrDefault(u => u.Name==request.UserName);
            if (context.Context != null && context.Context.PasswordHash != request.PasswordHash)
            {
                errors.Add(new Error { Code = Error.AccessDenied, Message = "Invalid credentials for user" });
                return false;                 
            }

            return true;
        }

        private static void GetOrCreateUserHandler(Developer authenticateddeveloper, GetOrCreateUserRequest request, GetOrCreateUserResponse response, PassThroughContext<User> context)
        {
            if (context.Context != null)
            {
                response.User = new Contracts.Entities.User {Id = context.Context.Id, CustomUserData = context.Context.Custom};
            }
            else
            {
                User newuser = new User
                                   {
                                       Id = Guid.NewGuid(),
                                       Name = request.UserName,
                                       Custom = request.CustomUserData,
                                       PasswordHash = request.PasswordHash
                                   };
                GameSourceRepository.Current.Insert(newuser);
                GameSourceRepository.Current.SubmitChanges();

                response.User = new Contracts.Entities.User { Id = newuser.Id, CustomUserData = request.CustomUserData };
            }

        }

        public EditUserResponse EditUser(EditUserRequest request)
        {
            return InvokeAuthenticatedMethod<EditUserRequest, EditUserResponse, User>(request, EditUserHandler, EditUserValidator);
        }

        private static bool EditUserValidator(Developer authenticateddeveloper, EditUserRequest request, PassThroughContext<User> context, List<Error> errors)
        {
            context.Context = GameSourceRepository.Current.Get<User>().SingleOrDefault(u => u.Id == request.UserId);
            if (context.Context == null)
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No user with this ID found" });
                return false;
            }

            if (string.IsNullOrEmpty(request.OldPasswordHash))
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No old password hash supplied"});
                return false;
            }

            if (string.IsNullOrEmpty(request.NewPasswordHash))
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No new password hash supplied" });
                return false;
            }

            if (request.OldPasswordHash != context.Context.PasswordHash)
            {
                errors.Add(new Error { Code = Error.AccessDenied, Message = "Incorrect old password hash supplied" });
                return false;
            }

            return true;
        }

        private static void EditUserHandler(Developer authenticateddeveloper, EditUserRequest request, EditUserResponse response, PassThroughContext<User> context)
        {
            context.Context.PasswordHash = request.NewPasswordHash;
            if (!string.IsNullOrEmpty(request.CustomUserData)) context.Context.Custom = request.CustomUserData;
            GameSourceRepository.Current.SubmitChanges();
        }

        public GetUserGamesResponse GetUserGames(GetUserGamesRequest request)
        {
            return InvokeAuthenticatedMethod<GetUserGamesRequest, GetUserGamesResponse>(request, GetUserGamesHandler, GetUserGamesValidator);
        }

        private static bool GetUserGamesValidator(Developer authenticateddeveloper, GetUserGamesRequest request, List<Error> errors)
        {
            User user = GameSourceRepository.Current.Get<User>().SingleOrDefault(u => u.Id == request.UserId);
            if (user == null)
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No user with this ID found" });
                return false;
            }

            return true;
        }

        private static void GetUserGamesHandler(Developer authenticateddeveloper, GetUserGamesRequest request, GetUserGamesResponse response)
        {
            var games = from g in GameSourceRepository.Current.Get<Game>()
                        join ug in GameSourceRepository.Current.Get<UserGame>() on g.Id equals ug.GameId
                        where g.DeveloperId == authenticateddeveloper.Id && ug.UserId == request.UserId
                        select g;

            response.UserGames = GameMapper.MapToContractEntities(authenticateddeveloper, games);
        }

        public EditUserGamesResponse EditUserGames(EditUserGamesRequest request)
        {
            return InvokeAuthenticatedMethod<EditUserGamesRequest, EditUserGamesResponse,Game>(request, EditUserGamesHandler, EditUserGamesValidator);

        }

        private static bool EditUserGamesValidator(Developer authenticateddeveloper, EditUserGamesRequest request, PassThroughContext<Game> context, List<Error> errors)
        {
            if (string.IsNullOrEmpty(request.GameUid))
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No game Uid supplied"});
                return false;
            }

            User user = GameSourceRepository.Current.Get<User>().SingleOrDefault(u => u.Id == request.UserId);
            if (user == null)
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No user with this ID found" });
                return false;
            }

            context.Context = GameSourceRepository.Current.Get<Game>().SingleOrDefault(g => g.Uid == request.GameUid && g.DeveloperId == authenticateddeveloper.Id);
            if (context.Context == null)
            {
                errors.Add(new Error { Code = Error.AccessDenied, Message = "No game with this Uid belongs to this developer" });
                return false;
            }

            return true;
        }

        private static void EditUserGamesHandler(Developer authenticateddeveloper, EditUserGamesRequest request, EditUserGamesResponse response, PassThroughContext<Game> context)
        {
            UserGame userGame = GameSourceRepository.Current.Get<UserGame>().SingleOrDefault(ug => ug.GameId == context.Context.Id && ug.UserId == request.UserId);
            if (request.AllowAccess)
            {
                if (userGame==null)
                {
                    userGame = new UserGame
                    {
                        Id = Guid.NewGuid(),
                        UserId = request.UserId,
                        GameId = context.Context.Id
                    };
                    GameSourceRepository.Current.Insert(userGame);//allow access
                }
            }
            else
            {
                if (userGame != null)
                {
                    GameSourceRepository.Current.Delete(userGame);//revoke access
                }               
            }

            GameSourceRepository.Current.SubmitChanges();

        }

        private static bool DeleteGameRequestValidator(Developer authenticateddeveloper, DeleteGameRequest request, PassThroughContext<Game> context, List<Error> errors)
        {
            if (string.IsNullOrEmpty(request.GameUid))
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No Game Uid supplied" });
                return false;
            }

            context.Context = GameSourceRepository.Current.Get<Game>().Single(p => p.Uid == request.GameUid);

            if (context.Context == null)
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No matching Game version found" });
                return false;
            }
            else if (context.Context.DeveloperId != authenticateddeveloper.Id)
            {
                errors.Add(new Error { Code = Error.AccessDenied, Message = "You are not authorized to delete this Game" });
                return false;
            }
            return true; 
        }

        private static void DeleteGameHandler(Developer developer, DeleteGameRequest request, DeleteGameResponse response, PassThroughContext<Game> context)
        {
            DeleteGame(context.Context);
            GameSourceRepository.Current.SubmitChanges();
            GameCache.Instance.Invalidate();
            GameVersionCache.Instance.Invalidate();
        }

        private static void DeleteGame(Game game)
        {
            var userGames = GameSourceRepository.Current.Get<UserGame>().Where(ug => ug.GameId == game.Id);
            var gameVersions = GameSourceRepository.Current.Get<GameVersion>().Where(gv => gv.GameId == game.Id);
            var gameFragments = GameSourceRepository.Current.Get<GameFragment>().Where(gf => gameVersions.Select(gv => gv.Id).Contains(gf.Id));

            foreach (var fragment in gameFragments)
            {
                DeleteGameFragment(fragment);
            }
            foreach (var version in gameVersions)
            {
                DeleteGameVersion(version);
            }
            GameSourceRepository.Current.DeleteAll(userGames);
            GameSourceRepository.Current.Delete(game);
        }


        private static void DeleteGameVersion(GameVersion domainEntity)
        {
            var pending = new PendingDelete
            {
                Id = Guid.NewGuid(),
                GameDataId = domainEntity.GameDataId
            };
            GameSourceRepository.Current.Insert(pending);
            GameSourceRepository.Current.Delete(domainEntity);
        }

        private static void DeleteGameFragment(GameFragment domainEntity)
        {
            var pending = new PendingDelete
            {
                Id = Guid.NewGuid(),
                GameDataId = domainEntity.GameDataId
            };
            GameSourceRepository.Current.Insert(pending);
            GameSourceRepository.Current.Delete(domainEntity);
        }
    }
}
