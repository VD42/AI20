@echo off
del log.txt
set /p last=<last.txt
start aicup2020.exe --batch-mode --config config4.json --save-results log.txt
start ..\strategy\x64\Release\AI20.exe
..\strategy\x64\Release\AI20_%last%.exe 127.0.0.1 31002
type log.txt
del log.txt
pause