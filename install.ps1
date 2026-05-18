# Mettle local installation script
# This script installs Mettle directly to C:\Program Files\Mettle and adds it to your PATH.

if (-not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Warning "Please run this script as an Administrator."
    Exit
}

$InstallDir = "$env:ProgramFiles\Mettle"

Write-Host "Installing Mettle to $InstallDir..."
if (-not (Test-Path $InstallDir)) {
    New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
}

if (-not (Test-Path "$InstallDir\runtime")) {
    New-Item -ItemType Directory -Force -Path "$InstallDir\runtime" | Out-Null
}

Write-Host "Copying files..."
Copy-Item "bin" "$InstallDir\" -Recurse -Force
Copy-Item "stdlib" "$InstallDir\" -Recurse -Force
Copy-Item "src\runtime\*" "$InstallDir\runtime\" -Recurse -Force
Copy-Item "mettle.ico" "$InstallDir\" -Force

if (Test-Path "installer\mettle-build.bat") {
    Copy-Item "installer\mettle-build.bat" "$InstallDir\bin\" -Force
}

#
# Windows file type: .mettle -> Mettle (same ProgID + commands as installer\Mettle.iss).
#
function Remove-LegacyMethlangAssociations {
    $classesRoot = "HKLM:\SOFTWARE\Classes"
    $methKey = Join-Path $classesRoot ".meth"
    if (Test-Path $methKey) {
        try {
            $progId = (Get-ItemProperty -LiteralPath $methKey -Name "(default)" -ErrorAction Stop).'(default)'
            if ($progId -eq "MethlangFile") {
                Remove-Item -LiteralPath $methKey -Recurse -Force -ErrorAction Stop
                Write-Host "Removed legacy association: .meth -> MethlangFile"
            }
        } catch { }
    }
    $mfKey = Join-Path $classesRoot "MethlangFile"
    if (Test-Path $mfKey) {
        try {
            Remove-Item -LiteralPath $mfKey -Recurse -Force -ErrorAction Stop
            Write-Host "Removed legacy ProgID: MethlangFile"
        } catch { }
    }
}

function Register-MettleFileAssociation {
    param(
        [Parameter(Mandatory = $true)][string]$IconPath,
        [Parameter(Mandatory = $true)][string]$CompilerExe
    )
    $classesRoot = "HKLM:\SOFTWARE\Classes"
    if (-not (Test-Path $classesRoot)) {
        throw "Cannot access $classesRoot (administrator required)."
    }

    New-Item -Path "$classesRoot\.mettle" -Force | Out-Null
    Set-ItemProperty -LiteralPath "$classesRoot\.mettle" -Name "(default)" -Value "MettleFile"

    New-Item -Path "$classesRoot\MettleFile" -Force | Out-Null
    Set-ItemProperty -LiteralPath "$classesRoot\MettleFile" -Name "(default)" -Value "Mettle Source File"

    New-Item -Path "$classesRoot\MettleFile\DefaultIcon" -Force | Out-Null
    Set-ItemProperty -LiteralPath "$classesRoot\MettleFile\DefaultIcon" -Name "(default)" -Value "$IconPath"

    New-Item -Path "$classesRoot\MettleFile\shell\open\command" -Force | Out-Null
    $openCmd = "`"$CompilerExe`" `"%1`""
    Set-ItemProperty -LiteralPath "$classesRoot\MettleFile\shell\open\command" -Name "(default)" -Value $openCmd

    Write-Host "Registered .mettle files (ProgID MettleFile, icon + open with mettle.exe)."
}

Write-Host "Updating Windows file associations..."
Remove-LegacyMethlangAssociations
$iconPath = Join-Path $InstallDir "mettle.ico"
$compilerPath = Join-Path $InstallDir "bin\mettle.exe"
if (-not (Test-Path $compilerPath)) {
    Write-Warning "mettle.exe not found at $compilerPath - skipping .mettle registration."
} elseif (-not (Test-Path $iconPath)) {
    Write-Warning "mettle.ico not found at $iconPath - skipping .mettle registration."
} else {
    Register-MettleFileAssociation -IconPath $iconPath -CompilerExe $compilerPath
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
    Write-Host "Mettle bin directory is already in PATH."
}

Write-Host ""
Write-Host "Checking compiler dependencies..."
if (-not (Get-Command "gcc" -ErrorAction SilentlyContinue)) {
    Write-Warning "gcc was not found in PATH. Mettle requires gcc for linking."
}

if (-not (Get-Command "nasm" -ErrorAction SilentlyContinue)) {
    Write-Warning "nasm was not found in PATH. Mettle requires nasm to assemble .s files."
}

Write-Host ""
Write-Host "Installation complete! Open a new terminal to use 'mettle-build' or 'mettle'. "
