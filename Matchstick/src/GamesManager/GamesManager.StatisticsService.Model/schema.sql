CREATE TABLE dbo.mgdf_statistics (
  "Id" uniqueidentifier NOT NULL PRIMARY KEY,
  "Timestamp" Int NOT NULL,
  "SessionId" VarChar(255) NOT NULL,
  "GameUid" VarChar(255) NOT NULL,
  "Name" VarChar(255) NOT NULL,
  "Value" VarChar(255) NOT NULL
);