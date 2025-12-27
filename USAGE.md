# Usage Guide

## Quick Start

After installation, the load balancer is available as `httplb`:

```bash
# Install
sudo make install

# Run
httplb -c /usr/local/etc/loadbalancer.conf
```

## Command Name

- **After installation**: `httplb` (global command)
- **From build directory**: `./build/server` (development)

## Building Windows .exe

### Option 1: Cross-Compilation (from Linux/macOS)

```bash
# Install MinGW-w64
# macOS:
brew install mingw-w64

# Ubuntu/Debian:
sudo apt-get install mingw-w64

# Build Windows .exe
./build-windows.sh
```

This creates `build/windows/httplb.exe`

### Option 2: Native Windows Build

See [BUILD-WINDOWS-EXE.md](BUILD-WINDOWS-EXE.md) for detailed instructions.

## Running After Installation

```bash
# 1. Configure backends
sudo cp /usr/local/etc/loadbalancer.conf.example /usr/local/etc/loadbalancer.conf
sudo nano /usr/local/etc/loadbalancer.conf

# 2. Run
httplb -c /usr/local/etc/loadbalancer.conf

# Or with custom port
httplb -p 3000 -c /usr/local/etc/loadbalancer.conf
```

## Development Mode

```bash
# Build
./configure && make

# Run from build directory
./build/server -c config.example
```

