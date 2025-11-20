#!/bin/bash
#
# EventCore Build Script
# Generic build script with all configuration options
#
# Usage: ./build.sh [OPTIONS]
#

set -e  # Exit on error

# -------------------------------
# Default values
# -------------------------------
BUILD_TYPE="Release"
INSTALL_PREFIX=""
BUILD_DIR="build"
BUILD_SHARED="ON"
BUILD_TESTS="ON"
BUILD_EXAMPLES="ON"
BUILD_BENCHMARKS="OFF"
CLEAN_BUILD=false
INSTALL_AFTER_BUILD=false
RUN_TESTS=false
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

# Print colored message
print_msg() {
    local color=$1
    shift
    echo -e "${color}$@${NC}"
}

# Print usage/help
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Build Configuration:
  --build-type TYPE         Build type: Debug, Release, RelWithDebInfo, MinSizeRel
                            (default: Release)
  --prefix PATH             Install prefix path
                            (default: \${BUILD_DIR} for local install)
  --build-dir DIR           Build directory name (default: build)
  --jobs N                  Number of parallel jobs (default: $(nproc))

Build Options:
  --shared [ON|OFF]         Build shared libraries (default: ON)
  --tests [ON|OFF]          Build tests (default: ON)
  --examples [ON|OFF]       Build examples (default: ON)
  --benchmarks [ON|OFF]     Build benchmarks (default: OFF)

Actions:
  --clean                   Clean build directory before building
  --install                 Install after successful build
  --test                    Run tests after build
  --verbose                 Verbose build output

Examples:
  # Debug build with local install
  $0 --build-type Debug --install

  # Release build, install to /opt/eventcore
  $0 --build-type Release --prefix /opt/eventcore --install

  # Clean release build with tests
  $0 --clean --build-type Release --test

  # Minimal build (no tests, no examples)
  $0 --tests OFF --examples OFF --benchmarks OFF

  # Development build
  $0 --build-type Debug --clean --install --test --verbose

  # Production build
  $0 --build-type Release --prefix /usr/local --clean --install

Help:
  -h, --help                Show this help message

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
        --prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --jobs)
            JOBS="$2"
            shift 2
            ;;
        --shared)
            BUILD_SHARED="$2"
            shift 2
            ;;
        --tests)
            BUILD_TESTS="$2"
            shift 2
            ;;
        --examples)
            BUILD_EXAMPLES="$2"
            shift 2
            ;;
        --benchmarks)
            BUILD_BENCHMARKS="$2"
            shift 2
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --install)
            INSTALL_AFTER_BUILD=true
            shift
            ;;
        --test)
            RUN_TESTS=true
            shift
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
# Get script directory (project root)
# -------------------------------
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Set install prefix to build dir if not specified
if [ -z "$INSTALL_PREFIX" ]; then
    INSTALL_PREFIX="${SCRIPT_DIR}/${BUILD_DIR}"
    print_msg $YELLOW "Install prefix not specified, using: ${INSTALL_PREFIX}"
fi

# -------------------------------
# Print configuration
# -------------------------------
print_msg $BLUE "========================================="
print_msg $BLUE "EventCore Build Configuration"
print_msg $BLUE "========================================="
echo "Build Type:          $BUILD_TYPE"
echo "Install Prefix:      $INSTALL_PREFIX"
echo "Build Directory:     $BUILD_DIR"
echo "Parallel Jobs:       $JOBS"
echo ""
echo "Build Shared Libs:   $BUILD_SHARED"
echo "Build Tests:         $BUILD_TESTS"
echo "Build Examples:      $BUILD_EXAMPLES"
echo "Build Benchmarks:    $BUILD_BENCHMARKS"
echo ""
echo "Clean Build:         $CLEAN_BUILD"
echo "Install After Build: $INSTALL_AFTER_BUILD"
echo "Run Tests:           $RUN_TESTS"
echo "Verbose:             $VERBOSE"
print_msg $BLUE "========================================="
echo ""

