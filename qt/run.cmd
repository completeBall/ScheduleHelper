@echo off
rem Run the freshly built app with the Qt runtime on PATH (no deployment needed during development).
rem   run.cmd                      normal start
rem   run.cmd --demo ..\activities.json --theme dark
rem   run.cmd --screenshot out --demo ..\activities.json
setlocal
set "QT=G:\Qt\6.8.3\msvc2022_64"
set "PATH=%QT%\bin;%PATH%"
set "QML_IMPORT_PATH=%QT%\qml"
"%~dp0build\app\GdipuActivityHelper.exe" %*
