;Includes

  !include "MUI2.nsh"
  
;--------------------------------

; The name of the installer
	Name "MGDF Game"
	OutFile "Setup.exe"

; The default installation directory
	InstallDir "$PROGRAMFILES\MGDF Game"

	RequestExecutionLevel admin

;--------------------------------
;Interface Configuration

  !define MUI_ICON "MGDF.ico"
  !define MUI_UNICON "MGDF.ico"


  !define MUI_HEADERIMAGE
  !define MUI_HEADER_TRANSPARENT_TEXT
  !define MUI_BGCOLOR 000000
  !define MUI_HEADERIMAGE_BITMAP "MGDFHeader.bmp"
  !define MUI_HEADERIMAGE_BITMAP_NOSTRETCH
  !define MUI_HEADERIMAGE_UNBITMAP "MGDFHeader.bmp"
  !define MUI_HEADERIMAGE_UNBITMAP_NOSTRETCH
  !define MUI_ABORTWARNING


;--------------------------------
;Pages

  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"
  
;--------------------------------
; check for and install .net 3.5
Section "Install .NET Framework 3.5"

	IfSilent Ignoredotnetsetup
	Call IsNetfx35Installed
	Pop $0

	IntCmp $0 0 Ignoredotnetsetup

	MessageBox MB_YESNO "MGDF requires that The .NET Framework 3.5 is installed, do you want to install it now?" IDYES Installdotnet
    Quit
Installdotnet:
	SetOutPath $TEMP\MGDF
	File DotNetFx35ClientSetup.exe
	ExecWait '"$TEMP\MGDF\DotNetFx35ClientSetup.exe" /norestart'
	Delete $TEMP\MGDF\DotNetFx35ClientSetup.exe
	
	Call IsNetfx35Installed ; check to see that the framework actually installed and that it wasn't cancelled.
	Pop $0
	
	IntCmp $0 0 Ignoredotnetsetup
	MessageBox MB_OK "The .NET Framework 3.5 was not installed, installation cannot continue."
	Quit

Ignoredotnetsetup:

SectionEnd

;--------------------------------
; install directx9 update
Section "Install Directx 9.0c August 2005 update"

	IfSilent Ignoredxsetup
	SetOutPath $TEMP\MGDF
	File dxredist\*.cab
	File dxredist\*.dll
	File dxredist\DXSETUP.exe

	ExecWait '"$TEMP\MGDF\dxsetup.exe" /silent'
	SetOutPath $TEMP
	RMDIR /r $TEMP\dxredist
Ignoredxsetup:

SectionEnd

;--------------------------------
; install vc 2008 redistributable
Section "Install VC++ 2008 redistributable"

	IfSilent Ignorevcredistsetup
	SetOutPath $TEMP\MGDF
	File vcredist_x86.exe

	ExecWait '"$TEMP\MGDF\vcredist_x86.exe" /q'
	Delete $TEMP\MGDF\vcredist_x86.exe
Ignorevcredistsetup:

SectionEnd

;--------------------------------
; install vc 2010 redistributable
Section "Install VC++ 2010 redistributable"

	IfSilent Ignorevc2010redistsetup
	SetOutPath $TEMP\MGDF
	File vc2010redist_x86.exe

	ExecWait '"$TEMP\MGDF\vc2010redist_x86.exe" /q'
	Delete $TEMP\MGDF\vc2010redist_x86.exe
Ignorevc2010redistsetup:

SectionEnd

;--------------------------------
; the core engine components
Section "Install MGDF"

	Var /GLOBAL DoServiceInstall

	; only stop the service if it currently exists
	IfFileExists "$INSTDIR\GamesManager.Service.exe" StopServiceBeforeInstall
	StrCpy $DoServiceInstall 1
	Goto SkipStopServiceBeforeInstall

StopServiceBeforeInstall:
		StrCpy $DoServiceInstall 0
		nsexec::ExecToLog 'net stop "MGDF.GamesManager.Service"'
		
SkipStopServiceBeforeInstall:

