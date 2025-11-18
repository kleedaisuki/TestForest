<#
@brief  Project development environment installer (PowerShell): create venv and install dependencies.
@brief  项目开发环境安装脚本（中文说明仅保留在这里以示 bilingual 要求，但不在脚本输出中）

Usage:
  Run from project root or from the scripts/ directory:
    pwsh .\scripts\install-dev.ps1
    powershell -ExecutionPolicy Bypass -File .\scripts\install-dev.ps1

Optional parameters:
  -PythonExe <string>   Custom Python executable (default: "python")
  -VenvPath <string>    Virtual environment path (default: ".venv")
  -Force                Recreate venv if it already exists
  -NoEditable           Skip `pip install -e .`
#>

[CmdletBinding()]
param(
    [string]$PythonExe = "python",
    [string]$VenvPath = ".venv",
    [switch]$Force,
    [switch]$NoEditable
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Write-Host "=== Initializing development environment ===" -ForegroundColor Cyan

# ---- 1. Change to project root ----
try {
    if ($PSScriptRoot) {
        $projectRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
    } else {
        $projectRoot = Get-Location
    }
    Set-Location $projectRoot
    Write-Host "Project root: $projectRoot"
} catch {
    Write-Error "Failed to locate project root: $_"
    exit 1
}

# ---- 2. Check Python ----
Write-Host "`n[1/4] Checking Python..." -ForegroundColor Yellow

try {
    $pyVersionOutput = & $PythonExe --version 2>&1
} catch {
    Write-Error "Cannot execute '$PythonExe'. Make sure Python is installed and in PATH."
    exit 1
}

Write-Host "Detected Python: $pyVersionOutput"

if ($pyVersionOutput -match "([0-9]+)\.([0-9]+)\.([0-9]+)") {
    $major = [int]$Matches[1]
    $minor = [int]$Matches[2]
    $patch = [int]$Matches[3]

    if ($major -lt 3 -or ($major -eq 3 -and $minor -lt 11)) {
        Write-Warning "Detected Python $major.$minor.$patch, recommended >= 3.11.0."
    }
} else {
    Write-Warning "Failed to parse Python version string."
}

# ---- 3. Create or recreate venv ----
Write-Host "`n[2/4] Virtual environment: $VenvPath" -ForegroundColor Yellow

$venvExists = Test-Path $VenvPath

if ($venvExists -and $Force) {
    Write-Host "Existing venv detected. -Force specified -> removing and recreating..."
    try {
        Remove-Item -Recurse -Force $VenvPath
        $venvExists = $false
    } catch {
        Write-Error "Failed to remove old venv: $_"
        exit 1
    }
}

if (-not $venvExists) {
    Write-Host "Creating virtual environment..."
    try {
        & $PythonExe -m venv $VenvPath
    } catch {
        Write-Error "Failed to create venv: $_"
        exit 1
    }
} else {
    Write-Host "Venv already exists. Use -Force to recreate."
}

$venvScripts = Join-Path $VenvPath "Scripts"
$pipExe = Join-Path $venvScripts "pip.exe"
$pythonInVenv = Join-Path $venvScripts "python.exe"

if (-not (Test-Path $pipExe)) {
    Write-Error "$pipExe not found. Venv may be broken."
    exit 1
}

# ---- 4. Install dependencies ----
Write-Host "`n[3/4] Installing dependencies..." -ForegroundColor Yellow

$hasRequirements = Test-Path "requirements.txt"
$hasPyproject   = Test-Path "pyproject.toml"

if ($hasRequirements) {
    Write-Host "Found requirements.txt, installing..."
    try {
        & $pipExe install -r "requirements.txt"
    } catch {
        Write-Error "Failed to install from requirements.txt: $_"
        exit 1
    }
} else {
    Write-Warning "requirements.txt not found, skipping."
}

if ($hasPyproject -and -not $NoEditable) {
    Write-Host "`npyproject.toml detected. Running 'pip install -e .' ..."
    try {
        & $pipExe install -e .
    } catch {
        Write-Warning "pip install -e . failed. You may run it manually later: $_"
    }
} elseif ($hasPyproject -and $NoEditable) {
    Write-Host "pyproject.toml detected, but -NoEditable specified. Skipping editable install."
}

# ---- 5. Initialize test directories ----
Write-Host "`n[4/4] Initializing test directories..." -ForegroundColor Yellow

$testRoot = "test-works"
$casesDir = Join-Path $testRoot "cases"
$logsDir  = Join-Path $testRoot "logs"

foreach ($dir in @($testRoot, $casesDir, $logsDir)) {
    if (-not (Test-Path $dir)) {
        Write-Host "Creating directory: $dir"
        New-Item -ItemType Directory -Path $dir | Out-Null
    }
}

Write-Host "`n=== Development environment is ready! ===" -ForegroundColor Green
Write-Host "Venv path: $VenvPath"
Write-Host "To activate the venv in this shell, run:"
Write-Host "  `& `"$venvScripts\Activate.ps1`""
