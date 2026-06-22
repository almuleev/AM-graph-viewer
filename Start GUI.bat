@echo off
chcp 1251 >nul
setlocal
cd /d "%~dp0"
title LVM Graph Viewer

set "EXE=%~dp0LVM-graph-viewer-win-x64.exe"

if exist "%EXE%" (
  start "" "%EXE%"
  exit /b 0
)

echo [!] Graphical viewer not found in this folder.
echo     Expected file: LVM-graph-viewer-win-x64.exe
echo.
echo     If you are building from source, run:
echo     powershell -ExecutionPolicy Bypass -File .\build_gui.ps1
echo.
pause
exit /b 1
