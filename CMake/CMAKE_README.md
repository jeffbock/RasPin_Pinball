# CMake Build System for PInball (Raspberry Pi)

This CMake configuration provides an alternative build system for the Raspberry Pi version of PInball, with the same optimization settings as the VS Code tasks.  

NOTE:  VS Code is the primary maintained build tool.  This has not been tested or updated extensively, and is probably got path problems since the last reorganization.  Use at your own risk.  Intention was to use this as a secondary build mechanism when the repo is cloned on a system without VS Code.

## Features

- ✅ **Automatic version updating** before each build
- ✅ **Optimized release builds** with same settings as tasks.json
- ✅ **Debug build support** with proper debug flags
- ✅ **Parallel compilation** support
- ✅ **Easy-to-use build scripts**

## Quick Start

### Option 1: Using the build script (Recommended)
```bash
# Make the script executable
chmod +x build_release_raspi.sh

# Build the release version
./build_release_raspi.sh
```

### Option 2: Manual CMake commands
```bash
# Create and enter build directory
mkdir build_release && cd build_release

# Configure for release build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build with parallel compilation
make -j$(nproc)
```

## Build Types

### Release Build (Default)
- Maximum optimization (`-O3`)
- CPU-specific optimizations (`-march=native`, `-mtune=native`)
- Link-time optimization (`-flto`)
- Fast math optimizations (`-ffast-math`)
- Loop unrolling (`-funroll-loops`)
- Stripped symbols for smaller binary (`-s`)
- Output: `raspibuild/release/Pinball`

### Debug Build
```bash
mkdir build_debug && cd build_debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

## Version Management

The CMake build automatically runs the version update script before compilation, just like the VS Code tasks. This ensures:
- Build number increments on every build
- Version display shows correct build number
- Consistent versioning across all build methods

## Output Location

- **Release builds**: `raspibuild/release/Pinball`
- **Debug builds**: `raspibuild/debug/Pinball` (if you create a debug build)

## Cleaning

```bash
# Use the clean script
chmod +x clean_raspi.sh
./clean_raspi.sh

# Or manually remove build directories
rm -rf build_release build_debug
```

## Comparison with tasks.json

The CMake build uses identical compiler flags and settings as the "Raspberry Pi: Full Pinball Build (Release)" task in VS Code, ensuring consistent builds regardless of build method.

## Requirements

- CMake 3.16 or higher
- GCC/G++ compiler
- All dependencies (EGL, GLES, X11, SDL2, wiringPi, etc.)
- Python3 (for version script)

## Troubleshooting

1. **Missing dependencies**: Install required development packages
   ```bash
   sudo apt update
   sudo apt install build-essential cmake libsdl2-dev libsdl2-mixer-dev libegl1-mesa-dev libgles2-mesa-dev libx11-dev libxrandr-dev
   ```

2. **wiringPi not found**: Install wiringPi library
   ```bash
   sudo apt install wiringpi
   ```

3. **Permission issues**: Make sure scripts are executable
   ```bash
   chmod +x build_release_raspi.sh clean_raspi.sh
   ```