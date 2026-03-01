# MethASM local installation script
# This script installs MethASM directly to C:\Program Files\MethASM and adds it to your PATH.

if (-not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Warning "Please run this script as an Administrator."
    Exit
}

$InstallDir = "$env:ProgramFiles\MethASM"

Write-Host "Installing MethASM to $InstallDir..."
if (-not (Test-Path $InstallDir)) {
    New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
}

# Ensure destination structured paths
if (-not (Test-Path "$InstallDir\src")) {
    New-Item -ItemType Directory -Force -Path "$InstallDir\src" | Out-Null
}

Write-Host "Copying files..."
Copy-Item "bin" "$InstallDir\" -Recurse -Force
Copy-Item "stdlib" "$InstallDir\" -Recurse -Force
Copy-Item "src\runtime" "$InstallDir\src\" -Recurse -Force
Copy-Item "masmicon.ico" "$InstallDir\" -Force

if (Test-Path "installer\masm-build.bat") {
    Copy-Item "installer\masm-build.bat" "$InstallDir\bin\" -Force
}

Write-Host "Updating Machine PATH variable..."
$Path = [Environment]::GetEnvironmentVariable("Path", "Machine")
$TargetBinDir = "$InstallDir\bin"
if ($Path -notmatch "[;]$( [regex]::Escape($TargetBinDir) )[;]?$") {
    $NewPath = $Path
    if (-not $NewPath.EndsWith(";")) {
        $NewPath += ";"
    }
    $NewPath += $TargetBinDir
    [Environment]::SetEnvironmentVariable("Path", $NewPath, "Machine")
    Write-Host "Added $TargetBinDir to Machine PATH."
}
else {
    Write-Host "MethASM bin directory is already in PATH."
}

Write-Host ""
Write-Host "Checking compiler dependencies..."
if (-not (Get-Command "gcc" -ErrorAction SilentlyContinue)) {
    Write-Warning "gcc was not found in PATH. MethASM requires gcc for linking."
}

if (-not (Get-Command "nasm" -ErrorAction SilentlyContinue)) {
    Write-Warning "nasm was not found in PATH. MethASM requires nasm to assemble .s files."
}

Write-Host ""
Write-Host "Installation complete! Open a new terminal to use 'masm-build' or 'methasm'. "
