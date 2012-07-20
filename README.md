MGDF - Matchstick game development framework
============================================

What is it
----------

MGDF is a framework designed to make developing, installing, distributing, and updating games easier. The framework uses DirectX 11 for graphics, DirectInput/XInput for user input (has full support for the xbox 360 controller), and OpenAL for audio (supports 3d positional audio and audio streaming using Ogg Vorbis). The framework also provides a virtual filesystem which allows easy access to resources stored in zipped archives (new archive formats can also be plugged in).

* MGDF makes developing games in c++ faster and easier as you no longer have to build and rebuild the same boilerplate code for initializing DirectX/Audio etc, loading preferences and setting up a render loop every single time you make a game. This means you can spend more time making your game.
* MGDF makes distributing games easier as it includes an auto update mechanism withfull versioning support.

Features
--------
The framework is composed of three main parts (the second two of which are not required for running games, but for online updating/distribution)

### The core
This is written in c++ and is the part of the framework which actually runs the games. The core works by initializing the graphics/input/audio devices, loading up user preferences and setting up a multithreaded render/simulation loop before boostrapping the user supplied game dll and passing control on to that dll. Once up and running the game dll can then access the frameworks functionality via a series of c++ interfaces (graphics, audio, input, virtual filesystem etc...)

### The GamesManager
This is written in C# (.NET 2.0) and consists a winforms application which provides a graphical user interface for launching and updating games.

### Statistics services
This is written in c# (.NET 3.5) and is a WCF web service that allows games to submit statistics to remotely in order to help developers get better information on how their games are being played. In order to respect users privacy, any games wishing to upload statistics to a statistics service will have to be granted permission by the user.

How do I build games using MGDF?
--------------------------------
To download everything you will need in order to begin building your own MGDF based games or hosting a Statistics Service, please download the MGDF SDK from matchstickframework.org (or alternatively download the source). Open the EmptyGame project from the Samples directory in the SDK - provided that the MGDF headers are on visual studio's include path the EmptyGame should compile and provide you with a stub implementation and all necessary files required to build an MGDF module.

Building Source
---------------
MGDF can be built with Microsoft Visual Studio 2008 on windows (VS2010/VS2011 will probably work too, but this is untested). To build MGDF you will need to have installed and built the following prerequisites.

* [boost 1.47](http://www.boost.org/users/history/version_1_47_0.html) (For a prebuilt installer go [here](http://www.boostpro.com/download/))
* Directx SDK [June 2010 version](http://www.microsoft.com/download/en/details.aspx?id=6812)).
* ensure boost and directX libs and headers are on visual studio's lib/include path.
* build vendor/libs.sln in debug win32 and release win32 configurations.
* cd into %installdir%/Matchstick and run 'automation/deploy.bat' to copy the vendor libraries and default data into the Matchstick bin/Debug folder.
* build Matchstick/Matchstick.sln and debug win32 and release win32 configurations

Running unit tests
------------------
All managed code projects use nunit 2.5.2 and providing nunit is installed (use testdriven.net for integration into the visual studio UI) they should work once the solution is compiled

all unmanaged code project use winunit which must first be compiled as part of the vendor\libs.sln project Once built, winunit can be invoked to run a unit test dll with the following command 

    winunit -b test.dll

To make running winunit tests easier you can set up an external tool in visual studio. Go to tools->external tools and select Add. Then fill in the following fields

    Title: Core tests
    Command: <path to winunit>\WinUnit.exe
    Arguments: -b "$(TargetDir)\core.tests.dll"
    Initial Directory: $(TargetDir)
    Also tick use output window

Running core.exe from command line
----------------------------------
core.exe can be invoked either via the GamesManager or directly via the command line. 

### Arguments
* -boot:<uid> (REQUIRED) specifies the uid of the game you wish to boot up.
* -logLevel:<level> (OPTIONAL) allows you to specify the logging verbosity, possible values in descending order of verbosity log_low|log_medium|log_high|log_error
* -userdiroverride (OPTIONAL) if specified, the directory from which to load and save user specific data to (save games/preferences etc.) will be either the gamesdir override (if specified) or application directory. If this parameter is omitted, the data directory will be the users appData\Local\MGDF\<uid> folder
* -gamesdiroverride:<directory> (OPTIONAL) allows you to specify a directory from which to load games from (by default this is the core.exe's application directory)

Running GamesManager.exe from command line
------------------------------------------

### Arguments
* -register: In order to make installing/uninstalling your game easier, you can use the gamesManager register argument to create installed program registry entries, desktop shortcuts, start menu shortcuts and windows games explorer integration.

* -deregister: Removes anything added via the register argument

