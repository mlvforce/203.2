# Packages Release build into dist\ with required SFML DLLs (vcpkg layout)
param(
  [string]$BuildDir = "build",
  [string]$Triplet = "x64-windows",
  [string]$VcpkgRoot = "$env:VCPKG_ROOT"
)

if (-not $VcpkgRoot) {
  if (Test-Path "$PSScriptRoot\vcpkg") { $VcpkgRoot = "$PSScriptRoot\vcpkg" } 
  else { Write-Error "Set VCPKG_ROOT or place vcpkg next to this script."; exit 1 }
}

$dist = "dist"
New-Item -ItemType Directory -Force -Path $dist | Out-Null
Copy-Item "$BuildDir\Release\whiteboard.exe" $dist

$bin = Join-Path $VcpkgRoot "installed\$Triplet\bin"
Copy-Item "$bin\sfml-*.dll" $dist -ErrorAction SilentlyContinue
Copy-Item "$bin\zlib*.dll" $dist -ErrorAction SilentlyContinue
Copy-Item "$bin\freetype*.dll" $dist -ErrorAction SilentlyContinue
Copy-Item "$bin\jpeg*.dll" $dist -ErrorAction SilentlyContinue
Copy-Item "$bin\openal*.dll" $dist -ErrorAction SilentlyContinue

Set-Content "$dist\README.txt" "Double-click whiteboard.exe to run. No install required."
Write-Host "Packed to dist\"
