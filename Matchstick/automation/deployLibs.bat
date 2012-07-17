mkdir bin\%1

echo copying MGDF library dll's to bin directory
XCOPY  ..\vendor\lib\%1\zlib.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\%1\xerces-c_2_7.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\%1\fw1FontWrapper.dll bin\%1 /E /Y /I

echo copying MGDF precompiled dll's to bin directory
XCOPY  ..\vendor\lib\Release\GameuxInstallHelper.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\Release\OpenAL32.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\Release\wrap_oal.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\Release\alut.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\Release\ogg.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\Release\vorbis.dll bin\%1 /E /Y /I
XCOPY  ..\vendor\lib\Release\vorbisfile.dll bin\%1 /E /Y /I

echo DEPLOY COMPLETED