echo copying MGDF content to bin dir

mkdir bin\%1\%2
XCOPY  "content" bin\%1\%2 /E /Y /I