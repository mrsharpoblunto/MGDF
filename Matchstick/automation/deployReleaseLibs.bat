mkdir bin\Release

echo copying MGDF Release library dll's to bin directory
XCOPY  ..\..\vendor\lib\lua.dll bin\Release /E /Y /I
XCOPY  ..\..\vendor\lib\luabind.dll bin\Release /E /Y /I
XCOPY  ..\..\vendor\lib\zlib.dll bin\Release /E /Y /I
XCOPY  ..\..\vendor\lib\xerces-c_2_7.dll bin\Release /E /Y /I

echo copying MGDF precompiled dll's to bin directory
XCOPY  lib\msvcp80.dll bin\Release /E /Y /I
XCOPY  lib\msvcr80.dll bin\Release /E /Y /I
XCOPY  lib\fmodex.dll bin\Release /E /Y /I
XCOPY  lib\fmodex_vc.lib bin\Release /E /Y /I
XCOPY  lib\GameuxInstallHelper.dll bin\Release /E /Y /I
XCOPY  lib\d3dx9_27.dll bin\Release /E /Y /I

echo DEPLOY COMPLETED