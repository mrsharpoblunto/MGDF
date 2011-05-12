mkdir bin\%1

echo copying MGDF Debug library dll's to bin directory
XCOPY  ..\vendor\lib\%1\lua.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\%1\luabind.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\%1\zlib.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\%1\xerces-c_2_7.dll bin\%1 /E /Y /I

echo copying MGDF precompiled dll's to bin directory
XCOPY  ..\vendor\lib\Release\msvcp80.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\Release\msvcr80.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\Release\fmodex.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\Release\GameuxInstallHelper.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\Release\OpenAL32.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\Release\alut.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\Release\ogg.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\Release\vorbis.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\Release\vorbisfile.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\Release\XInput9_1_0.dll bin\%1 /E /Y /I

if not %1 == Debug goto else
XCOPY  ..\vendor\lib\%1\d3dx9d_27.dll bin\%1 /E /Y /I
goto endif
:else
XCOPY  ..\vendor\lib\%1\d3dx9_27.dll bin\%1 /E /Y /I
:endif

echo DEPLOY COMPLETED