param(
  [string]$InstallDriver = 'true',
  [switch]$InstallOnly,
  [switch]$SkipDeviceVerification,
  [string]$InstallLogPath
)

$ErrorActionPreference = 'Stop'
$installDriverEnabled = $InstallDriver -notin @('false', 'False', '0')
$root = Split-Path -Parent $PSScriptRoot
$userProject = Join-Path $root 'vendor\WinUHid\WinUHid\WinUHid.vcxproj'
$devicesProject = Join-Path $root 'vendor\WinUHid\WinUHidDevs\WinUHidDevs.vcxproj'
$driverProject = Join-Path $root 'vendor\WinUHid\WinUHid Driver\WinUHid Driver.vcxproj'
$output = Join-Path $root 'vendor\WinUHid\bin\win32-x64'
$driverOutput = Join-Path $root 'vendor\WinUHid\WinUHid Driver\build\Release\x64\WinUHid Driver'
$package = Join-Path $root 'vendor\WinUHid\package'
$devconCandidates = @(
  'C:\Program Files (x86)\Windows Kits\10\Tools\10.0.26100.0\x64\devcon.exe',
  'C:\Program Files (x86)\Windows Kits\10\Tools\10.0.22621.0\x64\devcon.exe'
)

function Install-WinUhidDriver {
  $driverInf = Join-Path $package 'WinUHidDriver.inf'
  $certificate = Join-Path $package 'WinUHidDriver.cer'
  if (-not (Test-Path -LiteralPath $driverInf)) {
    throw "Driver package is missing: $driverInf"
  }
  if (-not (Test-Path -LiteralPath $certificate)) {
    throw "Driver signing certificate is missing: $certificate"
  }
  Write-Host "Trusting local WDK test certificate: $certificate"
  Import-Certificate -FilePath $certificate -CertStoreLocation 'Cert:\LocalMachine\Root' | Out-Null
  Import-Certificate -FilePath $certificate -CertStoreLocation 'Cert:\LocalMachine\TrustedPublisher' | Out-Null
  $devcon = $devconCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
  if (-not $devcon) {
    throw 'devcon.exe was not found. Install the Windows Driver Kit tools before installing the root-enumerated WinUHid device.'
  }
  & $devcon remove '*WinUHid*' | Out-Null
  if ($LASTEXITCODE -ne 0) {
    Write-Warning "No previous WinUHid device was removed (devcon exit code $LASTEXITCODE)."
  }
  Write-Host "Installing and creating Root\WinUHid with: $devcon"
  & $devcon install $driverInf 'Root\WinUHid'
  if ($LASTEXITCODE -ne 0) {
    throw "devcon could not install the WinUHid driver package (exit code $LASTEXITCODE)."
  }
}

if ($InstallOnly) {
  $principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
  if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'InstallOnly must run elevated.'
  }
  if ($InstallLogPath) {
    Start-Transcript -Path $InstallLogPath -Force | Out-Null
  }
  try {
    Install-WinUhidDriver
    Write-Host 'WinUHid driver package installed.'
  } finally {
    if ($InstallLogPath) {
      Stop-Transcript | Out-Null
    }
  }
  exit 0
}

