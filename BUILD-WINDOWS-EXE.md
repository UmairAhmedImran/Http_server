# Building Windows .exe

This guide explains how to create a Windows executable (.exe) file.

## Method 1: Cross-Compilation (Linux/macOS)

### Prerequisites

Install MinGW-w64 cross-compiler:

**macOS:**
```bash
brew install mingw-w64
```

**Ubuntu/Debian:**
```bash
sudo apt-get install mingw-w64
```

**Fedora/CentOS:**
```bash
sudo dnf install mingw64-gcc
```

### Build

```bash
./build-windows.sh
```

This creates `build/windows/httplb.exe`

### Manual Build

```bash
# 64-bit
x86_64-w64-mingw32-gcc -Wall -Wextra -Iinclude \
    src/*.c -o httplb.exe -lpthread -static

# 32-bit
i686-w64-mingw32-gcc -Wall -Wextra -Iinclude \
    src/*.c -o httplb.exe -lpthread -static
```

## Method 2: Native Windows Build

### Using WSL (Recommended)

```bash
# In WSL
tar -xzf http-loadbalancer-1.0.0.tar.gz
cd http-loadbalancer-1.0.0
./configure && make
# Binary is at build/server (works in WSL)
```

### Using MinGW on Windows

1. Install MSYS2: https://www.msys2.org/
2. Install MinGW:
   ```bash
   pacman -S mingw-w64-x86_64-gcc
   pacman -S mingw-w64-x86_64-pthreads
   ```
3. Build:
   ```bash
   gcc -Wall -Wextra -Iinclude src/*.c -o httplb.exe -lpthread
   ```

## Method 3: Pre-built Binary

For distribution, you can provide pre-built Windows binaries:
- `httplb-windows-x64.exe` (64-bit)
- `httplb-windows-x86.exe` (32-bit)

## Using the .exe

1. Copy `httplb.exe` to Windows
2. Copy `config.example` to `config`
3. Edit `config` with your backend servers
4. Run:
   ```cmd
   httplb.exe -c config
   ```

## Distribution

Include the .exe in your GitHub releases:
- `httplb-windows-x64.exe`
- `httplb-windows-x86.exe`

Users can download and run directly without building.

