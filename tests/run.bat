@echo off
setlocal EnableDelayedExpansion

rem tests\run.bat -- end-to-end tests for cminux v1.0.0 (Windows)

cd /d "%~dp0"

if not defined CMX_BIN set "CMX_BIN=..\build\cminux.exe"
if not exist "%CMX_BIN%" (
    where cminux.exe >nul 2>nul
    if errorlevel 1 (
        echo run.bat: cannot find cminux ^(set CMX_BIN or put it on PATH^)
        exit /b 1
    )
    set "CMX_BIN=cminux.exe"
)

set /a pass=0
set /a fail=0

for %%F in (*.cmx) do (
    set "name=%%~nF"
    set "src=%%F"
    set "exe=%%~nF.exe"

    call :run_one "%%~nF" "%%F" "!exe!"
)

echo.
echo !pass! passed, !fail! failed
if !fail! neq 0 exit /b 1
exit /b 0

:run_one
set "name=%~1"
set "src=%~2"
set "exe=%~3"

rem 1. transpile + compile
"%CMX_BIN%" "%src%" --keep -o "%exe%"
if errorlevel 1 (
    echo FAIL %name%: cminux exited non-zero
    set /a fail+=1
    goto :eof
)

rem 2. diff emitted .cpp
if exist "%name%.expected.cpp" (
    fc /b "%name%.expected.cpp" "%name%.cmx.cpp" >nul
    if errorlevel 1 (
        echo FAIL %name%: emitted .cpp does not match expected
        echo --- diff
        fc "%name%.expected.cpp" "%name%.cmx.cpp"
        set /a fail+=1
        goto :eof
    )
)

rem 3. run, capture stdout
if exist "%name%.expected.stdout" (
    "%exe%" > "%name%.actual.stdout" 2>nul
    fc /b "%name%.expected.stdout" "%name%.actual.stdout" >nul
    if errorlevel 1 (
        echo FAIL %name%: stdout does not match expected
        echo --- expected
        type "%name%.expected.stdout"
        echo --- actual
        type "%name%.actual.stdout"
        set /a fail+=1
        goto :eof
    )
    del /q "%name%.actual.stdout" 2>nul
)

rem 4. exit code
if exist "%name%.expected.exit" (
    "%exe%" >nul 2>nul
    set "actual_exit=!errorlevel!"
    set /p expected_exit=<"%name%.expected.exit"
    if not "!actual_exit!"=="!expected_exit!" (
        echo FAIL %name%: exit !actual_exit!, expected !expected_exit!
        set /a fail+=1
        goto :eof
    )
)

echo PASS %name%
set /a pass+=1

if not defined KEEP (
    del /q "%name%.cmx.cpp" 2>nul
    del /q "%exe%" 2>nul
)
goto :eof