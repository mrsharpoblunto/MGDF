CREATE TABLE dbo.mgdf_statistics (
  "Id" uniqueidentifier NOT NULL PRIMARY KEY,
  "GameUid" VarChar(255) NOT NULL,
  "Name" VarChar(127) NOT NULL,
  "Value" VarChar(127) NOT NULL
);