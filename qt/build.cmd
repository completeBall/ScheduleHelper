@echo off
rem Build helper: sets up MSVC 14.44 + the VS-bundled CMake/Ninja, then configures and builds.
rem   build.cmd            configure + build (Release)
rem   build.cmd test       also run ctest
rem   build.cmd noapp      core library + tests only
setlocal
set "VS=C:\Program Files\Microsoft Visual Studio\18\Community"
set "QT=G:\Qt\6.8.3\msvc2022_64"
call "%VS%\VC\Auxiliary\Build\vcvars64.bat" -vcvars_ver=14.44 >nul || exit /b 1
set "PATH=%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%QT%\bin;%PATH%"
set "APP=ON"
if /I "%1"=="noapp" set "APP=OFF"
cmake -S "%~dp0." -B "%~dp0build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QT% -DBUILD_APP=%APP% || exit /b 1
cmake --build "%~dp0build" || exit /b 1
if /I "%1"=="test" ctest --test-dir "%~dp0build" --output-on-failure || exit /b 1
if /I "%1"=="noapp" ctest --test-dir "%~dp0build" --output-on-failure || exit /b 1
