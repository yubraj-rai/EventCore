#!/bin/bash
set -e
if ! command -v clang-format &> /dev/null; then echo "Error: clang-format is not installed"; exit 1; fi
find . -name "*.h" -o -name "*.cpp" | grep -v build/ | xargs clang-format -i -style=file
echo "Code formatting completed"
