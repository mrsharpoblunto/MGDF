<%@ Page Language="C#" Inherits="System.Web.Mvc.ViewPage" %>
<%@ Import Namespace="MGDF.GamesManager.Common.Extensions"%>
<%@ Import Namespace="MatchstickFramework.Web.Models"%>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" >
<head runat="server">
    <title>MGDF - Matchstick Game Development Framework</title>
    <link href="/Content/styles.css" type="text/css" rel="Stylesheet" />
</head>
<body>
    <div id="container">
        <div id="header">
            <img src="/Content/Images/MGDF.jpg" alt="Matchstick Game Development Framework" />
        </div>
        <div id="content">
            <h2><a href="<%=VersionManager.GetLatestVersion(1)!=null ? VersionManager.GetLatestVersion(1).FileName.AbsoluteUri(Request.Url.Scheme+"://" +Request.Url.Host) : ""%>">Download Latest Version</a></h2>

            <div id="content-inner">
                <p>MGDF is a framework designed to make developing, installing, distributing, and updating games easier. The framework uses DirectX 9 for graphics, DirectInput/XInput for user input (has full support for the xbox 360 controller), and OpenAL for audio (supports 3d positional audio and audio streaming using Ogg Vorbis). The framework also provides a virtual filesystem which allows easy access to resources stored in zipped archives (new archive formats can also be plugged in).</p>
                <ul>
                <li>MGDF makes developing games in c++ faster and easier as you no longer have to build and rebuild the same boilerplate code for initializing DirectX/Audio etc, loading preferences and setting up a render loop every single time you make a game. This means you can spend more time making your game.</li>
                <li>MGDF makes distributing games easier as you don't have to worry about creating an installer for your game, simply zip up your game into an archive and the framework will handle the installation of your game on the clients machine.</li>
                <li>MGDF makes it easier to keep your game up to date by providing an open online distribution system for your games, which you can either self host or have someone else host for you.</li>
                </ul>
            </div>
           
            <h3><a href="https://github.com/mrsharpoblunto/MGDF">View Source Code</a> | <a href="<%=VersionManager.GetLatestSDKVersion(1)!=null ? VersionManager.GetLatestSDKVersion(1).FileName.AbsoluteUri(Request.Url.Scheme+"://" +Request.Url.Host) : ""%>">Download Latest SDK</a></h3>
        </div>
    </div>
</body>
</html>
