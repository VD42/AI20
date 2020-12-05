@echo off
del log.txt
set /p last=<last.txt

for /L %%u in (1,1,10) do ( 
    start /min aicup2020.exe --batch-mode --config config3.json --save-results log.txt
    start /min ..\strategy\x64\Release\AI20_%last%.exe 127.0.0.1 31002
    ..\strategy\x64\Release\AI20.exe
    type log.txt
    del log.txt
)

pause