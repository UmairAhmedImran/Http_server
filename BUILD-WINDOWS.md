# Building on Windows

This guide explains how to build the HTTP Load Balancer on Windows.

## Prerequisites

You need one of the following:

### Option 1: WSL (Windows Subsystem for Linux) - Recommended

WSL allows you to run Linux on Windows, making the build process identical to Linux.

1. **Install WSL:**
   ```powershell
   wsl --install
   ```
   Or follow: https://docs.microsoft.com/en-us/windows/wsl/install

2. **Use Linux build instructions:**
   ```bash
   # In WSL terminal
   tar -xzf http-loadbalancer-1.0.0.tar.gz
   cd http-loadbalancer-1.0.0
   ./configure
   make
   ```

### Option 2: MinGW-w64 (Native Windows Build)

MinGW provides GCC compiler for Windows.

1. **Install MinGW-w64:**
   - Download from: https://www.mingw-w64.org/downloads/
   - Or use MSYS2: https://www.msys2.org/
   - Or use Chocolatey: `choco install mingw`

2. **Add to PATH:**
   - Add MinGW `bin` directory to your PATH environment variable

3. **Install pthread library:**
   ```bash
   # Using MSYS2
   pacman -S mingw-w64-x86_64-gcc
   pacman -S mingw-w64-x86_64-pthreads
   ```

4. **Build:**
   ```bash
   # Extract the .zip file
   unzip http-loadbalancer-1.0.0.zip
   cd http-loadbalancer-1.0.0
   
   # Build manually (configure script may not work on Windows)
   gcc -Wall -Iinclude src/*.c -o http-loadbalancer.exe -lpthread
   ```

### Option 3: Visual Studio (Requires Code Changes)

Visual Studio requires modifications to the code for Windows sockets.

**Note:** The current code uses POSIX sockets and won't compile directly with MSVC without modifications.

If you want to use Visual Studio:
1. Replace POSIX socket calls with Winsock2
2. Replace `pthread` with Windows threads
3. Modify signal handling for Windows

This is not recommended unless you need native Windows support.

## Quick Start with WSL

```bash
# 1. Extract
tar -xzf http-loadbalancer-1.0.0.tar.gz
cd http-loadbalancer-1.0.0

# 2. Configure and build
./configure
make

# 3. Run
./build/server -c config.example
```

## Building with MinGW

```bash
# Extract
unzip http-loadbalancer-1.0.0.zip
cd http-loadbalancer-1.0.0

# Build
gcc -Wall -Wextra -Iinclude \
    src/http.c src/server.c src/backend.c \
    src/main.c src/logging.c src/config.c \
    -o http-loadbalancer.exe -lpthread

# Run
./http-loadbalancer.exe -c config.example
```

## Using Pre-built Binary (Future)

For convenience, we may provide pre-built Windows binaries in future releases:
- `http-loadbalancer-windows-x64.exe`
- `http-loadbalancer-windows-x86.exe`

## Troubleshooting

### "pthread not found"

**MinGW/MSYS2:**
```bash
pacman -S mingw-w64-x86_64-pthreads
```

**Or compile with:**
```bash
gcc ... -lpthread -static
```

### "configure: command not found"

The `configure` script is a bash script. Use WSL or MSYS2 bash:
```bash
bash configure
```

### "Permission denied"

On Windows, you may need to run as Administrator or adjust file permissions.

## Recommended Approach

**For most users:** Use WSL (Windows Subsystem for Linux)
- Easiest setup
- Identical to Linux build process
- No code modifications needed
- Full compatibility

**For native Windows:** Use MinGW-w64
- Requires manual build steps
- May need code adjustments
- More complex setup

## Running on Windows

After building, you can run the load balancer:

```bash
# WSL
./build/server -c config.example

# MinGW (native)
./http-loadbalancer.exe -c config.example
```

## Configuration

The configuration file format is the same on all platforms:

```
127.0.0.1:9001:3
127.0.0.1:9002:2
127.0.0.1:9003:1
```

## Notes

- The load balancer uses POSIX sockets, which work in WSL
- For native Windows, consider using WSL for the best experience
- Windows-specific builds may be provided in future releases

