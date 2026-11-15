MGDF - Matchstick game development framework
============================================

MGDF runs C++ games on Windows 10 and later. It provides DirectX 12 graphics,
OpenXR, OpenAL audio, RawInput and XInput, HTTP and WebSocket networking, and a
virtual file system for files and archives.

Start an installed game with `updater.exe`, or start `core.exe` directly. The core loads the game module and runs its
simulation and render threads. A game installer or store client owns installation,
shortcuts, uninstall records, and updates.

The SDK and API reference are at https://www.matchstickframework.org.

Building source
---------------

Use Visual Studio 2026 with the C++ desktop tools, .NET Framework 4.8 targeting
pack, and vcpkg. Enable vcpkg with `vcpkg integrate install`. Restore the Docgen
project's `packages.config`, then build `Matchstick.sln` for x64 Debug or Release.
The crash reporter and updater use only .NET Framework assemblies. They compile
shared window source into each executable. Each runs as one file, without a
configuration file or another assembly. No IL merge step is needed.

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
updater.tests.exe
```

The full core suite includes graphics and OpenXR tests. The crash reporter tests
use a local HTTP server. They check report files, the upload contract, failures,
cancellation, and process exit codes. The core suite starts a child process to
check external minidumps with errors hidden. The reporter tests need no test
packages.

Starting a game
---------------

Use `updater.exe` for an installed game shortcut. A Steam build starts `core.exe`
and does not ship the updater. The game manifest is `game/game.json` beside the core. Use
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
`crashreporter.exe` beside `core.exe`. The core starts the
reporter and supplies all paths and game data. The reporter writes
a uniquely named `crash-<id>.dmp` in the game user folder after a crash.
An inherited read handle keeps the log tied to that process. Each new game run
creates a new log file object. A crash signal shows the dialog. Without a signal, only an exception status
(`0xC0000000` and above, except `0xC000013A`) shows the dialog, with the log
and no dump. All other exit codes show no dialog and send no report. A core
startup failure uses exit code 1.

Core sends at most 2048 characters of error text before it signals the reporter.
A fatal error includes its sender and message. An unhandled exception uses the
translated Windows error. The dialog shows this text in a read-only box with
four visible lines, word wrap, a vertical scroll bar, and a Copy button. The text
length does not change the window size. The box is absent when there is no text.

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
`crashreporter.exe`, `updater.exe`, native runtime libraries, resources, and
`dependencies/vc2022_redist.x64.exe`. The game installer must run the VC++ runtime
installer. Install .NET Framework 4.8 for both tools if it is absent.

To make an unsigned package for a local check, use `--target=Package` with the
same build number. This target builds the solution and generates the API data.

The game installer owns shortcuts, uninstall records, and runtime installation.
The `Publish` target uploads release packages and updates the docs site. Run it
only as a separate release action with the required signing and service access.

Game manifest
-------------

The manifest is `game/game.json` beside the executables. Its fields are
`gameUid`, `gameName`, `version`, `preferences`, `statisticsService`,
`crashReportService`, `privacyPolicy`, and `updateService`. The last four
fields are optional. Only the updater reads `updateService`.

Installer updates
-----------------

The updater checks `updateService` with a five-second timeout. A missing service,
a failed check, or a version that is not newer starts `core.exe` without a
message. Versions have two to four numeric parts; missing parts are zero.
The service returns this JSON:

```json
{
  "version": "1.2.3",
  "installerUrl": "https://example.com/GameSetup_1.2.3.exe",
  "sha256": "<64 hexadecimal digits>",
  "installerArguments": ["/S", "/D={installDir}"]
}
```

Both URLs must use HTTPS. HTTP with a literal loopback IP is allowed for local
tests. Redirects keep this rule. The manifest is limited to 64 KiB; the installer
is limited to 4 GiB. The window offers Update now or Later. Cancel stops the
download and starts the game. A download or hash failure also starts the game.

Each argument is one array entry. `{installDir}` expands to the installed folder.
For NSIS, put `/D={installDir}` last. It is passed without quotes, including when
the path has spaces. Other arguments use Windows argument quoting. The updater
passes its own arguments unchanged to the game.

Before it runs an installer, the updater checks SHA-256. When the updater has a
valid Authenticode signature, the installer must have a valid signature from the
same certificate. Signature checks use the local certificate cache. Developer
builds without a valid signature use the hash check. A read handle blocks writes
and replacement from verification through installer startup.

The updater copies itself into a private work folder below `%TEMP%/MGDF-updater`.
The copy opens and checks the installer before the first updater releases it.
It then waits for the first updater to exit, runs the installer with
`ShellExecuteEx` and no verb, and waits for it to finish. The installer manifest
controls elevation. The temporary updater keeps its original token and starts
`core.exe` after a complete install. A refused UAC prompt starts the old game
when no repair is pending. An installer failure shows a short message and keeps
the game closed until repair completes. Updaters started as administrator
exit without starting a game. The next start removes old work folders, but keeps
folders held by a running updater.

The updater tests use a local HTTP server, a small installer process, and a game
process. They check cancellation, hashes, parent exit, argument preservation,
the game token, temporary file cleanup, and both standalone executables.

Update completion contract
--------------------------

The installer must write `game/game.json` last, after every file and required
install step succeeds. The new version is the completion record. A failed install
must leave the old version in place.

Each install folder has one file lock in `%LOCALAPPDATA%/MGDF-updater-state`.
The temporary updater inherits that lock. A second updater shows a short message
and exits. Before setup starts, the temporary updater opens each installed EXE
and DLL without sharing and asks Windows Restart Manager whether those files
are in use. Close the game and the crash dialog before an update.

The pending record is written before setup starts. It keeps the target manifest
and the installer's PID and creation time. A new updater does not start the game
or another installer while that process runs. After a failure, it offers repair
from the saved manifest and has no Later button. Cancel, a failed download, or a
refused UAC prompt keeps the game closed until repair completes. Exit code 0, a
reboot success code, or the target version in the game manifest completes repair.

If the temporary updater ends while setup is starting, before it can record the
PID, the pending record stays in the starting state. The updater blocks a game
start because it cannot prove that setup has ended. Run the installer to repair
the folder, then remove that starting record only after setup has exited.

The installer stays open without write sharing from the temporary updater's
hash and signature checks through the installer process creation. The first
updater also holds it until the temporary copy has opened and checked it.

Run either tool with `--mgdf-tool-check` to check that its standalone executable
loads. This check needs no other file in its folder.
