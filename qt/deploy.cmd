@echo off
rem Build (Release) and assemble a self-contained folder: qt\dist\GdipuActivityHelper
rem Use "deploy.cmd release" for an isolated qt\dist-release\GdipuActivityHelper folder.
rem The folder can be zipped and copied to any Windows 10/11 x64 machine (no Qt or VS install needed).
setlocal
call "%~dp0build.cmd" || exit /b 1

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if defined VS_ROOT set "VS=%VS_ROOT%"
if not defined VS if exist "%VSWHERE%" for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS=%%I"
if defined QT_ROOT set "QT=%QT_ROOT%"
if not defined QT if exist "C:\Qt\6.8.3\msvc2022_64" set "QT=C:\Qt\6.8.3\msvc2022_64"
if not defined VS exit /b 1
if not defined QT exit /b 1
set "OUT=%~dp0dist\GdipuActivityHelper"
if /I "%1"=="release" set "OUT=%~dp0dist-release\GdipuActivityHelper"
set "CRT="
for /d %%D in ("%VS%\VC\Redist\MSVC\*") do if exist "%%~fD\x64\Microsoft.VC143.CRT" set "CRT=%%~fD\x64\Microsoft.VC143.CRT"
if not defined CRT (
    echo Visual C++ runtime files were not found.
    exit /b 1
)

if exist "%OUT%" rmdir /s /q "%OUT%"
mkdir "%OUT%" || exit /b 1
copy /y "%~dp0build\app\GdipuActivityHelper.exe" "%OUT%\" >nul || exit /b 1

"%QT%\bin\windeployqt.exe" --release --no-translations --no-compiler-runtime --no-opengl-sw ^
    --no-system-d3d-compiler --no-system-dxc-compiler ^
    --qmldir "%~dp0app\qml" "%OUT%\GdipuActivityHelper.exe" || exit /b 1

rem ---- trim what this app never loads (it uses only the Basic Controls style) ----
for %%S in (Fusion Imagine Material Universal FluentWinUI3 Windows) do (
    if exist "%OUT%\qml\QtQuick\Controls\%%S" rmdir /s /q "%OUT%\qml\QtQuick\Controls\%%S"
)
del /q "%OUT%\Qt6QuickControls2Fusion*.dll" "%OUT%\Qt6QuickControls2Imagine*.dll" "%OUT%\Qt6QuickControls2Material*.dll" ^
       "%OUT%\Qt6QuickControls2Universal*.dll" "%OUT%\Qt6QuickControls2FluentWinUI3*.dll" "%OUT%\Qt6QuickControls2WindowsStyleImpl.dll" 2>nul
if exist "%OUT%\qmltooling" rmdir /s /q "%OUT%\qmltooling"
if exist "%OUT%\position" rmdir /s /q "%OUT%\position"
del /q "%OUT%\resources\qtwebengine_devtools_resources.pak" 2>nul

rem App-local C++ runtime (same 14.44 toolset the exe was built with): no vc_redist installer needed
copy /y "%CRT%\*.dll" "%OUT%\" >nul || exit /b 1

rem Chromium's own menus / dialogs in Chinese
copy /y "%QT%\translations\qtwebengine_locales\zh-CN.pak" "%OUT%\translations\qtwebengine_locales\" >nul

echo.
echo Deployed to %OUT%
