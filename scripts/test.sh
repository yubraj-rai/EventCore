#!/bin/bash
set -e
BUILD_DIR="build"
if [ ! -d "$BUILD_DIR" ]; then echo "Build directory not found. Run build.sh first."; exit 1; fi
cd $BUILD_DIR
echo "Running tests..."
ctest --output-on-failure
if [ $? -eq 0 ]; then echo "All tests passed!"; else echo "Some tests failed!"; exit 1; fi
