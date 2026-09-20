$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$sdkRoot = Join-Path $PSScriptRoot 'vendor/webview2-1.0.4191.47'
if (-not (Test-Path (Join-Path $sdkRoot 'lib/net462/Microsoft.Web.WebView2.Core.dll'))) {
    $packageDirectory = Join-Path $PSScriptRoot 'vendor'
    New-Item -ItemType Directory -Force -Path $packageDirectory | Out-Null
    $packageFile = Join-Path $packageDirectory 'webview2-1.0.4191.47.zip'
    Invoke-WebRequest -Uri 'https://api.nuget.org/v3-flatcontainer/microsoft.web.webview2/1.0.4191.47/microsoft.web.webview2.1.0.4191.47.nupkg' -OutFile $packageFile
    if ((Get-FileHash -LiteralPath $packageFile -Algorithm SHA256).Hash -ne 'F492BBF547D0DA329553B6727435B677579B1E9F91CC9E4A1AD029366D5F23D0') { throw 'SDK下载校验失败' }
    Expand-Archive -LiteralPath $packageFile -DestinationPath $sdkRoot -Force
}
$compiler = 'C:/Windows/Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$buildDirectory = Join-Path $PSScriptRoot 'build'
$outputDirectory = Join-Path $projectRoot 'dist'
New-Item -ItemType Directory -Force -Path $buildDirectory,$outputDirectory | Out-Null
$coreDll = Join-Path $sdkRoot 'lib/net462/Microsoft.Web.WebView2.Core.dll'
$formsDll = Join-Path $sdkRoot 'lib/net462/Microsoft.Web.WebView2.WinForms.dll'
$loaderDll = Join-Path $sdkRoot 'runtimes/win-x64/native/WebView2Loader.dll'
$appDll = Join-Path $buildDirectory 'App.dll'
$appSource = Join-Path $PSScriptRoot 'App.cs'
$launcherSource = Join-Path $PSScriptRoot 'Launcher.cs'
$coreSource = Join-Path $projectRoot 'extension/core.js'
$scheduleSource = Join-Path $PSScriptRoot 'Schedule.cs'
$excelSource = Join-Path $PSScriptRoot 'ExcelExport.cs'
$timetableSource = Join-Path $projectRoot 'extension/timetable.js'
& $compiler /nologo /target:library /platform:x64 /optimize+ "/out:$appDll" /reference:System.dll /reference:System.Core.dll /reference:System.Drawing.dll /reference:System.Windows.Forms.dll /reference:System.Web.Extensions.dll /reference:System.IO.Compression.dll /reference:System.IO.Compression.FileSystem.dll "/reference:$coreDll" "/reference:$formsDll" "/resource:${coreSource},core.js" "/resource:${timetableSource},timetable.js" $appSource $scheduleSource $excelSource
if ($LASTEXITCODE -ne 0) { throw '桌面程序编译失败' }
$exe = Join-Path $outputDirectory '广轻活动汇总.exe'
& $compiler /nologo /target:winexe /platform:x64 /optimize+ "/out:$exe" /reference:System.dll /reference:System.Core.dll /reference:System.Windows.Forms.dll "/resource:${appDll},App.dll" "/resource:${coreDll},Microsoft.Web.WebView2.Core.dll" "/resource:${formsDll},Microsoft.Web.WebView2.WinForms.dll" "/resource:${loaderDll},WebView2Loader.dll" $launcherSource
if ($LASTEXITCODE -ne 0) { throw 'EXE打包失败' }
Get-Item -LiteralPath $exe | Select-Object FullName,Length
