# Release Process

This document describes how to create and publish a release of the HTTP Load Balancer.

## Creating a Release

### Step 1: Update Version

Update the version number in:
- `VERSION` file
- `configure` script (VERSION variable)
- `configure.ac` (if using autotools)

### Step 2: Create Distribution Packages

```bash
./make-dist.sh
```

This creates both:
- `http-loadbalancer-1.0.0.tar.gz` (Linux/Mac/Unix)
- `http-loadbalancer-1.0.0.zip` (Windows/Cross-platform)

Or manually:
```bash
make dist  # Creates .tar.gz only
```

### Step 3: Test the Distribution

**Linux/Mac:**
```bash
cd /tmp
tar -xzf ~/Desktop/http-loadbalancer-1.0.0.tar.gz
cd http-loadbalancer-1.0.0
./configure
make
sudo make install
```

**Windows:**
```bash
# Extract the .zip file
unzip http-loadbalancer-1.0.0.zip
cd http-loadbalancer-1.0.0
# See BUILD-WINDOWS.md for build instructions
```

### Step 4: Create GitHub Release

1. Go to GitHub repository
2. Click "Releases" → "Draft a new release"
3. Tag: `v1.0.0`
4. Title: `HTTP Load Balancer v1.0.0`
5. Description:
   ```markdown
   ## Release v1.0.0
   
   ### Installation
   
   ```bash
   tar -xzf http-loadbalancer-1.0.0.tar.gz
   cd http-loadbalancer-1.0.0
   ./configure
   make
   sudo make install
   ```
   
   See [INSTALL.md](INSTALL.md) for detailed instructions.
   
   ### Changes
   - Initial release
   - Weighted round-robin load balancing
   - Health checking
   - Reverse proxy support
   - Configurable via config file
   ```
6. Attach both packages:
   - `http-loadbalancer-1.0.0.tar.gz` (Linux/Mac)
   - `http-loadbalancer-1.0.0.zip` (Windows)
7. Publish release

## Distribution Checklist

- [ ] Version number updated
- [ ] CHANGELOG updated (if maintained)
- [ ] README.md is current
- [ ] All tests pass
- [ ] Distribution packages created (.tar.gz and .zip)
- [ ] Distribution tested on Linux/Mac (extract, configure, build, install)
- [ ] Windows build instructions verified (BUILD-WINDOWS.md)
- [ ] GitHub release created
- [ ] Both packages attached to release

## File Structure in Distribution

The distribution tarball should contain:

```
http-loadbalancer-1.0.0/
├── README.md
├── INSTALL.md
├── VERSION
├── configure
├── configure.ac
├── Makefile.in
├── config.example
├── .gitignore
├── include/
│   ├── backend.h
│   ├── config.h
│   ├── http.h
│   ├── logging.h
│   └── server.h
├── src/
│   ├── backend.c
│   ├── config.c
│   ├── http.c
│   ├── logging.c
│   ├── main.c
│   └── server.c
└── testing/
    ├── backends/
    └── *.sh
```

## What NOT to Include

- Build artifacts (`build/`, `*.o`)
- User config files (`config`)
- Log files (`server.log`)
- Git files (`.git/`)
- Generated files (`Makefile`, `config.h`, etc.)

