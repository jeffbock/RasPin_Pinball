# Copies required Windows runtime DLLs (FFmpeg + ANGLE and its dependencies)
# to the Windows debug and release build output directories.
# Run after a clean build or when DLLs are missing from the build output.

$ffmpeg       = "C:\Users\jeffd\PInballProj\FFmpegLib\ffmpeg-master-latest-win64-gpl-shared\ffmpeg-master-latest-win64-gpl-shared\bin"
$angleDebug   = "C:\ArmDev\angle\out\debug"
$angleRelease = "C:\ArmDev\angle\out\release"
$root         = Split-Path $PSScriptRoot -Parent

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
        Copy-Item "$ffmpeg\$d" "$t\" -Force
        Write-Host "  Copied $d -> $t"
    }
    foreach ($d in $angleDlls) {
        Copy-Item "$angleSrc\$d" "$t\" -Force
        Write-Host "  Copied $d -> $t"
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
