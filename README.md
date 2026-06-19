MGDF - Matchstick game development framework
============================================

What is it
----------

MGDF is a framework designed to make developing, installing, distributing, and updating games on Windows (Windows 10+ supported) easier. The framework uses DirectX 11 for graphics, RawInput/XInput for user input (has full support for the XBox 360 controller), and OpenAL for audio (supports 3d positional audio and audio streaming using Ogg Vorbis). The framework also provides a virtual file system which allows easy access to resources stored in zipped archives (new archive formats can also be plugged in).

* MGDF makes developing games in c++ faster and easier as you no longer have to build and rebuild the same boilerplate code for initializing DirectX/Audio etc, loading preferences and setting up a render loop every single time you make a game. This means you can spend more time making your game.
* MGDF makes distributing games easier as it includes an auto update mechanism with full versioning support.

How do I build games using MGDF?
--------------------------------
To download the SDK, or view the getting started and API reference, go to www.matchstickframework.org. Alternatively you can clone this repo instead of downloading the pre-built SDK.

Features
--------
The framework is composed of three main parts (the second two of which are not required for running games, but for online updating/distribution)

### The core
This is written in c++ and is the part of the framework which actually runs the games. The core works by initializing the graphics/input/audio devices, loading up user preferences and setting up a multithreaded render/simulation loop before boostrapping the user supplied game dll and passing control on to that dll. Once up and running the game dll can then access the frameworks functionality via a series of COM interfaces (graphics, audio, input, virtual filesystem etc...)

### The GamesManager (Optional)
This is written in C# (.NET 4.6) and consists a windows forms application which provides a graphical user interface for launching and updating games.

Building Source
---------------

* Using Microsoft Visual Studio 2026 (with the vcpkg component installed & enabled via `vcpkg integrate install`) build Matchstick.sln in x64 debug/release configurations

Running unit tests
------------------
All managed code projects use nunit 3,	all unmanaged code tests are contained within the core.tests.exe binary which must first be compiled as part of the Matchstick.sln project Once built, running this exe from the command line will run the test suite.

To make running the unmanaged unit tests easier you can set up an external tool in visual studio. Go to tools->external tools and select Add. Then fill in the following fields

    Title: Core tests
    Command: <MGDF bin dir>\core.tests.exe Initial Directory: $(TargetDir) Also tick use output window

Running core.exe from command line
----------------------------------
core.exe can be invoked either via the GamesManager or directly via the command line.

Configuration can be supplied either as command line arguments or as environment variables. Every command line argument `-name [value]` has an equivalent environment variable `MGDF_NAME` (the argument name upper-cased and prefixed with `MGDF_`); for flags that take no value, set the variable to any non-empty value. Command line arguments take precedence over environment variables.

For ease of debugging the environment variables can also be placed in a `.env` file in the application directory. Each line is a `KEY=VALUE` pair; blank lines and lines beginning with `#` are ignored, and values may optionally be wrapped in matching single or double quotes. On startup these values are loaded into the process environment (without overwriting any variable already set in the real environment), so they are visible both to the framework and to the bootstrapped game module (via `getenv`/`GetEnvironmentVariable`), as well as to any child processes.

### Arguments
* -loglevel <level> (env `MGDF_LOGLEVEL`): (OPTIONAL) The logging verbosity, possible values in descending order of verbosity log_low|log_medium|log_high|log_error
* -userdiroverride (env `MGDF_USERDIROVERRIDE`): (OPTIONAL) The directory from which to load and save user specific data to (save games/preferences etc.) will be either the gamesdir override (if specified) or application directory. If this parameter is omitted, the data directory will be the users appData\Local\MGDF\<uid> folder
* -gamediroverride <directory> (env `MGDF_GAMEDIROVERRIDE`): (OPTIONAL) The directory from which to load games from (by default this is the core.exe's application /game subdirectory). Useful for local development
* -metricsport <port_number> (env `MGDF_METRICSPORT`): (OPTIONAL) A port to expose internal metrics via a prometheus compatible endpoint at http://localhost:<port_number>/metrics
* -logendpoint <url> (env `MGDF_LOGENDPOINT`): (OPTIONAL) A Loki logging endpoint to push internal logs to
* -statisticsendpointoverride <url> (env `MGDF_STATISTICSENDPOINTOVERRIDE`): (OPTIONAL) A statistics endpoint to push statistics to (overrides any statistics service specified in the games manifest)

An example `.env` file:

    # logging
    MGDF_LOGLEVEL=log_high
    MGDF_GAMEDIROVERRIDE="C:\dev\mygame\game"

Running GamesManager.exe from command line
------------------------------------------

As with core.exe, these arguments can also be supplied as `MGDF_`-prefixed environment variables (e.g. `MGDF_GAMEDIROVERRIDE`), including via a `.env` file in the application directory. Command line arguments take precedence over environment variables. The GamesManager loads the `.env` file into the process environment before launching core.exe, so the values are inherited by the game as well.

### Arguments
* -register: In order to make installing/uninstalling your game easier, you can use the gamesManager register argument to create installed program registry entries, desktop shortcuts, start menu shortcuts and windows games explorer integration.

* -deregister: Removes anything added via the register argument
 
* -gamediroverride <directory>: (OPTIONAL) allows you to specify a directory from which to load games from (by default this is the core.exe's application /game subdirectory). Useful for local development

* -noupdatecheck: (OPTIONAL) disables checks for updates.

Building a release package
--------------------------

* To build a release package, run `build\build.ps1 -Target Dist -buildnumber=x.x.x` from a powershell prompt. This will put the generated release in /dist
* To publish the release, locate /dist from the previous step and run `build\build.ps1 -Target Publish -dist=<dist_folder_from_previous_step> -backup=<some_build_archive_folder> -s3accesskey=xxxx -s3secretkey=yyyy` from a powershell prompt.
