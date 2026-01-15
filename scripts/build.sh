#!/bin/bash

# Build script for v5.cpp - Works on Mac and Linux
# Usage: ./scripts/build.sh [release|debug]

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# Build mode (default: release)
BUILD_MODE="${1:-release}"

# Detect OS
OS="$(uname -s)"
case "${OS}" in
    Linux*)     OS_TYPE="Linux";;
    Darwin*)    OS_TYPE="Mac";;
    *)          echo -e "${RED}Error: Unsupported OS: ${OS}${NC}" >&2; exit 1;;
esac

echo -e "${GREEN}Building v5.cpp for ${OS_TYPE}...${NC}"

# Detect compiler
if command -v g++ &> /dev/null; then
    COMPILER="g++"
    COMPILER_VERSION=$(g++ --version | head -n1)
elif command -v clang++ &> /dev/null; then
    COMPILER="clang++"
    COMPILER_VERSION=$(clang++ --version | head -n1)
else
    echo -e "${RED}Error: No C++ compiler found (g++ or clang++)${NC}" >&2
    exit 1
fi

echo -e "Compiler: ${COMPILER_VERSION}"

# Set compiler flags based on build mode
if [ "$BUILD_MODE" = "debug" ]; then
    CXXFLAGS="-std=c++17 -Wall -Wextra -g -O0 -DDEBUG"
    echo -e "${YELLOW}Build mode: DEBUG${NC}"
else
    CXXFLAGS="-std=c++17 -Wall -Wextra -O3 -DNDEBUG"
    echo -e "${GREEN}Build mode: RELEASE${NC}"
fi

# Create bin directory if it doesn't exist
mkdir -p bin

# Source and output files
SOURCE_FILE="v5.cpp"
OUTPUT_FILE="bin/v5"

# Compile
echo -e "Compiling ${SOURCE_FILE}..."
echo -e "Command: ${COMPILER} ${CXXFLAGS} -o ${OUTPUT_FILE} ${SOURCE_FILE}"

if ${COMPILER} ${CXXFLAGS} -o "${OUTPUT_FILE}" "${SOURCE_FILE}"; then
    # Make executable
    chmod +x "${OUTPUT_FILE}"
    
    # Get file size
    if [ "$OS_TYPE" = "Mac" ]; then
        SIZE=$(stat -f%z "${OUTPUT_FILE}")
    else
        SIZE=$(stat -c%s "${OUTPUT_FILE}")
    fi
    
    SIZE_MB=$(echo "scale=2; $SIZE / 1024 / 1024" | bc)
    
    echo -e "${GREEN}✓ Build successful!${NC}"
    echo -e "Output: ${OUTPUT_FILE}"
    echo -e "Size: ${SIZE_MB} MB"
    echo -e ""
    echo -e "Run with: ${OUTPUT_FILE} -m <map> [options]"
else
    echo -e "${RED}✗ Build failed!${NC}" >&2
    exit 1
fi

