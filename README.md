MGDF - Matchstick game development framework
============================================

MGDF runs C++ games on Windows 10 and later. It provides DirectX 12 graphics,
OpenXR, OpenAL audio, RawInput and XInput, HTTP and WebSocket networking, and a
virtual file system for files and archives.

Start the game with `core.exe`. The core loads the game module and runs its
simulation and render threads. A game installer or store client owns installation,
shortcuts, uninstall records, and updates.

The SDK and API reference are at https://www.matchstickframework.org.

Building source
---------------

Use Visual Studio 2026 with the C++ desktop tools, .NET Framework 4.8 targeting
pack, and vcpkg. Enable vcpkg with `vcpkg integrate install`. Restore the Docgen
project's `packages.config`, then build `Matchstick.sln` for x64 Debug or Release.
The crash reporter uses only .NET Framework assemblies. It has no NuGet packages.

The first build downloads the pinned NVIDIA Streamline SDK to
`dependencies/streamline` and checks its SHA-256. To build without network access,
put `streamline-sdk-v<version>.zip` from the NVIDIA Streamline releases there
before the build. Release uses the signed runtime DLLs. Debug uses the development
DLLs. The build also copies the DirectX 12 Agility runtime and OpenXR loader.

Build rules
-----------

- Use x64 for Debug and Release. C++ projects select the 64-bit build tools.
- Each C++ project uses a precompiled header. Start new sources with
  `#include "stdafx.h"`, or `StdAfx.h` in core. Use `NotUsing` only when a source
  requires a define before the header. State the reason beside that setting.
- Core sources in `core.tests` use a separate header built from core's
  `StdAfx.h`. Header-only projects and the IDL project need no precompiled header.
- Keep the process limits in `Directory.Build.rsp` and `Directory.Build.props`.
- Keep the output paths and incremental link-time code generation settings.

Tests
-----

After a solution build, run these commands from `bin/x64/Debug` or
`bin/x64/Release`:

```
core.tests.exe
crashreporter.tests.exe
```

The full core suite includes graphics and OpenXR tests. The crash reporter tests
use a local HTTP server. They check report files, the upload contract, failures,
cancellation, and process exit codes. The core suite starts a child process to
check external minidumps with errors hidden. The reporter tests need no test
packages.

Starting a game
---------------

Use `core.exe` as the entry point for the player, installer shortcut, or store
client. The game manifest is `game/game.json` beside the core. Use
`-gamediroverride` to select another game folder.

Each argument `-name [value]` also has an environment variable `MGDF_NAME`. Set a
flag to any non-empty value. Command line arguments take precedence. The core
loads `.env` beside `core.exe` before it reads the arguments. A `.env` file uses
`KEY=VALUE` lines. It accepts blank lines, `#` comments, and quoted values. It does
not replace environment variables that are already set.

### Arguments

- `-loglevel <level>`: `log_low`, `log_medium`, `log_high`, or `log_error`.
- `-userdiroverride`: Store game user data in the local `user` folder. Without
  this flag, use `%LOCALAPPDATA%/MGDF/1/<gameUid>`.
- `-gamediroverride <directory>`: Select the game folder.
- `-capturediroverride <directory>`: Select the capture folder. The default is the
  captures folder under game user data. Relative paths use the current directory.
- `-metricsport <port>`: Serve Prometheus metrics at `http://localhost:<port>/metrics`.
- `-logendpoint <url>`: Send logs to a Loki endpoint.
- `-statisticsendpointoverride <url>`: Override the manifest statistics endpoint.
- `-hideerrors`: Write a crash dump when a reporter is available. Show no error
  dialog and send no crash report.

For example:

```
MGDF_LOGLEVEL=log_high
MGDF_GAMEDIROVERRIDE="C:\dev\mygame\game"
```

Crash reports and statistics
----------------------------

Set `crashReportService` in `game.json` to the crash service URL. Ship
`crashreporter.exe` and its `.config` file beside `core.exe`. The core starts the
reporter and supplies all paths and game data. The reporter writes
`minidump.dmp` in the game user folder after a crash. An abnormal exit without a
crash signal can still send the log. A normal exit shows no dialog.

The reporter asks the player to send the report. `privacyPolicy`, when present,
is the URL for the dialog's privacy link. The reporter sends a JSON `POST` with
`detail`, `gameUid`, `gameVersion`, and `frameworkVersion`. The service returns
`success` and `uploadUrl`. The reporter sends an `application/zip` `PUT` to that
URL. The zip contains `log.txt` and, when available, `minidump.dmp`.

Set `statisticsService` to enable the statistics endpoint. The host sends values
that the game supplies through `IMGDFStatisticsManager`. The game owns the
statistics controls and decides when to push values.

Building packages
-----------------

From `build`, restore the local Cake tool with `dotnet tool restore`. Run:

```
dotnet cake build.cake --target=Dist --buildnumber=x.x.x --signsecret=<key-container>
```

`Dist` builds and signs the binaries, runs both test programs, and writes runtime,
SDK, symbols, and API documentation output under `dist`. The SDK includes the
runtime zip, headers, IDL, and sample game. The runtime contains `core.exe`,
`crashreporter.exe`, its configuration, native runtime libraries, resources, and
`dependencies/vc2022_redist.x64.exe`. The game installer must run the VC++ runtime
installer. Install .NET Framework 4.8 for the crash reporter if it is absent.

To make an unsigned package for a local check, use `--target=Package` with the
same build number. This target builds the solution and generates the API data.

The framework does not register an installed game or make update manifests.
The `Publish` target uploads release packages and updates the docs site. Run it
only as a separate release action with the required signing and service access.
