cmake_minimum_required(VERSION 3.16)
project(PInball VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Custom target to update version before building
add_custom_target(update_version
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/update_version.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Updating build version number..."
)

# Define source files
set(PINBALL_SOURCES
    src/3rdparty/stb_image.cpp
    src/PBOGLES.cpp
    src/PBGfx.cpp
    src/PBRaspPiRender.cpp
    src/PBDebounce.cpp
    src/Pinball_IO.cpp
    src/Pinball_Table.cpp
    src/PBSound.cpp
    src/PinballMenus.cpp
    src/Pinball_Engine.cpp
    src/Pinball.cpp
)

# Create the executable
add_executable(Pinball ${PINBALL_SOURCES})

# Make the executable depend on version update
add_dependencies(Pinball update_version)

# Include directories
target_include_directories(Pinball PRIVATE
    src/include_ogl_raspi
    src/3rdparty
    /usr/include/EGL
    /usr/include/GLES3
    /usr/include/KHR
    /usr/include/X11
    /usr/include/X11/extensions
)

# Release build optimizations (matching tasks.json settings)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(Pinball PRIVATE
        -O3                    # Maximum optimization
        -march=native          # Optimize for target CPU
        -mtune=native          # Tune for target CPU
        -flto                  # Link-time optimization
        -ffast-math            # Fast math optimizations
        -funroll-loops         # Unroll loops
        -DNDEBUG               # Define NDEBUG for release
    )
    
    # Link-time optimization for release
    set_target_properties(Pinball PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION TRUE
        LINK_FLAGS "-s"  # Strip symbols for smaller binary
    )
endif()

# Debug build settings
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(Pinball PRIVATE
        -g                     # Debug information
        -O0                    # No optimization for debugging
    )
endif()

# Link libraries
target_link_libraries(Pinball
    EGL
    GLESv2
    pthread
    m
    X11
    Xrandr
    wiringPi
    SDL2
    SDL2_mixer
)

# Set output directory
set_target_properties(Pinball PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/raspibuild/release
)

# Create output directory if it doesn't exist
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/raspibuild/release)

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Output directory: ${CMAKE_SOURCE_DIR}/raspibuild/release")

# Custom target for easy building
add_custom_target(build_release
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config Release
    DEPENDS Pinball
    COMMENT "Building optimized release version..."
)