#!/bin/bash
#
# EventCore Build Script
# Auto-clean build with fixed prefix
#

set -e  # Exit on error

# -------------------------------
# Configuration
# -------------------------------
BUILD_TYPE="Release"
INSTALL_PREFIX="/home/yubraj/EventCore/eventroot"
BUILD_DIR="build"
BUILD_SHARED="ON"
BUILD_TESTS="ON"
BUILD_EXAMPLES="ON"
BUILD_BENCHMARKS="OFF"
JOBS=$(nproc)
VERBOSE=false

# -------------------------------
# Color codes
# -------------------------------
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# -------------------------------
# Functions
# -------------------------------

print_msg() {
    local color=$1
    shift
    echo -e "${color}$@${NC}"
}

usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Simple EventCore build script with auto-clean and fixed prefix.

Options:
  --build-type TYPE     Build type: Debug, Release (default: Release)
  --jobs N              Number of parallel jobs (default: $(nproc))
  --verbose             Verbose build output
  --help                Show this help

Examples:
  $0                          # Standard release build
  $0 --build-type Debug       # Debug build
  $0 --jobs 4                 # Build with 4 jobs
  $0 --verbose                # Verbose output

Installation: Always installs to $INSTALL_PREFIX
Build: Always cleans and rebuilds from scratch
EOF
    exit 0
}

# -------------------------------
# Parse command line arguments
# -------------------------------
while [[ $# -gt 0 ]]; do
    case $1 in
        --build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        --jobs)
            JOBS="$2"
            shift 2
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            print_msg $RED "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# -------------------------------
# Validate build type
# -------------------------------
case $BUILD_TYPE in
    Debug|Release|RelWithDebInfo|MinSizeRel)
        ;;
    *)
        print_msg $RED "Invalid build type: $BUILD_TYPE"
        echo "Valid types: Debug, Release, RelWithDebInfo, MinSizeRel"
        exit 1
        ;;
esac

# -------------------------------
# Get script directory
# -------------------------------
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# -------------------------------
# Print configuration
# -------------------------------
print_msg $BLUE "========================================="
print_msg $BLUE "EventCore Auto-Build"
print_msg $BLUE "========================================="
echo "Build Type:          $BUILD_TYPE"
echo "Install Prefix:      $INSTALL_PREFIX"
echo "Build Directory:     $BUILD_DIR (always cleaned)"
echo "Parallel Jobs:       $JOBS"
echo ""
echo "Build Shared Libs:   $BUILD_SHARED"
echo "Build Tests:         $BUILD_TESTS"
echo "Build Examples:      $BUILD_EXAMPLES"
echo "Build Benchmarks:    $BUILD_BENCHMARKS"
echo "Verbose:             $VERBOSE"
print_msg $BLUE "========================================="
echo ""

# -------------------------------
# Clean and create build directory
# -------------------------------
print_msg $YELLOW "Cleaning previous build..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# -------------------------------
# Configure with CMake
# -------------------------------
print_msg $GREEN "Configuring CMake..."
CMAKE_ARGS=(
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
    -DBUILD_SHARED_LIBS="$BUILD_SHARED"
    -DBUILD_TESTS="$BUILD_TESTS"
    -DBUILD_EXAMPLES="$BUILD_EXAMPLES"
    -DBUILD_BENCHMARKS="$BUILD_BENCHMARKS"
)

if [ "$VERBOSE" = true ]; then
    CMAKE_ARGS+=(-DCMAKE_VERBOSE_MAKEFILE=ON)
fi

cmake "${CMAKE_ARGS[@]}" ..

# -------------------------------
# Build
# -------------------------------
print_msg $GREEN "Building with $JOBS jobs..."
MAKE_ARGS="-j${JOBS}"

if [ "$VERBOSE" = true ]; then
    MAKE_ARGS="VERBOSE=1 ${MAKE_ARGS}"
fi

make $MAKE_ARGS
print_msg $GREEN "Build completed successfully!"

# -------------------------------
# Install
# -------------------------------
print_msg $GREEN "Installing to $INSTALL_PREFIX..."
make install
print_msg $GREEN "Installation completed!"

# -------------------------------
# Verify installation
# -------------------------------
print_msg $GREEN "Verifying installation..."
echo ""
echo "Installed components:"

if [ -f "$INSTALL_PREFIX/bin/eventcore_server" ]; then
    echo "  ✓ eventcore_server"
else
    print_msg $RED "  ✗ eventcore_server not found!"
fi

if [ -d "$INSTALL_PREFIX/bin/examples" ]; then
    EXAMPLE_COUNT=$(ls "$INSTALL_PREFIX/bin/examples" 2>/dev/null | wc -l)
    echo "  ✓ examples ($EXAMPLE_COUNT executables)"
fi

if [ -d "$INSTALL_PREFIX/bin/tests" ]; then
    TEST_COUNT=$(ls "$INSTALL_PREFIX/bin/tests" 2>/dev/null | wc -l)
    echo "  ✓ tests ($TEST_COUNT executables)"
fi

if [ -f "$INSTALL_PREFIX/lib/libeventcore_static.a" ]; then
    echo "  ✓ libeventcore_static.a"
fi

if [ -f "$INSTALL_PREFIX/lib/libeventcore.so" ]; then
    echo "  ✓ libeventcore.so"
fi

if [ -d "$INSTALL_PREFIX/include/eventcore" ]; then
    HEADER_COUNT=$(find "$INSTALL_PREFIX/include/eventcore" -name "*.h" 2>/dev/null | wc -l)
    echo "  ✓ headers ($HEADER_COUNT header files)"
fi

if [ -d "$INSTALL_PREFIX/logs" ]; then
    echo "  ✓ logs directory"
fi

# -------------------------------
# Print usage instructions
# -------------------------------
echo ""
print_msg $BLUE "========================================="
print_msg $GREEN "Build Complete!"
print_msg $BLUE "========================================="
echo ""
echo "Quick start:"
echo "  Run server:    $INSTALL_PREFIX/bin/eventcore_server"
echo ""
echo "Available examples:"
if [ -d "$INSTALL_PREFIX/bin/examples" ]; then
    for example in "$INSTALL_PREFIX"/bin/examples/*; do
        if [ -x "$example" ]; then
        echo "  $(basename "$example")"
        fi
    done
fi
echo ""
echo "Environment setup:"
echo "  Add to your ~/.bashrc:"
echo "    export PATH=\"$INSTALL_PREFIX/bin:\$PATH\""
echo "    export LD_LIBRARY_PATH=\"$INSTALL_PREFIX/lib:\$LD_LIBRARY_PATH\""
echo ""
echo "Next time, simply run: ./build.sh"
print_msg $BLUE "========================================="
