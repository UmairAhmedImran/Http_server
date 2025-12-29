# User Guide: Using the Load Balancer

This guide is for users who want to deploy and configure the load balancer with their own backend servers.

## Quick Start

### 1. Deploy on EC2

Follow the [DEPLOY-EC2.md](DEPLOY-EC2.md) guide or use the automated script:

```bash
# On your EC2 instance
git clone https://github.com/yourusername/http-loadbalancer.git
cd http-loadbalancer
chmod +x deploy-ec2.sh
./deploy-ec2.sh
```

### 2. Configure Your Backends

Edit the configuration file:

```bash
sudo nano /etc/httplb/loadbalancer.conf
```

Add your 3 backend servers in the format `host:port:weight`:

```
# Example 1: Using IP addresses
192.168.1.10:8080:3
192.168.1.11:8080:2
192.168.1.12:8080:1

# Example 2: Using domain names/URLs (recommended for production)
api1.example.com:8080:3
api2.example.com:8080:2
api3.example.com:8080:1

# Example 3: Using subdomains
backend1.myservice.com:80:3
backend2.myservice.com:80:2
backend3.myservice.com:80:1
```

**Configuration Format:**
- `host`: IP address, hostname, or domain name of backend server
- `port`: Port number where backend is running
- `weight`: Traffic weight (1-100). Higher = more requests

**Note:** The load balancer supports both IP addresses and domain names. Using domain names is recommended for production as they're easier to manage and can point to different servers if needed.

**Weight Distribution:**
- Weight 3:2:1 = 50% : 33% : 17% of traffic
- Weight 5:3:2 = 50% : 30% : 20% of traffic
- Weight 1:1:1 = 33% : 33% : 33% of traffic (equal)

### 3. Start the Load Balancer

```bash
# Start service
sudo systemctl start httplb

# Check status
sudo systemctl status httplb

# View logs
sudo journalctl -u httplb -f
```

### 4. Test

```bash
# From EC2 instance
curl http://localhost:8080/

# From your local machine (replace with EC2 public IP)
curl http://your-ec2-ip:8080/
```

## Configuration Examples

### Example 1: Backends on Same EC2 Instance

If you're running all backends on the same instance:

```
127.0.0.1:9001:3
127.0.0.1:9002:2
127.0.0.1:9003:1
```

Start your backends:
```bash
# Terminal 1
python3 -m http.server 9001

# Terminal 2
python3 -m http.server 9002

# Terminal 3
python3 -m http.server 9003
```

### Example 2: Backends on Different EC2 Instances

If your backends are on separate EC2 instances:

```
# Option A: Use private IPs (same VPC)
10.0.1.10:8080:3
10.0.1.11:8080:2
10.0.1.12:8080:1

# Option B: Use public IPs
54.123.45.67:8080:3
54.123.45.68:8080:2
54.123.45.69:8080:1

# Option C: Use domain names (recommended)
api1.example.com:8080:3
api2.example.com:8080:2
api3.example.com:8080:1
```

**Important:** Ensure security groups allow traffic from load balancer to backends.

### Example 3: Mixed Configuration

Some backends local, some remote:

```
127.0.0.1:9001:3
10.0.1.20:8080:2
10.0.1.21:8080:1
```

### Example 4: Different Ports

Backends running on different ports:

```
192.168.1.10:3000:3
192.168.1.11:4000:2
192.168.1.12:5000:1
```

### Example 5: Real Production URLs

Using actual deployed backend URLs:

```
# Production backends with domain names
api.production.com:443:3
api2.production.com:443:2
api3.production.com:443:1

# Or with different ports
backend1.myservice.com:8080:3
backend2.myservice.com:8080:2
backend3.myservice.com:8080:1

# Mixed: Some with domains, some with IPs
api.example.com:80:3
192.168.1.10:8080:2
api3.example.com:443:1
```

## Updating Configuration

When you need to change backends:

```bash
# 1. Edit config
sudo nano /etc/httplb/loadbalancer.conf

# 2. Restart service
sudo systemctl restart httplb

# 3. Verify
sudo systemctl status httplb
curl http://localhost:8080/
```

## Running on Custom Port

To run on a different port (e.g., 80 for HTTP):

1. **Edit service file:**
   ```bash
   sudo nano /etc/systemd/system/httplb.service
   ```
   Change `-p 8080` to `-p 80`

2. **Update firewall:**
   ```bash
   sudo ufw allow 80/tcp
   ```

3. **Restart:**
   ```bash
   sudo systemctl daemon-reload
   sudo systemctl restart httplb
   ```

## Monitoring

### Check Service Status

```bash
sudo systemctl status httplb
```

### View Logs

```bash
# Real-time logs
sudo journalctl -u httplb -f

# Last 100 lines
sudo journalctl -u httplb -n 100

# Logs since today
sudo journalctl -u httplb --since today
```

### Check Load Distribution

```bash
# Make multiple requests and observe backend responses
for i in {1..20}; do
  curl http://localhost:8080/
  sleep 0.5
done
```

## Troubleshooting

### Load Balancer Won't Start

```bash
# Check logs
sudo journalctl -u httplb -n 50

# Check if port is in use
sudo netstat -tlnp | grep 8080

# Check config file syntax
cat /etc/httplb/loadbalancer.conf
```

### Backends Not Responding

```bash
# Test connectivity from load balancer
telnet backend-ip backend-port

# Or
nc -zv backend-ip backend-port

# Check backend is running
curl http://backend-ip:backend-port/
```

### All Requests Going to One Backend

- Check weights in config file
- Verify all backends are active (check logs)
- Ensure health checks are passing

### Permission Errors

```bash
# Ensure config file is readable
sudo chmod 644 /etc/httplb/loadbalancer.conf

# Check service user
sudo systemctl show httplb | grep User
```

## Best Practices

1. **Use Private IPs**: If backends are in same VPC, use private IPs
2. **Equal Weights for Testing**: Start with 1:1:1 for equal distribution
3. **Monitor Health**: Check logs regularly for backend failures
4. **Backup Config**: Keep backups of your configuration
5. **Test Changes**: Test configuration changes in staging first

## Security

1. **Firewall Rules**: Only allow necessary ports
2. **Security Groups**: Restrict access to backends
3. **Regular Updates**: Keep system and load balancer updated
4. **Monitor Logs**: Watch for suspicious activity

## Advanced Usage

### Running Multiple Instances

You can run multiple load balancer instances for high availability:

```bash
# Instance 1: Port 8080
httplb -p 8080 -c /etc/httplb/config1.conf

# Instance 2: Port 8081
httplb -p 8081 -c /etc/httplb/config2.conf
```

### Custom Log Location

Edit service file to redirect logs:

```ini
[Service]
StandardOutput=append:/var/log/httplb/output.log
StandardError=append:/var/log/httplb/error.log
```

## Support

For issues or questions:
- Check [README.md](README.md)
- Review [DEPLOY-EC2.md](DEPLOY-EC2.md)
- Check GitHub Issues
- Review logs: `sudo journalctl -u httplb`

