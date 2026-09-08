@echo off
chcp 1251 >nul
setlocal
cd /d "%~dp0"
title AMSignal

set "EXE="
for /f "delims=" %%F in ('dir /b /a-d /o:d "%~dp0AMSignal-*-x64.exe" 2^>nul') do set "EXE=%~dp0%%F"

if exist "%EXE%" (
  start "" "%EXE%"
  exit /b 0
)

echo [!] Graphical viewer not found in this folder.
echo     Expected file: AMSignal-0.13.0-x64.exe
echo.
echo     If you are building from source, run:
echo     powershell -ExecutionPolicy Bypass -File .\build_gui.ps1
echo.
pause
exit /b 1
