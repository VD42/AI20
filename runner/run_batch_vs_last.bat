@echo off
del log.txt
set /p last=<last.txt
set /a last1=%last%-1
set /a last2=%last%-2

for /L %%u in (1,1,10) do ( 
    start /min aicup2020.exe --batch-mode --config config2.json --save-results log.txt
    start /min ..\strategy\x64\Release\AI20_%last%.exe 127.0.0.1 31002
    start /min ..\strategy\x64\Release\AI20_%last1%.exe 127.0.0.1 31003
    start /min ..\strategy\x64\Release\AI20_%last2%.exe 127.0.0.1 31004
    ..\strategy\x64\Release\AI20.exe
    type log.txt
    del log.txt
)

pause