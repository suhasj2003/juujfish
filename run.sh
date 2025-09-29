#!/bin/bash

set -e

# Variables
CMAKE_PRESET="make-release"
RUN_ENGINE=true

ROOT_DIR=$(pwd)
BUILD_DIR="$ROOT_DIR/build"

# Check for debug mode or compile only flags
for arg in "$@"; do
    if [[ $arg == "-d" ]]; then
        CMAKE_PRESET="make-debug"
    elif [[ $arg == "-c" ]]; then
        RUN_ENGINE=false
    fi
done

# Create build directory if it doesn't exist
echo "Preparing build directory..."
mkdir -p "$BUILD_DIR"

# Execute CMake with the specified preset
echo "Executing CMake with preset: $CMAKE_PRESET"
(
    cd "$BUILD_DIR"
    cmake --preset "$CMAKE_PRESET" "$ROOT_DIR" >> /dev/null
)

# Build the executable
echo "Building the executable..."
(
    cd "$BUILD_DIR"
    make >> /dev/null
)

# Run the engine if not in compile-only mode
if $RUN_ENGINE; then
    echo "Running the engine..."
    ./chessengine
else
    echo "Build completed. You can run the engine with ./chessengine."
fi
