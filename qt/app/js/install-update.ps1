param(
    [Parameter(Mandatory = $true)][string]$Archive,
    [Parameter(Mandatory = $true)][string]$InstallDir,
    [Parameter(Mandatory = $true)][int]$ProcessId,
    [switch]$SkipLaunch
)

$ErrorActionPreference = 'Stop'
$InstallDir = [IO.Path]::GetFullPath($InstallDir).TrimEnd('\')
$Archive = [IO.Path]::GetFullPath($Archive)
$parent = [IO.Path]::GetDirectoryName($InstallDir)
$name = [IO.Path]::GetFileName($InstallDir)
$exe = Join-Path $InstallDir 'GdipuActivityHelper.exe'
$stage = Join-Path $parent ($name + '.update-' + $ProcessId)
$backup = Join-Path $parent ($name + '.backup-' + $ProcessId)
$status = Join-Path ([IO.Path]::GetDirectoryName($Archive)) 'update-status.txt'
$restarted = $false

try {
    if ($name -eq '' -or $parent -eq $InstallDir -or -not (Test-Path -LiteralPath $exe -PathType Leaf)) {
        throw 'Invalid installation directory.'
    }
    if (-not (Test-Path -LiteralPath $Archive -PathType Leaf)) { throw 'Update archive is missing.' }
    if (Test-Path -LiteralPath $stage) { throw 'Update staging directory already exists.' }
    if (Test-Path -LiteralPath $backup) { throw 'Update backup directory already exists.' }

    $deadline = [DateTime]::UtcNow.AddMinutes(3)
    while ($ProcessId -gt 0 -and (Get-Process -Id $ProcessId -ErrorAction SilentlyContinue)) {
        if ([DateTime]::UtcNow -gt $deadline) { throw 'The running application did not exit.' }
        Start-Sleep -Milliseconds 500
    }

    New-Item -ItemType Directory -Path $stage | Out-Null
    Expand-Archive -LiteralPath $Archive -DestinationPath $stage
    $payload = Join-Path $stage 'GdipuActivityHelper'
    if (-not (Test-Path -LiteralPath (Join-Path $payload 'GdipuActivityHelper.exe') -PathType Leaf) -or
        -not (Test-Path -LiteralPath (Join-Path $payload 'Qt6Core.dll') -PathType Leaf)) {
        throw 'The update archive has an unexpected layout.'
    }

    Move-Item -LiteralPath $InstallDir -Destination $backup
    try {
        Move-Item -LiteralPath $payload -Destination $InstallDir
        if (-not $SkipLaunch) {
            $newProcess = Start-Process -FilePath $exe -WorkingDirectory $InstallDir -PassThru
            Start-Sleep -Seconds 5
            if ($newProcess.HasExited) { throw 'The updated application exited immediately.' }
        }
        'ok' | Set-Content -LiteralPath $status -Encoding UTF8
    } catch {
        ('failed: ' + $_.Exception.Message) | Set-Content -LiteralPath $status -Encoding UTF8
        if (Test-Path -LiteralPath $InstallDir) { Remove-Item -LiteralPath $InstallDir -Recurse -Force }
        Move-Item -LiteralPath $backup -Destination $InstallDir
        if (-not $SkipLaunch) {
            Start-Process -FilePath $exe -WorkingDirectory $InstallDir
            $restarted = $true
        }
        throw
    }
    Remove-Item -LiteralPath $backup -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $Archive -Force -ErrorAction SilentlyContinue
} catch {
    ('failed: ' + $_.Exception.Message) | Set-Content -LiteralPath $status -Encoding UTF8
    if (-not $SkipLaunch -and -not $restarted -and (Test-Path -LiteralPath $exe -PathType Leaf) -and
        -not (Get-Process -Id $ProcessId -ErrorAction SilentlyContinue)) {
        Start-Process -FilePath $exe -WorkingDirectory $InstallDir
    }
    exit 1
} finally {
    if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
}
