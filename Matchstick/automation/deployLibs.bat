mkdir bin\%1
mkdir bin\%1\%2

echo copying MGDF library dll's to bin directory
XCOPY  ..\vendor\lib\%1\%2\zlib.dll bin\%1\%2 /E /Y /I
XCOPY  ..\vendor\lib\%1\%2\fw1FontWrapper.dll bin\%1\%2 /E /Y /I

echo copying MGDF precompiled dll's to bin directory
XCOPY  ..\vendor\lib\Release\GameuxInstallHelper.dll bin\%1\%2 /E /Y /I
XCOPY  ..\vendor\lib\%1\Release\OpenAL32.dll bin\%1\%2 /E /Y /I
XCOPY  ..\vendor\lib\%1\Release\wrap_oal.dll bin\%1\%2 /E /Y /I
XCOPY  ..\vendor\lib\%1\Release\alut.dll bin\%1\%2 /E /Y /I
XCOPY  ..\vendor\lib\%1\Release\ogg.dll bin\%1\%2 /E /Y /I
XCOPY  ..\vendor\lib\%1\Release\vorbis.dll bin\%1\%2 /E /Y /I
XCOPY  ..\vendor\lib\%1\Release\vorbisfile.dll bin\%1\%2 /E /Y /I

echo DEPLOY COMPLETED