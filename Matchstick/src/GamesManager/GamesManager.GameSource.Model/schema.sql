-- 
-- Table structure for table developers
-- 

CREATE TABLE dbo.mgdf_developers (
  "Id" uniqueidentifier NOT NULL PRIMARY KEY,
  "Name" varchar(255) NOT NULL,
  "Homepage" varchar(255) default NULL,
  "Uid" varchar(255) UNIQUE NOT NULL,
  "Custom" varchar(255) UNIQUE NULL,
  "DeveloperKey" varchar(255) UNIQUE NOT NULL,
  "SecretKey" varchar(255) UNIQUE NOT NULL
);

-- 
-- Table structure for table gamefragments
-- 

CREATE TABLE dbo.mgdf_gamefragments (
  "Id" uniqueidentifier NOT NULL PRIMARY KEY,
  "PublishOnComplete" bit NOT NULL,
  "GameDataId" varchar(255) default NULL,
  "Md5Hash" varchar(255) default NULL,
  "CreatedDate" datetime NOT NULL,
  "DeveloperId" uniqueidentifier default NULL,
  "GameVersionId" uniqueidentifier default NULL
);

-- 
-- Table structure for table games
-- 

CREATE TABLE dbo.mgdf_games (
  "Id" uniqueidentifier NOT NULL PRIMARY KEY,
  "Name" varchar(255) NOT NULL,
  "Description" varchar(255) NOT NULL,
  "Uid" varchar(255) NOT NULL,
  "RequiresAuthentication" bit NOT NULL,
  "Homepage" varchar(255) NOT NULL,
  "DeveloperId" uniqueidentifier default NULL,
  "InterfaceVersion" int NOT NULL,
);

-- 
-- Table structure for table gameversions
-- 

CREATE TABLE dbo.mgdf_gameversions (
  "Id" uniqueidentifier NOT NULL PRIMARY KEY,
  "Version" varchar(255) NOT NULL,
  "IsUpdate" bit NOT NULL,
  "Published" bit NOT NULL,
  "UpdateMinVersion" varchar(255) default NULL,
  "UpdateMaxVersion" varchar(255) default NULL,
  "GameDataId" varchar(255) default NULL,
  "Md5Hash" varchar(255) default NULL,
  "GameId" uniqueidentifier default NULL,
  "CreatedDate" datetime NOT NULL,
  "Description" varchar(255) NULL
);

-- 
-- Table structure for table usergames
-- 

CREATE TABLE dbo.mgdf_usergames (
  "Id" uniqueidentifier NOT NULL PRIMARY KEY,
  "GameId" uniqueidentifier NOT NULL,
  "UserId" uniqueidentifier NOT NULL
);

-- 
-- Table structure for table users
-- 

CREATE TABLE dbo.mgdf_users (
  "Id" uniqueidentifier NOT NULL PRIMARY KEY,
  "Name" varchar(255) UNIQUE NOT NULL,
  "Custom" varchar(255) UNIQUE NULL,
  "PasswordHash" varchar(255) default NULL
);

CREATE TABLE dbo.mgdf_defaultfileservergamedata (
  "Id" uniqueidentifier NOT NULL PRIMARY KEY,
  "GameFile" VarChar(255) NOT NULL
);

CREATE TABLE dbo.mgdf_amazons3fileservergamedata (
  "Id" uniqueidentifier NOT NULL PRIMARY KEY,
  "UploadId" VarChar(255) NOT NULL,
  "NextPartIndex" int NOT NULL,
  "Key" VarChar(255) NOT NULL
);


CREATE TABLE dbo.mgdf_amazons3uploadpart (
  "Id" uniqueidentifier NOT NULL PRIMARY KEY,
  "GameDataId" uniqueidentifier NOT NULL,
  "ETag" VarChar(255) NOT NULL,
  "PartIndex" int NOT NULL
);

CREATE TABLE dbo.mgdf_cnonce (
  "Id" uniqueidentifier NOT NULL PRIMARY KEY,
  "Expires" datetime NOT NULL
);

CREATE TABLE dbo.mgdf_queuedfiledelete (
  "Id" uniqueidentifier NOT NULL PRIMARY KEY,
  "FileName" VarChar(255) NOT NULL,
);

--
-- Constraints for amazon s3 upload parts
--

ALTER TABLE dbo.mgdf_amazons3uploadpart
  ADD CONSTRAINT "FK_mgdf_amazons3uploadparts_amazons3fileservergamedata_GameDataId" FOREIGN KEY ("GameDataId") REFERENCES "mgdf_amazons3fileservergamedata" ("Id");
  
-- 
-- Constraints for table gamefragments
-- 

ALTER TABLE dbo.mgdf_gamefragments
  ADD CONSTRAINT "FK_mgdf_gameFragments_Developers_DeveloperId" FOREIGN KEY ("DeveloperId") REFERENCES "mgdf_developers" ("Id");
ALTER TABLE dbo.mgdf_gamefragments
  ADD CONSTRAINT "FK_mgdf_gameFragments_GameVersions_GameVersionId" FOREIGN KEY ("GameVersionId") REFERENCES "mgdf_gameversions" ("Id");

-- 
-- Constraints for table games
-- 

ALTER TABLE dbo.mgdf_games
  ADD CONSTRAINT "FK_mgdf_games_Developers_DeveloperId" FOREIGN KEY ("DeveloperId") REFERENCES "mgdf_developers" ("Id");

  -- 
-- Constraints for table gameversions
-- 

ALTER TABLE dbo.mgdf_gameversions
  ADD CONSTRAINT "FK_mgdf_gameversions_Games_GameId" FOREIGN KEY ("GameId") REFERENCES "mgdf_games" ("Id");

-- 
-- Constraints for table user games
-- 
  
ALTER TABLE dbo.mgdf_usergames
  ADD CONSTRAINT "FK_mgdf_UserGames_User_UserId" FOREIGN KEY ("UserId") REFERENCES "mgdf_users" ("Id");
ALTER TABLE dbo.mgdf_usergames
  ADD CONSTRAINT "FK_mgdf_UserGames_Game_GameId" FOREIGN KEY ("GameId") REFERENCES "mgdf_games" ("Id");
