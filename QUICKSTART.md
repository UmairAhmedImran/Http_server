# Quick Start Guide

Get up and running with the HTTP Load Balancer in 5 minutes.

## Download and Install

```bash
# Download and extract
tar -xzf http-loadbalancer-1.0.0.tar.gz
cd http-loadbalancer-1.0.0

# Configure, build, and install
./configure
make
sudo make install
```

## Configure

```bash
# Copy example config
sudo cp /usr/local/etc/loadbalancer.conf.example /usr/local/etc/loadbalancer.conf

# Edit with your backends (host:port:weight format)
sudo nano /usr/local/etc/loadbalancer.conf
```

Example config:
```
192.168.1.10:8080:5
192.168.1.11:8080:5
192.168.1.12:8080:2
```

## Run

```bash
# Start the load balancer (after installation)
httplb -c /usr/local/etc/loadbalancer.conf

# Or from build directory
./build/server -c config
```

## Test

```bash
# In another terminal
curl http://localhost:8080/
```

That's it! Your load balancer is running.

For more details, see [INSTALL.md](INSTALL.md) and [README.md](README.md).

