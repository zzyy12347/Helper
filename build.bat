@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0"

set "BUILD_DIR=%~dp0build"
set "CONFIG=Release"
set "QT_PATH=%~1"

echo.
echo === ItemShopFinder build ===
echo Project: %~dp0
echo Config : %CONFIG%
echo.

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake was not found in PATH.
    echo Install CMake first, then run this file again.
    pause
    exit /b 1
)

if not "%QT_PATH%"=="" (
    if exist "%QT_PATH%\lib\cmake" (
        set "CMAKE_PREFIX_PATH=%QT_PATH%;%CMAKE_PREFIX_PATH%"
    ) else (
        echo [ERROR] The Qt path passed to this script does not look valid:
        echo         %QT_PATH%
        echo Expected something like: C:\Qt\6.7.2\msvc2019_64
        pause
        exit /b 1
    )
)

if not "%QT_ROOT%"=="" (
    set "CMAKE_PREFIX_PATH=%QT_ROOT%;%CMAKE_PREFIX_PATH%"
)

echo [1/3] Configuring CMake...
if "%CMAKE_PREFIX_PATH%"=="" (
    cmake -S . -B "%BUILD_DIR%"
) else (
    cmake -S . -B "%BUILD_DIR%" -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%"
)
if errorlevel 1 (
    echo.
    echo [ERROR] CMake configure failed.
    echo If Qt is installed but not found, run:
    echo     build.bat C:\Qt\6.x.x\msvc2019_64
    pause
    exit /b 1
)

echo.
echo [2/3] Building...
cmake --build "%BUILD_DIR%" --config %CONFIG% --parallel
if errorlevel 1 (
    echo.
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

set "EXE_PATH=%BUILD_DIR%\%CONFIG%\ItemShopFinder.exe"
if not exist "%EXE_PATH%" (
    set "EXE_PATH=%BUILD_DIR%\ItemShopFinder.exe"
)

if not exist "%EXE_PATH%" (
    echo.
    echo [ERROR] Build finished, but ItemShopFinder.exe was not found.
    pause
    exit /b 1
)

echo.
echo [3/3] Deploying Qt runtime if windeployqt is available...
where windeployqt >nul 2>nul
if errorlevel 1 (
    if not "%QT_PATH%"=="" if exist "%QT_PATH%\bin\windeployqt.exe" (
        "%QT_PATH%\bin\windeployqt.exe" --release --compiler-runtime "%EXE_PATH%"
    ) else (
        echo [WARN] windeployqt was not found. The EXE was built, but Qt DLLs may need to be copied manually.
    )
) else (
    windeployqt --release --compiler-runtime "%EXE_PATH%"
)

echo.
echo [OK] Build complete:
echo      %EXE_PATH%
echo.
pause
exit /b 0
