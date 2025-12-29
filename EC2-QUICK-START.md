# EC2 Quick Start Guide

Get your load balancer running on EC2 in 5 minutes!

## Prerequisites

- EC2 instance running (Ubuntu 22.04 or Amazon Linux 2023)
- SSH access to the instance
- Your 3 backend servers ready (IP addresses and ports)

## Step-by-Step

### 1. Connect to EC2

```bash
ssh -i your-key.pem ubuntu@your-ec2-ip
```

### 2. Download and Install

```bash
# Download distribution
wget https://github.com/yourusername/http-loadbalancer/releases/download/v1.0.0/http-loadbalancer-1.0.0.tar.gz

# Extract and build
tar -xzf http-loadbalancer-1.0.0.tar.gz
cd http-loadbalancer-1.0.0
./configure && make && sudo make install
```

**OR use automated script:**
```bash
git clone https://github.com/yourusername/http-loadbalancer.git
cd http-loadbalancer
chmod +x deploy-ec2.sh
./deploy-ec2.sh
```

### 3. Configure Your Backends

```bash
sudo nano /etc/httplb/loadbalancer.conf
```

Add your 3 backends (one per line):
```
# Option 1: Using IP addresses
192.168.1.10:8080:3
192.168.1.11:8080:2
192.168.1.12:8080:1

# Option 2: Using domain names/URLs (recommended)
api1.example.com:8080:3
api2.example.com:8080:2
api3.example.com:8080:1
```

**Format:** `HOST:PORT:WEIGHT`
- `HOST` can be an IP address (e.g., `192.168.1.10`) or domain name (e.g., `api.example.com`)
- `PORT` is the port number (e.g., `8080`, `443`)
- `WEIGHT` is the traffic weight (1-100)

### 4. Start the Service

```bash
sudo systemctl start httplb
sudo systemctl status httplb
```

### 5. Test

```bash
# From EC2
curl http://localhost:8080/

# From your computer (replace with EC2 public IP)
curl http://YOUR-EC2-IP:8080/
```

## Configuration Examples

### Same EC2 Instance (Testing)
```
127.0.0.1:9001:3
127.0.0.1:9002:2
127.0.0.1:9003:1
```

### Different EC2 Instances
```
# Using IPs
10.0.1.10:8080:3
10.0.1.11:8080:2
10.0.1.12:8080:1

# Or using domain names
api1.example.com:8080:3
api2.example.com:8080:2
api3.example.com:8080:1
```

## Security Group Setup

In AWS Console → EC2 → Security Groups:

**Inbound Rules:**
- SSH (22) - Your IP
- Custom TCP (8080) - 0.0.0.0/0 (or specific IPs)

## Common Commands

```bash
# Start
sudo systemctl start httplb

# Stop
sudo systemctl stop httplb

# Restart (after config change)
sudo systemctl restart httplb

# Status
sudo systemctl status httplb

# Logs
sudo journalctl -u httplb -f

# Edit config
sudo nano /etc/httplb/loadbalancer.conf
```

## Troubleshooting

**Service won't start?**
```bash
sudo journalctl -u httplb -n 50
```

**Backends not responding?**
```bash
# Test connectivity
telnet backend-ip backend-port
```

**Port already in use?**
```bash
# Check what's using port 8080
sudo netstat -tlnp | grep 8080
```

## Next Steps

- See [USER-GUIDE.md](USER-GUIDE.md) for detailed configuration
- See [DEPLOY-EC2.md](DEPLOY-EC2.md) for advanced setup
- Monitor logs: `sudo journalctl -u httplb -f`

