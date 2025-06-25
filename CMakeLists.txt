# Simple C-based 3D Game Engine Skeleton

# === Directory Layout ===
# project/
# ├── src/
# │   ├── main.c
# │   ├── render.c / render.h     # Raylib or OpenGL rendering
# │   ├── physics.c / physics.h   # ODE physics integration
# │   ├── audio.c / audio.h       # Raylib audio
# │   ├── assets/                 # Models, textures, sounds
# ├── include/                   # Headers for external libraries (if needed)
# ├── libs/                      # External single-file libraries or submodules
# ├── CMakeLists.txt             # Or Makefile

# === External Dependencies ===
# Recommended as single-header or vendored in libs/:
# - cglm
# - stb_image.h
# - tinyobjloader-c
# - miniaudio.h or OpenAL
# - ode (Open Dynamics Engine)

# === Build System: CMakeLists.txt ===

# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(SimpleGameEngine C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

add_subdirectory(libs/raylib)

# Use system-installed ODE
find_library(ODE_LIBRARY ode REQUIRED)
include_directories(/usr/include/ode)

# Add OpenMP
find_package(OpenMP REQUIRED)

include_directories(libs/raylib/src)
include_directories(include libs)

file(GLOB SRC src/*.c)

add_executable(game ${SRC})
target_link_libraries(game
    PUBLIC
    raylib
    ${ODE_LIBRARY}
    OpenMP::OpenMP_C
    m
    pthread
    dl
    rt
    X11
)

