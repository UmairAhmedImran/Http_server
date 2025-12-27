# Building on macOS

Building on macOS is straightforward and similar to Linux.

## Prerequisites

macOS comes with most tools pre-installed, but you may need:

### Install Xcode Command Line Tools

```bash
xcode-select --install
```

This installs:
- GCC/Clang compiler
- Make
- Other development tools

## Installation

### Method 1: Using Homebrew (Optional)

If you have Homebrew, you can ensure you have the latest tools:

```bash
brew install gcc make
```

### Method 2: Direct Build

```bash
# 1. Extract
tar -xzf http-loadbalancer-1.0.0.tar.gz
cd http-loadbalancer-1.0.0

# 2. Configure
./configure

# 3. Build
make

# 4. Install (optional)
sudo make install
```

## Using the Load Balancer

```bash
# Run from build directory
./build/server -c config.example

# Or if installed
http-loadbalancer -c /usr/local/etc/loadbalancer.conf
```

## Notes

- macOS uses Clang by default (works fine with GCC flags)
- pthread is included in the system
- No additional dependencies needed
- Works identically to Linux

## Troubleshooting

### "configure: permission denied"

Make the script executable:
```bash
chmod +x configure
```

### "pthread not found"

This shouldn't happen on macOS. If it does:
```bash
xcode-select --install
```

### Port already in use

Find and kill the process:
```bash
lsof -i :8080
kill -9 <PID>
```

## Building Universal Binary (Optional)

For distribution, you can build a universal binary:

```bash
# Build for both Intel and Apple Silicon
gcc -arch x86_64 -arch arm64 src/*.c -o build/server-universal -lpthread
```

