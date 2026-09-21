@echo off
rem Build helper: sets up MSVC 14.44 + the VS-bundled CMake/Ninja, then configures and builds.
rem   build.cmd            configure + build (Release)
rem   build.cmd test       also run ctest
rem   build.cmd noapp      core library + tests only
setlocal
rem Ninja must recognize MSVC's include lines to track header dependencies.
set "VSLANG=1033"
chcp 65001 >nul
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if defined VS_ROOT set "VS=%VS_ROOT%"
if not defined VS if exist "%VSWHERE%" for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS=%%I"
if not defined VS (
    echo Visual Studio with C++ tools was not found. Set VS_ROOT and retry.
    exit /b 1
)
if defined QT_ROOT set "QT=%QT_ROOT%"
if not defined QT if exist "C:\Qt\6.8.3\msvc2022_64" set "QT=C:\Qt\6.8.3\msvc2022_64"
if not defined QT (
    echo Qt 6.8.3 msvc2022_64 was not found. Set QT_ROOT and retry.
    exit /b 1
)
call "%VS%\VC\Auxiliary\Build\vcvars64.bat" -vcvars_ver=14.44 >nul || exit /b 1
set "PATH=%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%QT%\bin;%PATH%"
set "APP=ON"
if /I "%1"=="noapp" set "APP=OFF"
cmake -S "%~dp0." -B "%~dp0build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QT% -DBUILD_APP=%APP% || exit /b 1
cmake --build "%~dp0build" || exit /b 1
if /I "%1"=="test" ctest --test-dir "%~dp0build" --output-on-failure || exit /b 1
if /I "%1"=="noapp" ctest --test-dir "%~dp0build" --output-on-failure || exit /b 1
