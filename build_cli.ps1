# Build the CLI using the same checkout version and toolchain as build_gui.ps1.
$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
if (-not (Get-Command g++ -ErrorAction SilentlyContinue)) {
    $env:Path = "C:\msys64\ucrt64\bin;" + $env:Path
}
$version = git -c core.excludesFile=NUL describe --tags --always --dirty 2>$null
if (-not $version) { $version = 'dev' }
$versionDefine = '-DAPP_VERSION=\"' + $version + '\"'
& g++ -std=c++17 -O2 -Wall -Wextra -finput-charset=UTF-8 -static $versionDefine -o lvm_reader.exe main.cpp lvm_parser.cpp data_io.cpp filter_engine.cpp spectrum_worker.cpp fft.cpp analysis.cpp -lshell32
if ($LASTEXITCODE -ne 0) { throw "CLI build failed (exit $LASTEXITCODE)." }
Write-Host "Built lvm_reader.exe ($version)"
