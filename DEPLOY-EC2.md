# Deploying Load Balancer on EC2

This guide walks you through deploying the HTTP Load Balancer on AWS EC2 so users can configure and use it with their own backend servers.

## Prerequisites

- AWS account with EC2 access
- SSH key pair for EC2
- Basic knowledge of Linux commands

## Step 1: Launch EC2 Instance

### 1.1 Create EC2 Instance

1. **Go to EC2 Console** → Launch Instance
2. **Choose AMI**: Ubuntu 22.04 LTS or Amazon Linux 2023
3. **Instance Type**: `t2.micro` (free tier) or `t3.small` for production
4. **Key Pair**: Select or create a new key pair
5. **Network Settings**: 
   - Create/select security group
   - Allow SSH (port 22) from your IP
   - Allow HTTP (port 80) and custom load balancer port (8080) from anywhere (0.0.0.0/0)
6. **Storage**: 8GB minimum
7. **Launch Instance**

### 1.2 Configure Security Group

**Inbound Rules:**
- SSH (22) - Your IP only
- HTTP (80) - 0.0.0.0/0 (if using port 80)
- Custom TCP (8080) - 0.0.0.0/0 (load balancer port)
- Custom TCP (9001-9003) - 0.0.0.0/0 (backend ports, if backends are on same instance)

**Outbound Rules:**
- All traffic - 0.0.0.0/0

## Step 2: Connect to EC2 Instance

```bash
# Replace with your key and instance details
ssh -i your-key.pem ubuntu@your-ec2-public-ip

# Or for Amazon Linux
ssh -i your-key.pem ec2-user@your-ec2-public-ip
```

## Step 3: Install Dependencies

### For Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y build-essential git
```

### For Amazon Linux:
```bash
sudo yum update -y
sudo yum install -y gcc make git
```

## Step 4: Install Load Balancer

### Option A: From Source Distribution

```bash
# Download the distribution
wget https://github.com/yourusername/http-loadbalancer/releases/download/v1.0.0/http-loadbalancer-1.0.0.tar.gz

# Extract
tar -xzf http-loadbalancer-1.0.0.tar.gz
cd http-loadbalancer-1.0.0

# Build and install
./configure
make
sudo make install
```

### Option B: From Git Repository

```bash
git clone https://github.com/yourusername/http-loadbalancer.git
cd http-loadbalancer
./configure
make
sudo make install
```

## Step 5: Configure Load Balancer

### 5.1 Create Configuration File

```bash
# Create config directory
sudo mkdir -p /etc/httplb

# Copy example config
sudo cp /usr/local/etc/loadbalancer.conf.example /etc/httplb/loadbalancer.conf

# Edit configuration
sudo nano /etc/httplb/loadbalancer.conf
```

### 5.2 Configure Your Backends

Edit `/etc/httplb/loadbalancer.conf` with your 3 backend servers:

```
# Format: host:port:weight
# The 'host' can be an IP address OR a domain name/URL

# Example 1: Using IP addresses
192.168.1.10:8080:3
192.168.1.11:8080:2
192.168.1.12:8080:1

# Example 2: Using domain names/URLs (RECOMMENDED for production)
api1.example.com:8080:3
api2.example.com:8080:2
api3.example.com:8080:1
```

**Configuration Options:**

1. **Same EC2 Instance** (for testing):
   ```
   127.0.0.1:9001:3
   127.0.0.1:9002:2
   127.0.0.1:9003:1
   ```

2. **Different EC2 Instances** (IP addresses):
   ```
   10.0.1.10:8080:3
   10.0.1.11:8080:2
   10.0.1.12:8080:1
   ```

3. **Using Domain Names/URLs** (recommended):
   ```
   api1.example.com:8080:3
   api2.example.com:8080:2
   api3.example.com:8080:1
   ```

4. **Production URLs with HTTPS**:
   ```
   api.production.com:443:3
   api2.production.com:443:2
   api3.production.com:443:1
   ```

5. **Mixed (IPs + Domains)**:
   ```
   127.0.0.1:9001:3
   api.example.com:8080:2
   10.0.1.21:8080:1
   ```

**Note:** The load balancer supports both IP addresses and domain names. Domain names are resolved via DNS, so ensure your EC2 instance can resolve the domain names correctly.

## Step 6: Set Up as System Service

### 6.1 Create Systemd Service File

```bash
sudo nano /etc/systemd/system/httplb.service
```

Add the following:

```ini
[Unit]
Description=HTTP Load Balancer
After=network.target

