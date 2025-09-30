#!/usr/bin/env python3
"""
Simple cross-platform build number updater for Pinball project
Works on both Windows and Raspberry Pi

Only updates the build number automatically.
Major and minor versions are managed manually in PInball.h
"""

import os
import re
import sys
from datetime import datetime

def read_build_counter():
    """Read or create build counter"""
    counter_file = "build_counter.txt"
    try:
        with open(counter_file, 'r') as f:
            return int(f.read().strip())
    except (FileNotFoundError, ValueError):
        return 0

def write_build_counter(counter):
    """Write build counter to file"""
    with open("build_counter.txt", 'w') as f:
        f.write(str(counter))

def update_version_in_header(header_file):
    """Update only the build number in the header file"""
    
    # Read current header file
    try:
        with open(header_file, 'r') as f:
            content = f.read()
    except FileNotFoundError:
        print(f"Error: {header_file} not found!")
        return False
    
    # Get and increment build counter
    build_number = read_build_counter() + 1
    write_build_counter(build_number)
    
    # Extract current major and minor version numbers (read-only)
    major_match = re.search(r'#define PB_VERSION_MAJOR\s+(\d+)', content)
    minor_match = re.search(r'#define PB_VERSION_MINOR\s+(\d+)', content)
    
    if major_match and minor_match:
        # Get current version numbers for display
        major = int(major_match.group(1))
        minor = int(minor_match.group(1))
        
        # Only update the build number
        content = re.sub(r'#define PB_VERSION_BUILD\s+\d+', f'#define PB_VERSION_BUILD {build_number}', content)
        
        # Write updated content back to file
        with open(header_file, 'w') as f:
            f.write(content)
        
        print(f"✅ Build number updated to {build_number} (Version: v{major}.{minor}.{build_number})")
        return True
        
    else:
        print("❌ Version defines not found in the expected format")
        return False

def main():
    # Change to script directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.dirname(script_dir)
    os.chdir(project_dir)
    
    # Update version in header file
    header_file = "src/Pinball.h"
    
    if not os.path.exists(header_file):
        print(f"❌ Error: {header_file} not found!")
        print(f"Current directory: {os.getcwd()}")
        sys.exit(1)
    
    success = update_version_in_header(header_file)
    
    if success:
        print(f"🎯 Ready to build with updated build number!")
    else:
        print("❌ Failed to update build number")
        sys.exit(1)

if __name__ == "__main__":
    main()