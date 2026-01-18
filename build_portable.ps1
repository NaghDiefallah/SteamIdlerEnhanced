# --- CONFIGURATION ---
$ProjectName = "SteamIdlerEnhanced"
$BuildDir = "build"
$ReleaseDir = "dist"
$ZipName = "SteamIdler_Portable_v1.0.zip"
$QtBinPath = "C:\Qt\6.10.1\mingw_64\bin"

# Clear the console
Clear-Host

# --- 1. SETUP FOLDERS ---
Write-Host ">>> Cleaning up previous distribution..." -ForegroundColor Cyan
if (Test-Path $ReleaseDir) { Remove-Item -Recurse -Force $ReleaseDir }
if (Test-Path $ZipName) { Remove-Item -Force $ZipName }
New-Item -ItemType Directory -Path "$ReleaseDir\themes"
New-Item -ItemType Directory -Path "$ReleaseDir\data\lang"

# --- 2. COMPILE PROJECT ---
Write-Host ">>> Compiling C++ Source (MinGW)..." -ForegroundColor Cyan
if (!(Test-Path $BuildDir)) { New-Item -ItemType Directory -Path $BuildDir }
Set-Location $BuildDir
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
mingw32-make -j$env:NUMBER_OF_PROCESSORS
if ($LASTEXITCODE -ne 0) { 
    Write-Host "`nBUILD FAILED - Check C++ errors above." -ForegroundColor Red
    Set-Location ..
    pause
    exit 
}
Set-Location ..

# --- 3. DEPLOY QT DLLs ---
Write-Host ">>> Deploying Qt Dependencies..." -ForegroundColor Cyan
Copy-Item "build\$ProjectName.exe" "$ReleaseDir\$ProjectName.exe"
& "$QtBinPath\windeployqt.exe" --release --no-translations --compiler-runtime "$ReleaseDir\$ProjectName.exe"

# --- 4. SYNC ASSETS ---
Write-Host ">>> Syncing Assets..." -ForegroundColor Cyan
Copy-Item -Recurse -Force "themes\*" "$ReleaseDir\themes\"
Copy-Item -Recurse -Force "data\lang\*" "$ReleaseDir\data\lang\"

# --- 5. CREATE ZIP ---
Write-Host ">>> Compressing to ZIP..." -ForegroundColor Cyan
Compress-Archive -Path "$ReleaseDir\*" -DestinationPath $ZipName -Force

Write-Host "`n===============================================" -ForegroundColor Green
Write-Host " SUCCESS: $ZipName created!" -ForegroundColor Green
Write-Host " Folder: $ReleaseDir" -ForegroundColor Green
Write-Host "===============================================" -ForegroundColor Green
Write-Host "Ready for Enigma Virtual Box or Direct Upload." -ForegroundColor Yellow
pause