@echo off
set /p last=<last.txt
start aicup2020.exe --config config2.json
start ..\strategy\x64\Release\AI20.exe
start ..\strategy\x64\Release\AI20_%last%.exe 127.0.0.1 31002
set /a last=%last%-1
start ..\strategy\x64\Release\AI20_%last%.exe 127.0.0.1 31003
set /a last=%last%-1
..\strategy\x64\Release\AI20_%last%.exe 127.0.0.1 31004