[Service]
Type=simple
User=root
ExecStart=/usr/local/bin/httplb -p 8080 -c /etc/httplb/loadbalancer.conf
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

### 6.2 Enable and Start Service

```bash
# Reload systemd
sudo systemctl daemon-reload

# Enable service (start on boot)
sudo systemctl enable httplb

# Start service
sudo systemctl start httplb

# Check status
sudo systemctl status httplb

# View logs
sudo journalctl -u httplb -f
```

## Step 7: Configure Backend Servers

### 7.1 If Backends are on Same EC2 Instance

You can run simple HTTP servers for testing:

```bash
# Terminal 1 - Backend 1
python3 -m http.server 9001

# Terminal 2 - Backend 2
python3 -m http.server 9002

# Terminal 3 - Backend 3
python3 -m http.server 9003
```

Or use the provided test backends:
```bash
cd /path/to/http-loadbalancer/testing/backends
python3 backend_9001.py &
python3 backend_9002.py &
python3 backend_9003.py &
```

### 7.2 If Backends are on Different Instances

1. **Deploy your application** on each backend instance
2. **Ensure backends are accessible** from load balancer instance:
   - Same VPC: Use private IPs
   - Different VPC: Use public IPs or VPN
   - Configure security groups to allow traffic from load balancer

3. **Test connectivity** from load balancer:
   ```bash
   curl http://backend-ip:port/
   ```

## Step 8: Test the Load Balancer

### 8.1 From EC2 Instance

```bash
# Test locally
curl http://localhost:8080/

# Check logs
sudo journalctl -u httplb -n 50
```

### 8.2 From Your Local Machine

```bash
# Replace with your EC2 public IP
curl http://your-ec2-public-ip:8080/

# Or use browser
open http://your-ec2-public-ip:8080/
```

### 8.3 Verify Load Distribution

```bash
# Make multiple requests and check which backend responds
for i in {1..10}; do
  curl http://your-ec2-public-ip:8080/
  sleep 1
done
```

## Step 9: Firewall Configuration

### Ubuntu/Debian (UFW):

```bash
sudo ufw allow 8080/tcp
sudo ufw allow 22/tcp
sudo ufw enable
```

### Amazon Linux (firewalld):

```bash
sudo firewall-cmd --permanent --add-port=8080/tcp
sudo firewall-cmd --reload
```

## Step 10: Update Configuration (For Users)

When users need to change backends:

```bash
# Edit config
sudo nano /etc/httplb/loadbalancer.conf

# Restart service to apply changes
sudo systemctl restart httplb

# Verify
sudo systemctl status httplb
```

## Troubleshooting

### Check if Load Balancer is Running

```bash
sudo systemctl status httplb
ps aux | grep httplb
netstat -tlnp | grep 8080
```

### Check Logs

```bash
# Systemd logs
sudo journalctl -u httplb -f

# Application logs
tail -f /var/log/httplb/server.log
```

### Test Backend Connectivity

```bash
# From load balancer instance
telnet backend-ip backend-port
# Or
nc -zv backend-ip backend-port
```

### Common Issues

1. **Port already in use**: Change port with `-p` option
2. **Backend unreachable**: Check security groups and network ACLs
3. **Permission denied**: Ensure config file is readable
4. **Service won't start**: Check logs with `journalctl -u httplb`

## Security Considerations

1. **Use HTTPS**: Consider adding SSL/TLS termination (nginx reverse proxy)
2. **Restrict Access**: Limit security group to specific IPs if possible
3. **Regular Updates**: Keep system and load balancer updated
4. **Monitor Logs**: Set up CloudWatch or similar monitoring
5. **Backup Config**: Keep backups of configuration files

## Next Steps

- Set up monitoring and alerting
- Configure auto-scaling for backends
- Add SSL/TLS termination
- Set up log rotation
- Configure health check notifications

