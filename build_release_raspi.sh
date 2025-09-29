#!/bin/bash
# Build script for Raspberry Pi release version using CMake

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}🔨 Building PInball for Raspberry Pi (Release)${NC}"

# Create build directory
BUILD_DIR="build_release"
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}📁 Creating build directory: $BUILD_DIR${NC}"
    mkdir -p "$BUILD_DIR"
fi

# Change to build directory
cd "$BUILD_DIR"

echo -e "${YELLOW}⚙️  Configuring CMake...${NC}"
# Configure CMake for Release build
cmake -DCMAKE_BUILD_TYPE=Release ..

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ CMake configuration successful${NC}"
    
    echo -e "${YELLOW}🏗️  Building project...${NC}"
    # Build the project
    make -j$(nproc)
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}🎉 Build completed successfully!${NC}"
        echo -e "${GREEN}📦 Executable location: raspibuild/release/Pinball${NC}"
        
        # Show file info
        if [ -f "../raspibuild/release/Pinball" ]; then
            echo -e "${YELLOW}📋 File information:${NC}"
            ls -lh ../raspibuild/release/Pinball
        fi
    else
        echo -e "${RED}❌ Build failed!${NC}"
        exit 1
    fi
else
    echo -e "${RED}❌ CMake configuration failed!${NC}"
    exit 1
fi

echo -e "${GREEN}✨ Build process complete!${NC}"