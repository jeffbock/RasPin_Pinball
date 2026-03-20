cmake_minimum_required(VERSION 3.16)
project(Pinball VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Build options ------------------------------------------------------------
# BUILD_TARGET: RASPI (Pi hardware + wiringPi) | DEBIAN (Linux simulator, no wiringPi)
set(BUILD_TARGET "RASPI" CACHE STRING "Build target platform: RASPI or DEBIAN")
set_property(CACHE BUILD_TARGET PROPERTY STRINGS RASPI DEBIAN)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# --- Paths --------------------------------------------------------------------
set(ROOT ${CMAKE_SOURCE_DIR}/..)
set(SRC  ${ROOT}/src)

# --- Pre-build: increment version + regenerate IO header ---------------------
add_custom_target(prebuild
    COMMAND python3 ${ROOT}/scripts/update_version.py
    COMMAND python3 ${ROOT}/scripts/generate_io_header.py
    WORKING_DIRECTORY ${ROOT}
    COMMENT "Updating version and generating IO header..."
)

# --- Source files -------------------------------------------------------------
set(PINBALL_SOURCES
    # Third-party (not user-editable)
    ${SRC}/3rdparty/stb_image.cpp
    ${SRC}/3rdparty/cgltf.cpp
    # System files (framework internals -- do not edit)
    ${SRC}/system/PBOGLES.cpp
    ${SRC}/system/PB3D.cpp
    ${SRC}/system/PBGfx.cpp
    ${SRC}/system/PBLinuxRender.cpp
    ${SRC}/system/PBDebounce.cpp
    ${SRC}/system/Pinball_IO.cpp
    ${SRC}/system/Pinball_Table.cpp
    ${SRC}/system/PBSound.cpp
    ${SRC}/system/PBVideo.cpp
    ${SRC}/system/PBVideoPlayer.cpp
    ${SRC}/system/PinballMenus.cpp
    ${SRC}/system/Pinball_Engine.cpp
    ${SRC}/system/Pinball.cpp
    # User files (your game logic -- edit these)
    ${SRC}/user/PBDevice.cpp
    ${SRC}/user/PBSequences.cpp
    ${SRC}/user/tablemodes/Pinball_Table_ModeInit.cpp
    ${SRC}/user/tablemodes/Pinball_Table_ModeStart.cpp
    ${SRC}/user/tablemodes/Pinball_Table_ModeMain.cpp
    ${SRC}/user/tablemodes/Pinball_Table_ModeReset.cpp
    ${SRC}/user/tablemodes/Pinball_Table_ModeGameEnd.cpp
    ${SRC}/user/tablemodes/Pinball_Table_ModePlayerEnd.cpp
)

# --- Output location ----------------------------------------------------------
if(BUILD_TARGET STREQUAL "RASPI")
    set(EXE_NAME  Pinball)
    set(OUT_DIR   ${ROOT}/build/raspi)
else()
    set(EXE_NAME  Pinball_DebianOS)
    set(OUT_DIR   ${ROOT}/build/debian)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(OUT_DIR ${OUT_DIR}/release)
else()
    set(OUT_DIR ${OUT_DIR}/debug)
endif()

file(MAKE_DIRECTORY ${OUT_DIR})

# --- Main executable ----------------------------------------------------------
add_executable(${EXE_NAME} ${PINBALL_SOURCES})
add_dependencies(${EXE_NAME} prebuild)
set_target_properties(${EXE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})

target_include_directories(${EXE_NAME} PRIVATE
    ${SRC}
    ${SRC}/system
    ${SRC}/user
    ${SRC}/3rdparty
    ${SRC}/include_ogl_raspi
    /usr/include/EGL
    /usr/include/GLES3
    /usr/include/KHR
    /usr/include/X11
    /usr/include/X11/extensions
)

if(BUILD_TARGET STREQUAL "RASPI")
    target_compile_definitions(${EXE_NAME} PRIVATE EXE_MODE_RASPI)
else()
    target_compile_definitions(${EXE_NAME} PRIVATE EXE_MODE_DEBIAN)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${EXE_NAME} PRIVATE
        -O3 -march=native -mtune=native -flto -ffast-math -funroll-loops -DNDEBUG
    )
    target_link_options(${EXE_NAME} PRIVATE -s)
    set_property(TARGET ${EXE_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    target_compile_options(${EXE_NAME} PRIVATE -g -O0)
endif()

target_link_libraries(${EXE_NAME} PRIVATE
    EGL GLESv2 pthread m X11 Xrandr
    SDL2 SDL2_mixer
    avcodec avformat avutil swscale swresample
)
if(BUILD_TARGET STREQUAL "RASPI")
    target_link_libraries(${EXE_NAME} PRIVATE wiringPi)
endif()

# --- PBUtils ------------------------------------------------------------------
add_executable(FontGen
    ${SRC}/3rdparty/stb_truetype.cpp
    ${SRC}/3rdparty/stb_image_write.cpp
    ${SRC}/PButils/FontGen.cpp
)
target_include_directories(FontGen PRIVATE ${SRC}/3rdparty)
set_target_properties(FontGen PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})

if(BUILD_TARGET STREQUAL "RASPI")
    add_executable(pblistdevices ${SRC}/PButils/pblistdevices.cpp)
    target_link_libraries(pblistdevices PRIVATE wiringPi pthread)
    set_target_properties(pblistdevices PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})

    add_executable(pbsetamp ${SRC}/PButils/pbsetamp.cpp)
    target_link_libraries(pbsetamp PRIVATE wiringPi pthread)
    set_target_properties(pbsetamp PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})
endif()

# pblaunch: GTK4 GUI launcher - builds on RASPI and DEBIAN if GTK4 is available
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(GTK4 QUIET IMPORTED_TARGET gtk4)
    if(GTK4_FOUND)
        add_executable(pblaunch ${SRC}/PButils/pblaunch.cpp)
        target_link_libraries(pblaunch PRIVATE PkgConfig::GTK4)
        set_target_properties(pblaunch PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})
        install(TARGETS pblaunch DESTINATION /usr/local/bin)
        message(STATUS "pblaunch: GTK4 found, will build")
    else()
        message(STATUS "pblaunch: GTK4 not found, skipping")
    endif()
else()
    message(STATUS "pblaunch: PkgConfig not found, skipping")
endif()

# --- Status -------------------------------------------------------------------
message(STATUS "Target : ${BUILD_TARGET}")
message(STATUS "Build  : ${CMAKE_BUILD_TYPE}")
message(STATUS "Output : ${OUT_DIR}/${EXE_NAME}")