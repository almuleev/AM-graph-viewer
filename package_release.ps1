param(
    [string]$OutputDir = "dist\AMGraphViewer",
    [switch]$Zip
)

$ErrorActionPreference = "Stop"

Set-Location $PSScriptRoot

# Only create a new release directory; never recursively delete a supplied path.
$releaseRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'dist'))
$releaseTarget = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot $OutputDir))
if (-not $releaseTarget.StartsWith($releaseRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'OutputDir must be a new directory inside the project dist directory.'
}
if (Test-Path -LiteralPath $releaseTarget) {
    throw 'Output directory already exists. Choose a new OutputDir; existing files will not be removed.'
}
$ancestor = Split-Path $releaseTarget -Parent
while ($ancestor.Length -ge $releaseRoot.Length) {
    if ((Test-Path -LiteralPath $ancestor) -and ((Get-Item -LiteralPath $ancestor).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw 'Release output cannot pass through a directory link.'
    }
    $ancestor = Split-Path $ancestor -Parent
}
$OutputDir = $releaseTarget

$version = git -c core.excludesFile= describe --tags --always --dirty 2>$null
if (-not $version) { $version = "dev" }
$guiExe = "AMGraphViewer-$version-win-x64.exe"
$zipName = "AMGraphViewer-$version-win-x64.zip"

$files = @(
    $guiExe,
    "lvm_reader.exe",
    "Start GUI.bat",
    "run.bat",
    "AM_logo.ico",
    "README.md",
    "README_EN.md",
    "README_RU.md",
    "LICENSE"
)

foreach ($file in $files) {
    if (-not (Test-Path -LiteralPath $file)) {
        throw "Required file not found: $file"
    }
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

foreach ($file in $files) {
    Copy-Item -LiteralPath $file -Destination $OutputDir -Force
}

if ($Zip) {
    $zipPath = Join-Path (Split-Path $OutputDir -Parent) $zipName
    if (Test-Path -LiteralPath $zipPath) {
        throw 'Release archive already exists; move it or choose a new parent directory.'
    }
    Compress-Archive -Path (Join-Path $OutputDir '*') -DestinationPath $zipPath -CompressionLevel Optimal
    Write-Host "Created $zipPath"
} else {
    Write-Host "Prepared $OutputDir"
}
