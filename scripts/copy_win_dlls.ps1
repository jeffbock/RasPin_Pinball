# Copies required Windows runtime DLLs (FFmpeg + ANGLE and its dependencies)
# to the Windows debug and release build output directories.
# Run after a clean build or when DLLs are missing from the build output.

$ffmpeg   = "C:\Users\jeffd\PInballProj\FFmpegLib\ffmpeg-master-latest-win64-gpl-shared\ffmpeg-master-latest-win64-gpl-shared\bin"
$angle    = "C:\ArmDev\angle\out\debug"
$root     = Split-Path $PSScriptRoot -Parent

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
    "libc++.dll",
    "dawn_native.dll",
    "dawn_proc.dll",
    "third_party_abseil-cpp_absl.dll",
    "third_party_zlib.dll"
)

$targets = @(
    "$root\build\windows\debug",
    "$root\build\windows\release"
)

foreach ($t in $targets) {
    New-Item -ItemType Directory -Force -Path $t | Out-Null
    foreach ($d in $ffmpegDlls) {
        Copy-Item "$ffmpeg\$d" "$t\" -Force
        Write-Host "  Copied $d -> $t"
    }
    foreach ($d in $angleDlls) {
        Copy-Item "$angle\$d" "$t\" -Force
        Write-Host "  Copied $d -> $t"
    }
}

Write-Host "Done: DLLs copied to build/windows/debug and build/windows/release"
