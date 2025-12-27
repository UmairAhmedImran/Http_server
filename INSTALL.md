# Installation Instructions

This document provides detailed instructions for installing the HTTP Load Balancer.

## Quick Start

```bash
./configure
make
sudo make install
```

## Detailed Installation

### Step 1: Download and Extract

Download the source tarball and extract it:

```bash
tar -xzf http-loadbalancer-1.0.0.tar.gz
cd http-loadbalancer-1.0.0
```

### Step 2: Configure

Run the configure script to prepare the build system:

```bash
./configure
```

**Configuration Options:**

```bash
# Default installation (to /usr/local)
./configure

# Install to /usr
./configure --prefix=/usr

# Custom installation path
./configure --prefix=/opt/loadbalancer

# Custom configuration directory
./configure --prefix=/usr/local --sysconfdir=/etc/loadbalancer
```

**What configure does:**
- Checks for required tools (compiler, pthread library)
- Sets installation directories
- Generates Makefile from template

### Step 3: Build

Compile the source code:

```bash
make
```

**Build Options:**

```bash
# Standard build
make

# Debug build (with debug symbols)
make debug

# Release build (optimized)
make release
```

### Step 4: Install

Install the load balancer system-wide:

```bash
sudo make install
```

This will:
- Install binary to `/usr/local/bin/http-loadbalancer` (or your prefix)
- Install example config to `/usr/local/etc/loadbalancer.conf.example` (or your sysconfdir)
- Create log directory

### Step 5: Configure Backends

```bash
# Copy example config
sudo cp /usr/local/etc/loadbalancer.conf.example /usr/local/etc/loadbalancer.conf

# Edit with your backends
sudo nano /usr/local/etc/loadbalancer.conf
```

Edit the config file with your backend servers:
```
192.168.1.10:8080:5
192.168.1.11:8080:5
192.168.1.12:8080:2
```

### Step 6: Run

```bash
# Run with system config
httplb -c /usr/local/etc/loadbalancer.conf

# Or run on custom port
httplb -p 3000 -c /usr/local/etc/loadbalancer.conf
```

## Installation Directories

By default (with `--prefix=/usr/local`):

- **Binary**: `/usr/local/bin/httplb`
- **Config**: `/usr/local/etc/loadbalancer.conf.example`
- **Logs**: `/usr/local/var/log/` (or current directory)

With `--prefix=/usr`:

- **Binary**: `/usr/bin/httplb`
- **Config**: `/usr/etc/loadbalancer.conf.example`

## Uninstallation

To remove the installed files:

```bash
sudo make uninstall
```

This removes:
- The binary (`httplb`)
- The example config file

**Note**: Your actual config file (`loadbalancer.conf`) is not removed automatically.

## Building from Git Repository

If you cloned from Git:

```bash
git clone <repository-url>
cd Http_server
./configure
make
sudo make install
```

## Troubleshooting

### configure fails: "pthread library not found"

Install pthread development package:
- **Ubuntu/Debian**: `sudo apt-get install libc6-dev`
- **CentOS/RHEL**: `sudo yum install glibc-devel`
- **macOS**: Usually pre-installed

### make fails: "command not found"

Ensure you have:
- GCC compiler: `sudo apt-get install build-essential` (Ubuntu/Debian)
- Make: Usually pre-installed

### Permission denied during make install

Use `sudo`:
```bash
sudo make install
```

### Binary not found after installation

Check your PATH:
```bash
echo $PATH
```

If `/usr/local/bin` is not in PATH, add it or use full path:
```bash
/usr/local/bin/http-loadbalancer
```

## Creating Distribution Tarball

To create a source distribution for others:

```bash
make dist
```

This creates `http-loadbalancer-1.0.0.tar.gz` in the parent directory.

## Systemd Service (Optional)

Create `/etc/systemd/system/http-loadbalancer.service`:

```ini
[Unit]
Description=HTTP Load Balancer
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/http-loadbalancer -c /usr/local/etc/loadbalancer.conf
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

Then:
```bash
sudo systemctl daemon-reload
sudo systemctl enable http-loadbalancer
sudo systemctl start http-loadbalancer
```