$msbuild = $null
$pathMsbuild = Get-Command msbuild.exe -ErrorAction SilentlyContinue
if ($pathMsbuild) {
  $msbuild = $pathMsbuild.Source
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$candidates = @(
  'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe',
  'C:\Program Files\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe'
)
if (Test-Path -LiteralPath $vswhere) {
  $vsPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
  if ($LASTEXITCODE -eq 0 -and $vsPath) {
    $candidates += Join-Path $vsPath 'MSBuild\Current\Bin\MSBuild.exe'
  }
}
foreach ($candidate in $candidates) {
  if (-not $msbuild -and (Test-Path -LiteralPath $candidate)) {
    $msbuild = $candidate
  }
}
if (-not $msbuild) {
  throw 'MSBuild was not found. Install Visual Studio Build Tools 2022/2026 or make it available through setup-msbuild.'
}

& $msbuild $userProject /p:Configuration=Release /p:Platform=x64 /m
if ($LASTEXITCODE -ne 0) {
  throw 'The WinUHid user library build failed.'
}

New-Item -ItemType Directory -Force -Path $output | Out-Null
$dll = Join-Path $root 'vendor\WinUHid\WinUHid\build\Release\x64\WinUHid.dll'
Copy-Item -LiteralPath $dll -Destination (Join-Path $output 'WinUHid.dll') -Force

& $msbuild $devicesProject /p:Configuration=Release /p:Platform=x64 /m
if ($LASTEXITCODE -ne 0) {
  throw 'The WinUHidDevs preset library build failed.'
}

$devicesDll = Join-Path $root 'vendor\WinUHid\WinUHidDevs\build\Release\x64\WinUHidDevs.dll'
if (-not (Test-Path -LiteralPath $devicesDll)) {
  throw 'The WinUHidDevs build did not produce WinUHidDevs.dll.'
}
Copy-Item -LiteralPath $devicesDll -Destination (Join-Path $output 'WinUHidDevs.dll') -Force

& $msbuild $driverProject /p:Configuration=Release /p:Platform=x64 /p:SkipPackageVerification=true /m
if ($LASTEXITCODE -ne 0) {
  throw 'The WinUHid UMDF2 driver build failed. Install the WDK, Visual Studio UMDF tools, and the x64 Spectre-mitigated C++ libraries (Individual components: Libs for Spectre).'
}

$driverDll = Join-Path $driverOutput 'WinUHidDriver.dll'
$driverInf = Join-Path $driverOutput 'WinUHidDriver.inf'
$driverCat = Join-Path $driverOutput 'winuhiddriver.cat'
$driverCertificate = Join-Path $root 'vendor\WinUHid\WinUHid Driver\build\Release\x64\WinUHidDriver.cer'
if (-not (Test-Path -LiteralPath $driverDll) -or
    -not (Test-Path -LiteralPath $driverInf) -or
    -not (Test-Path -LiteralPath $driverCat) -or
    -not (Test-Path -LiteralPath $driverCertificate)) {
  throw 'The driver build did not produce the driver DLL, INF, catalog, and signing certificate.'
}

New-Item -ItemType Directory -Force -Path $package | Out-Null
Copy-Item -LiteralPath $driverDll -Destination $package -Force
Copy-Item -LiteralPath $driverInf -Destination $package -Force
Copy-Item -LiteralPath $driverCat -Destination $package -Force
Copy-Item -LiteralPath $driverCertificate -Destination (Join-Path $package 'WinUHidDriver.cer') -Force

if ($installDriverEnabled) {
  $principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
  if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    $installLog = Join-Path $env:TEMP 'native-x360-pad-winuhid-install.log'
    if (Test-Path -LiteralPath $installLog) {
      Remove-Item -LiteralPath $installLog -Force
    }
    $arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`" -InstallOnly -InstallLogPath `"$installLog`""
    $elevated = Start-Process powershell.exe -Verb RunAs -ArgumentList $arguments -Wait -PassThru
    if ($elevated.ExitCode -ne 0) {
      $details = if (Test-Path -LiteralPath $installLog) {
        Get-Content -LiteralPath $installLog -Raw
      } else {
        'No installer output was captured.'
      }
      throw "Elevated WinUHid driver installation failed (exit code $($elevated.ExitCode)).`n$details`nInstaller log: $installLog"
    }
    Get-Content -LiteralPath $installLog
  } else {
    Install-WinUhidDriver
  }
}

if ($SkipDeviceVerification) {
  Write-Host "Bundled WinUHid.dll at $output"
  Write-Host "WinUHid driver package built without installing the device."
  exit 0
}

$deviceReady = $false
for ($attempt = 0; $attempt -lt 20; $attempt++) {
  try {
    $handle = [System.IO.File]::Open('\\.\WinUHid', 'Open', 'ReadWrite', 'None')
    $handle.Dispose()
    $deviceReady = $true
    break
  } catch {
    Start-Sleep -Milliseconds 250
  }
}
if (-not $deviceReady) {
  throw 'The driver was installed, but the \\.\WinUHid device interface is unavailable.'
}

Write-Host "Bundled WinUHid.dll at $output"
Write-Host 'WinUHid driver installed and device interface verified.'
