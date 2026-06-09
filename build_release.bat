@echo off
setlocal

where cmake >nul 2>nul
if errorlevel 1 (
  echo [ERROR] CMake not found.
  exit /b 1
)

where windeployqt >nul 2>nul
if errorlevel 1 (
  echo [ERROR] windeployqt not found. Open the Qt/MSVC developer environment first.
  exit /b 1
)

cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1

cmake --build build --config Release
if errorlevel 1 exit /b 1

if not exist release mkdir release
copy /Y build\XrayQtClient.exe release\XrayQtClient.exe >nul

if exist third_party\xray\xray.exe (
  if not exist release\third_party\xray mkdir release\third_party\xray
  copy /Y third_party\xray\xray.exe release\third_party\xray\xray.exe >nul
)

windeployqt --release --compiler-runtime release\XrayQtClient.exe
if errorlevel 1 exit /b 1

echo.
echo Done. Portable build is in the release folder.
endlocal
