@echo off
setlocal
set "APP_NAME=SteamIdlerEnhanced"
set "QT_PATH=C:\Qt\6.10.1\mingw_64"
set "MINGW_PATH=C:\Qt\Tools\mingw1310_64\bin"
set "DIST_DIR=..\dist"

echo [1/6] Cleaning and Killing Processes...
taskkill /f /im %APP_NAME%.exe /t 2>nul
if exist build rd /s /q build 2>nul
if exist %DIST_DIR% rd /s /q %DIST_DIR% 2>nul
mkdir build
mkdir %DIST_DIR%

echo [1.5/6] Copying DLLs for deployment...
copy /y "%MINGW_PATH%\libgcc_s_seh-1.dll" "build\" 2>nul
copy /y "%MINGW_PATH%\libstdc++-6.dll" "build\" 2>nul
copy /y "%MINGW_PATH%\libwinpthread-1.dll" "build\" 2>nul

cd build

echo [2/6] Configuring CMake (Release Mode)...
set PATH=%MINGW_PATH%;%PATH%
cmake -G "MinGW Makefiles" ^
      -DCMAKE_PREFIX_PATH=%QT_PATH% ^
      -DCMAKE_BUILD_TYPE=Release ..

echo [3/6] Compiling...
cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed.
    exit /b %ERRORLEVEL%
)

echo [4/6] Deploying to dist folder...
:: Copy the EXE to the dist folder
copy /y Release\%APP_NAME%.exe %DIST_DIR%\ 2>nul || copy /y %APP_NAME%.exe %DIST_DIR%\ 2>nul

:: Deploy Qt dependencies with proper flags
echo Deploying Qt libraries...
windeployqt --release --no-translations --compiler-runtime --no-opengl-sw --force-d3dcompiler "%DIST_DIR%\%APP_NAME%.exe"

:: Copy MinGW DLLs to dist
copy /y libgcc_s_seh-1.dll "%DIST_DIR%\" 2>nul
copy /y libstdc++-6.dll "%DIST_DIR%\" 2>nul
copy /y libwinpthread-1.dll "%DIST_DIR%\" 2>nul

echo [5/6] Syncing Assets...
mkdir "%DIST_DIR%\data\lang" 2>nul
mkdir "%DIST_DIR%\themes" 2>nul
mkdir "%DIST_DIR%\logs" 2>nul

:: Copy game data and themes
xcopy /y /e "..\data\lang\*.json" "%DIST_DIR%\data\lang\"
xcopy /y /e "..\themes\*.qss" "%DIST_DIR%\themes\"
copy /y "..\data\app_list.json" "%DIST_DIR%\data\"

:: Create .gitkeep files for folders to ensure they exist in version control
type nul > "%DIST_DIR%\logs\.gitkeep"

echo [6/6] Complete!
cd ..

echo.
echo ===================================================
echo [SUCCESS] Portable folder created in: %DIST_DIR%
echo.
echo Deployment includes:
echo   - SteamIdlerEnhanced.exe (main application)
echo   - Qt6 libraries (auto-deployed by windeployqt)
echo   - MinGW runtime DLLs (libgcc, libstdc++, libwinpthread)
echo   - themes/ folder (Dark, Light, Glass, Cyberpunk themes)
echo   - data/ folder (app_list.json, language files)
echo   - logs/ folder (for application logs)
echo.
echo Note: The executable is self-contained and will configure
echo the DLL search path automatically on startup.
echo ===================================================