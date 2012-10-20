========================================================================
    Matchstick game development framework
    SDK oveview
========================================================================

The SDK includes a number of tools that make developing games using the MGDF framework easier, aswell as providing web service implementations so that you can host your own online game source and statistics service sites.

Providing auto updates for your game
====================================

To get your game to auto update you will need to set the "gamesourceservice" field in the games game.json file to the hostname of your update site e.g "http://games.example.com".
When loading your game the GamesManager will then search for a latest.json file at <gamesourceservice>/downloads/1/<gameUid>/latest.json. This file must be of the following format.

{
"Latest":{
"Version":"0.1",
"Url":"http://www.junkship.net/downloads/console.mza",
"MD5":"xxxxxx"
},
"Framework":{
"Version":"0.1",
"Url":"http://www.junkship.net/downloads/MGDF.zip",
"MD5":"xxxxxx"
},
"UpdateOlderVersions":[
    {
        "Url":"http://www.junkship.net/downloads/console-update.mza",
        "MD5":"yyyyyy",
        "FromVersion":"0.1"
    },
]
}

The Latest object should contain the url and version information for the latest available full install package. The Framework object is optional and can specify the version of the MGDF framework required for this game version and where it can be downloaded. Finally the UpdateOlderVersions array is optional and can contain a list of update packages that can be used to upgrade specific installed versions to the latest version. 
NOTE: The GamesManager supports downloading game updates from urls that use http authentication (either basic or digest) if you want to restrict download access to paid users. The GamesManager will pop up a username/password prompt in this case before the update will proceed.
Tools
=====

PackageGen.exe
==============

This tool allows you to create MGDF installer packages or create an update package based upon the difference between two existing installer packages.
Packages created by this tool are then able to be installed directly by the user or uploaded to an online game source for distribution.

Usage

PackageGen <installer package folder> -o <installer package>
    Creates a new installer package from a directory.
PackageGen -u <old installer package> <new installer package> -o <installer package>
    Creates a new installer package that updates the old game to the new one.

Server
======

StatisticsService
=================

a c# implementation of an MGDF statistics service. In order to run this service you will need to have a SQL server database and IIS 7+ for hosting the service.

First you will have to create the database schema by running schema.sql

then you will haev to create a web.config file (you can do this by renaming the supplied default-web.config) and replace the following values with your own.

${ConnectionString} - A connection string to a SQL server database
${Host} - the hostname the service will be hosted at.




