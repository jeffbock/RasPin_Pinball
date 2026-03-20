#!/bin/bash
# Clean script for CMake build artifacts

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}🧹 Cleaning build artifacts...${NC}"

# Remove build directories
if [ -d "build_release" ]; then
    echo -e "${YELLOW}🗑️  Removing build_release directory...${NC}"
    rm -rf build_release
fi

if [ -d "build" ]; then
    echo -e "${YELLOW}🗑️  Removing build directory...${NC}"
    rm -rf build
fi

# Remove output binaries (optional - comment out if you want to keep them)
# if [ -f "raspibuild/release/Pinball" ]; then
#     echo -e "${YELLOW}🗑️  Removing release binary...${NC}"
#     rm -f raspibuild/release/Pinball
# fi

echo -e "${GREEN}✅ Clean completed!${NC}"