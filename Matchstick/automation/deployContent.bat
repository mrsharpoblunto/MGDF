echo copying MGDF content to bin dir

mkdir bin\%1
XCOPY  "content" bin\%1 /E /Y /I