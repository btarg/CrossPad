param(
  [string]$Root = (Split-Path -Parent $PSScriptRoot),
  [ValidateSet('win32-x64', 'linux-x64')]
  [string]$Target = 'win32-x64',
  [switch]$SkipImportTest,
  [switch]$SkipPackageTest
)

$ErrorActionPreference = 'Stop'
$rootPath = (Resolve-Path -LiteralPath $Root).Path

function Assert-File {
  param([string]$RelativePath)

  $path = Join-Path $rootPath $RelativePath
  if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
    throw "Required build artifact is missing: $RelativePath"
  }

  $file = Get-Item -LiteralPath $path
  if ($file.Length -le 0) {
    throw "Build artifact is empty: $RelativePath"
  }

  Write-Host ("Verified {0} ({1:N0} bytes)" -f $RelativePath, $file.Length)
}

function Assert-CommandSucceeded {
  param(
    [string]$Command,
    [string[]]$Arguments
  )

  & $Command @Arguments
  if ($LASTEXITCODE -ne 0) {
    throw "Command failed with exit code $LASTEXITCODE`: $Command $($Arguments -join ' ')"
  }
}

$requiredFiles = @(
  "prebuilds\$Target\virtual_x360.node"
)

if ($Target -eq 'win32-x64') {
  $requiredFiles += @(
    'vendor\WinUHid\bin\win32-x64\WinUHid.dll',
    'vendor\WinUHid\bin\win32-x64\WinUHidDevs.dll',
    'vendor\WinUHid\package\WinUHidDriver.dll',
    'vendor\WinUHid\package\WinUHidDriver.inf',
    'vendor\WinUHid\package\winuhiddriver.cat'
  )
}

foreach ($relativePath in $requiredFiles) {
  Assert-File $relativePath
}

$infPath = Join-Path $rootPath 'vendor\WinUHid\package\WinUHidDriver.inf'
if ($Target -eq 'win32-x64') {
  $infText = Get-Content -LiteralPath $infPath -Raw
  if ($infText -notmatch '(?im)^\s*\[Version\]\s*$') {
    throw 'WinUHidDriver.inf is missing its [Version] section.'
  }
  if ($infText -notmatch '(?im)^\s*CatalogFile\s*=\s*winuhiddriver\.cat\s*$') {
    throw 'WinUHidDriver.inf does not reference winuhiddriver.cat.'
  }
  Write-Host 'Verified WinUHidDriver.inf structure.'
}

if (-not $SkipImportTest) {
  Push-Location $rootPath
  try {
    Assert-CommandSucceeded 'node' @(
      '-e',
      "const crosspad = require('./'); if (!crosspad || typeof crosspad.createXboxOneController !== 'function') { throw new Error('CrossPad API is incomplete.'); } console.log('CrossPad package import succeeded.');"
    )
  } finally {
    Pop-Location
  }
}

if (-not $SkipPackageTest) {
  Push-Location $rootPath
  $stdoutPath = Join-Path ([System.IO.Path]::GetTempPath()) "crosspad-npm-pack-$([System.Guid]::NewGuid()).json"
  $stderrPath = Join-Path ([System.IO.Path]::GetTempPath()) "crosspad-npm-pack-$([System.Guid]::NewGuid()).log"
  try {
    $npmCommand = if ($env:OS -eq 'Windows_NT') { 'npm.cmd' } else { 'npm' }
    $packProcess = Start-Process -FilePath $npmCommand `
      -ArgumentList @('pack', '--dry-run', '--json') `
      -WorkingDirectory $rootPath `
      -RedirectStandardOutput $stdoutPath `
      -RedirectStandardError $stderrPath `
      -NoNewWindow `
      -Wait `
      -PassThru
    if ($packProcess.ExitCode -ne 0) {
      $details = if (Test-Path -LiteralPath $stderrPath) {
        Get-Content -LiteralPath $stderrPath -Raw
      } else {
        ''
      }
      throw "npm pack --dry-run failed with exit code $($packProcess.ExitCode). $details"
    }

    $packJson = Get-Content -LiteralPath $stdoutPath -Raw | ConvertFrom-Json
    $packNames = @($packJson.files | ForEach-Object { $_.path })
    foreach ($relativePath in $requiredFiles) {
      $packagePath = $relativePath -replace '\\', '/'
      if ($packNames -notcontains $packagePath) {
        throw "Required artifact is not included in the npm package: $packagePath"
      }
    }
    Write-Host 'Verified required artifacts are included in npm pack output.'
  } finally {
    Pop-Location
    if (Test-Path -LiteralPath $stdoutPath) {
      Remove-Item -LiteralPath $stdoutPath -Force
    }
    if (Test-Path -LiteralPath $stderrPath) {
      Remove-Item -LiteralPath $stderrPath -Force
    }
  }
}

Write-Host "Build verification succeeded for $Target."
