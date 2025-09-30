#!/bin/bash
set -e
BUILD_TYPE="Release"
BUILD_DIR="build"
ENABLE_TESTS=1
ENABLE_EXAMPLES=1

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug) BUILD_TYPE="Debug"; shift ;;
        --clean) echo "Cleaning build directory..."; rm -rf $BUILD_DIR; shift ;;
        --no-tests) ENABLE_TESTS=0; shift ;;
        --no-examples) ENABLE_EXAMPLES=0; shift ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

echo "Building EventCore..."
echo "Build type: $BUILD_TYPE"
mkdir -p $BUILD_DIR
cd $BUILD_DIR

CMAKE_OPTIONS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
if [ $ENABLE_TESTS -eq 1 ]; then CMAKE_OPTIONS="$CMAKE_OPTIONS -DBUILD_TESTS=ON"; fi
if [ $ENABLE_EXAMPLES -eq 1 ]; then CMAKE_OPTIONS="$CMAKE_OPTIONS -DBUILD_EXAMPLES=ON"; fi

cmake $CMAKE_OPTIONS ..
make -j$(nproc)
echo "Build completed successfully!"
