param(
  [string]$Root = (Split-Path -Parent $PSScriptRoot),
  [ValidateSet('win32-x64', 'linux-x64')]
  [string]$Target = 'win32-x64'
)

$ErrorActionPreference = 'Stop'
$rootPath = (Resolve-Path -LiteralPath $Root).Path
$source = Join-Path $rootPath 'build\Release\virtual_x360.node'
$destinationDirectory = Join-Path $rootPath "prebuilds\$Target"
$destination = Join-Path $destinationDirectory 'virtual_x360.node'

if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
  throw "Built native addon is missing: $source"
}

$sourceFile = Get-Item -LiteralPath $source
if ($sourceFile.Length -le 0) {
  throw "Built native addon is empty: $source"
}

New-Item -ItemType Directory -Force -Path $destinationDirectory | Out-Null
Copy-Item -LiteralPath $source -Destination $destination -Force
Write-Host "Staged native addon at $destination"
