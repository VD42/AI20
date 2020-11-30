@echo off
set /p last=<last.txt
start aicup2020.exe --config config2.json
start ..\strategy\x64\Debug\AI20.exe
..\strategy\x64\Release\AI20_%last%.exe 127.0.0.1 31002