# -------------------------------
# Clean build directory if requested
# -------------------------------
if [ "$CLEAN_BUILD" = true ]; then
    print_msg $YELLOW "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# -------------------------------
# Create build directory
# -------------------------------
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# -------------------------------
# Configure with CMake
# -------------------------------
print_msg $GREEN "Configuring with CMake..."
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
print_msg $GREEN "Building..."
MAKE_ARGS="-j${JOBS}"

if [ "$VERBOSE" = true ]; then
    MAKE_ARGS="VERBOSE=1 ${MAKE_ARGS}"
fi

make $MAKE_ARGS
print_msg $GREEN "Build completed successfully!"

# -------------------------------
# Run tests if requested
# -------------------------------
if [ "$RUN_TESTS" = true ]; then
    print_msg $GREEN "Running tests..."
    ctest --output-on-failure
    print_msg $GREEN "All tests passed!"
fi

# -------------------------------
# Install if requested
# -------------------------------
if [ "$INSTALL_AFTER_BUILD" = true ]; then
    print_msg $GREEN "Installing..."
    
    if [ ! -w "$(dirname "$INSTALL_PREFIX")" ] && [ "$INSTALL_PREFIX" != "${SCRIPT_DIR}/${BUILD_DIR}" ]; then
        print_msg $YELLOW "Installing to system location, may require sudo..."
        sudo make install
    else
        make install
    fi
    
    print_msg $GREEN "Installation completed successfully!"
    print_msg $GREEN "Installed to: $INSTALL_PREFIX"
fi

# -------------------------------
# Print summary
# -------------------------------
echo ""
print_msg $BLUE "========================================="
print_msg $BLUE "Build Summary"
print_msg $BLUE "========================================="
echo "Build completed in: ${BUILD_DIR}"
echo ""

if [ "$INSTALL_AFTER_BUILD" = true ]; then
    echo "Installation structure:"
    echo "  Executables: ${INSTALL_PREFIX}/bin/"
    echo "  Libraries:   ${INSTALL_PREFIX}/lib/"
    echo "  Headers:     ${INSTALL_PREFIX}/include/"
    echo ""
fi

echo "Built targets:"
if [ -f "bin/eventcore_server" ]; then
    echo "  ✓ eventcore_server"
fi
if [ -d "bin/examples" ]; then
    echo "  ✓ examples ($(ls bin/examples/ 2>/dev/null | wc -l) executables)"
fi
if [ -d "bin/tests" ]; then
    echo "  ✓ tests ($(ls bin/tests/ 2>/dev/null | wc -l) test executables)"
fi
if [ -f "lib/libeventcore_static.a" ]; then
    echo "  ✓ static library"
fi
if [ -f "lib/libeventcore.so" ]; then
    echo "  ✓ shared library"
fi

echo ""
print_msg $GREEN "Quick Commands:"
echo "  Run server:       ${BUILD_DIR}/bin/eventcore_server"
if [ "$BUILD_EXAMPLES" = "ON" ]; then
    echo "  Run example:      ${BUILD_DIR}/bin/examples/example_server"
fi
if [ "$BUILD_TESTS" = "ON" ]; then
    echo "  Run tests:        cd ${BUILD_DIR} && ctest --output-on-failure"
fi
if [ "$INSTALL_AFTER_BUILD" = true ] && [ "$INSTALL_PREFIX" != "${SCRIPT_DIR}/${BUILD_DIR}" ]; then
    echo "  Run installed:    ${INSTALL_PREFIX}/bin/eventcore_server"
    echo ""
    echo "  Add to PATH:      export PATH=\"${INSTALL_PREFIX}/bin:\$PATH\""
fi

echo ""
print_msg $BLUE "========================================="
print_msg $GREEN "Done!"
print_msg $BLUE "========================================="