;--- core engine executables ---
	SetOutPath $INSTDIR
	File "..\bin\release\*.dll"
	File "..\bin\release\GamesManager.exe"
	File "..\bin\release\GamesManager.exe.config"
	File "..\bin\release\GamesManager.Service.exe"
	File "..\bin\release\GamesManager.Service.exe.config"
	File "..\bin\release\GamesManager.BootStrapper.exe"
	File "..\bin\release\GamesManager.BootStrapper.exe.config"
	File "..\bin\release\GamesManager.FrameworkUpdater.exe"
	File "..\bin\release\GamesManager.FrameworkUpdater.exe.config"
	File "..\bin\release\core.exe"
	File "MGDF.ico"

;--- core content schemas ---
	SetOutPath $INSTDIR\schemas
	File /r "..\bin\release\schemas\*.*"

;--- core content ---
	SetOutPath $INSTDIR\games
	SetOutPath $INSTDIR\games\core
	File /r "..\bin\release\games\core\*.*"

	SetOutPath $INSTDIR

	StrCmp $DoServiceInstall 1 InstallService
	Goto SkipInstallService

;we only want to install the service if we haven't already
InstallService:
	nsexec::ExecToLog '"$WINDIR\Microsoft.NET\Framework\v2.0.50727\installutil.exe" "$INSTDIR\GamesManager.Service.exe"'

SkipInstallService:
	nsexec::ExecToLog 'net start "MGDF.GamesManager.Service"'
	
	WriteRegStr HKCR ".mza" "" "MGDF.Installer"	
	WriteRegStr HKCR "MGDF.Installer\DefaultIcon" "" "$INSTDIR\MGDF.ico"	
	WriteRegStr HKCR "MGDF.Installer\shell\open\command" "" '"$INSTDIR\GamesManager.exe" -install:"%1"'	
			
; Tell the compiler to write an uninstaller and to look for a "Uninstall" section
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MGDF1" "DisplayName" "MGDF 1.0"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MGDF1" "UninstallString" "$INSTDIR\Uninstall.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MGDF1" "DisplayIcon" "$INSTDIR\MGDF.ico"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MGDF1" "URLInfoAbout" "http://matchstickframework.org"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MGDF1" "Publisher" "No.8 Interactive"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MGDF1" "InstallLocation" "$INSTDIR"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MGDF1" "DisplayVersion" "1.0.0.0"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MGDF1" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MGDF1" "NoRepair" 1	
	WriteUninstaller $INSTDIR\Uninstall.exe	
SectionEnd



;--------------------------------
; uninstall the core engine
Section "Uninstall"

; cwanr user of any installed MGDF games are present before uninstalling.
ExecWait '"$INSTDIR\GamesManager.exe" -checkuninstall' $0
IntCmp $0 -1 CancelUninstall

goto ContinueUninstall

CancelUninstall:
	Quit
	
ContinueUninstall:
nsexec::ExecToLog 'net stop "MGDF.GamesManager.Service"'
nsexec::ExecToLog '"$WINDIR\Microsoft.NET\Framework\v2.0.50727\installutil.exe" "$INSTDIR\Gamesmanager.Service.exe" /u'	
	
;--- remove core engine components ---
	Delete $INSTDIR\*.dll
	Delete $INSTDIR\*.exe
	Delete $INSTDIR\*.config
	Delete $INSTDIR\MGDF.ico

	RMDIR /r $INSTDIR\games
	RMDIR /r $INSTDIR\schemas

	;--- remove uninstaller ---
	Delete $INSTDIR\Uninstall.exe
	
	RMDIR /r $INSTDIR

	; delete uninstaller registry keys
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MGDF1"
	DeleteRegKey HKCR ".mza"
	DeleteRegKey HKCR "MGDF.Installer"
SectionEnd

 Function IsNetfx35Installed
;Check is Net 3.5 (or at least the client framework) is install
;Push 0 for true, Pop -1 for false
;SetRegView 64
ReadRegDWORD $0  HKLM "Software\Microsoft\NET Framework Setup\NDP\v3.5" "Install"
StrCmp $0 1 Installed
ReadRegDWORD $0  HKLM "Software\Microsoft\NET Framework Setup\DotNetClient\v3.5" "Install"
StrCmp $0 1 Installed
Push -1
Goto EndCheck
Installed:
Push 0
EndCheck:
FunctionEnd

