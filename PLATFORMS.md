# Platform Support

This document describes platform support and distribution formats.

## Supported Platforms

### Linux ✅
- **Format**: `.tar.gz`
- **Status**: Fully supported
- **Tested on**: Ubuntu, Debian, CentOS, Fedora
- **Build**: Standard `./configure && make`
- **Install**: `sudo make install`

### macOS ✅
- **Format**: `.tar.gz`
- **Status**: Fully supported
- **Requirements**: Xcode Command Line Tools
- **Build**: Standard `./configure && make`
- **Install**: `sudo make install`
- **See**: [BUILD-MACOS.md](BUILD-MACOS.md)

### Windows ⚠️
- **Format**: `.zip`
- **Status**: Supported via WSL or MinGW
- **Recommended**: Use WSL (Windows Subsystem for Linux)
- **Native**: Requires MinGW-w64 (see BUILD-WINDOWS.md)
- **See**: [BUILD-WINDOWS.md](BUILD-WINDOWS.md)

## Distribution Formats

### .tar.gz (Linux/macOS/Unix)
- Standard Unix format
- Preserves file permissions
- Works on all POSIX systems
- Extract with: `tar -xzf file.tar.gz`

### .zip (Windows/Cross-platform)
- Universal format
- Works on all platforms
- Windows-friendly
- Extract with: `unzip file.zip` or Windows Explorer

## Download Links

When creating a GitHub release, provide both formats:

```
Downloads:
- http-loadbalancer-1.0.0.tar.gz (Linux/macOS)
- http-loadbalancer-1.0.0.zip (Windows/Cross-platform)
```

## Platform-Specific Notes

### Linux
- No special requirements
- Works out of the box
- pthread included in glibc

### macOS
- Uses Clang (GCC-compatible)
- pthread included
- May need Xcode Command Line Tools

### Windows
- **WSL**: Best experience, identical to Linux
- **MinGW**: Native Windows, requires manual build
- **MSVC**: Not supported (requires code changes)

## Building for Multiple Platforms

### From Linux/Mac

```bash
# Create both distributions
./make-dist.sh

# This creates:
# - http-loadbalancer-1.0.0.tar.gz
# - http-loadbalancer-1.0.0.zip
```

### Testing Distributions

**Linux/Mac:**
```bash
tar -xzf http-loadbalancer-1.0.0.tar.gz
cd http-loadbalancer-1.0.0
./configure && make
```

**Windows:**
```bash
unzip http-loadbalancer-1.0.0.zip
cd http-loadbalancer-1.0.0
# Follow BUILD-WINDOWS.md
```

## Future Enhancements

Potential future additions:
- Pre-built Windows binaries (.exe)
- macOS universal binaries
- Docker images
- Package managers (Homebrew, apt, yum)

