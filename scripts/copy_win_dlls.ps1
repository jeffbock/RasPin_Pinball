# Copies required Windows runtime DLLs (FFmpeg + ANGLE and its dependencies)
# to the Windows debug and release build output directories.
# Run after a clean build or when DLLs are missing from the build output.

$ffmpeg       = "C:\Users\jeffd\PInballProj\FFmpegLib\ffmpeg-master-latest-win64-gpl-shared\ffmpeg-master-latest-win64-gpl-shared\bin"
$angleRelease = "C:\ArmDev\angle\out\release"
$root         = Split-Path $PSScriptRoot -Parent
$sdl          = Join-Path $root "src\lib_ogl_win"

$ffmpegDlls = @(
    "avcodec-62.dll",
    "avformat-62.dll",
    "avutil-60.dll",
    "swscale-9.dll",
    "swresample-6.dll"
)

$angleDlls = @(
    "libEGL.dll",
    "libGLESv2.dll",
    "dawn_native.dll",
    "dawn_proc.dll"
)

$sdlDlls = @(
    "SDL2.dll",
    "SDL2_mixer.dll",
    "libgme.dll",
    "libogg-0.dll",
    "libopus-0.dll",
    "libopusfile-0.dll",
    "libwavpack-1.dll",
    "libxmp.dll"
)

function Copy-RuntimeDll([string]$sourceDirectory, [string]$dllName, [string]$destination) {
    $sourcePath = Join-Path $sourceDirectory $dllName
    if (-not (Test-Path $sourcePath)) {
        throw "Required runtime DLL is missing: $sourcePath"
    }

    Copy-Item $sourcePath $destination -Force
    Write-Host "  Copied $dllName -> $destination"
}

# Map each build target to its corresponding ANGLE source
$targets = @(
    @{ Path = "$root\build\windows\debug";   Angle = $angleRelease },
    @{ Path = "$root\build\windows\release"; Angle = $angleRelease }
)

foreach ($entry in $targets) {
    $t = $entry.Path
    $angleSrc = $entry.Angle
    New-Item -ItemType Directory -Force -Path $t | Out-Null
    foreach ($d in $ffmpegDlls) {
        Copy-RuntimeDll $ffmpeg $d $t
    }
    foreach ($d in $angleDlls) {
        Copy-RuntimeDll $angleSrc $d $t
    }
    foreach ($d in $sdlDlls) {
        Copy-RuntimeDll $sdl $d $t
    }
    # Clean up debug-only DLLs that are statically linked in release ANGLE
    foreach ($stale in @("libc++.dll", "third_party_abseil-cpp_absl.dll", "third_party_zlib.dll")) {
        $stalePath = "$t\$stale"
        if (Test-Path $stalePath) {
            Remove-Item $stalePath -Force
            Write-Host "  Removed stale $stale from $t"
        }
    }
}

Write-Host "Done: DLLs copied to build/windows/debug and build/windows/release"
