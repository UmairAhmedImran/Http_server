#!/bin/bash

# Script to build Windows .exe using MinGW cross-compiler
# Usage: ./build-windows.sh

echo "Building Windows executable..."

# Check for MinGW cross-compiler
if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
    CC="x86_64-w64-mingw32-gcc"
    TARGET="x86_64-w64-mingw32"
    echo "Found MinGW-w64 cross-compiler"
elif command -v i686-w64-mingw32-gcc >/dev/null 2>&1; then
    CC="i686-w64-mingw32-gcc"
    TARGET="i686-w64-mingw32"
    echo "Found MinGW-w32 cross-compiler"
else
    echo "Error: MinGW cross-compiler not found!"
    echo ""
    echo "Install MinGW-w64:"
    echo "  macOS: brew install mingw-w64"
    echo "  Ubuntu/Debian: sudo apt-get install mingw-w64"
    echo "  Or download from: https://www.mingw-w64.org/downloads/"
    exit 1
fi

# Create Windows build directory
mkdir -p build/windows

# Compile for Windows
echo "Compiling for Windows..."
$CC -Wall -Wextra -Iinclude \
    src/http.c src/server.c src/backend.c \
    src/main.c src/logging.c src/config.c \
    -o build/windows/httplb.exe \
    -lpthread \
    -static-libgcc -static-libstdc++

if [ $? -eq 0 ]; then
    echo ""
    echo "Build successful!"
    echo "Windows executable: build/windows/httplb.exe"
    echo ""
    echo "To test on Windows:"
    echo "  1. Copy httplb.exe to Windows machine"
    echo "  2. Copy config.example to config"
    echo "  3. Edit config with your backend servers"
    echo "  4. Run: httplb.exe -c config"
else
    echo ""
    echo "Build failed!"
    exit 1
fi

