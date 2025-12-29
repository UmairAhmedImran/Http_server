#!/bin/bash

# Automated deployment script for EC2
# Usage: ./deploy-ec2.sh

set -e

echo "========================================="
echo "HTTP Load Balancer - EC2 Deployment"
echo "========================================="
echo ""

# Detect OS
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
else
    echo "Error: Cannot detect OS"
    exit 1
fi

echo "Detected OS: $OS"
echo ""

# Install dependencies
echo "Step 1: Installing dependencies..."
if [ "$OS" = "ubuntu" ] || [ "$OS" = "debian" ]; then
    sudo apt-get update
    sudo apt-get install -y build-essential git
elif [ "$OS" = "amzn" ] || [ "$OS" = "rhel" ] || [ "$OS" = "centos" ]; then
    sudo yum update -y
    sudo yum install -y gcc make git
else
    echo "Warning: Unknown OS. Please install build-essential/gcc manually."
fi

echo "Dependencies installed."
echo ""

# Build and install
echo "Step 2: Building load balancer..."
if [ ! -f "configure" ]; then
    echo "Error: configure script not found. Run from project root."
    exit 1
fi

./configure
make
sudo make install

echo "Load balancer installed."
echo ""

# Create config directory
echo "Step 3: Setting up configuration..."
sudo mkdir -p /etc/httplb

if [ ! -f "/etc/httplb/loadbalancer.conf" ]; then
    sudo cp /usr/local/etc/loadbalancer.conf.example /etc/httplb/loadbalancer.conf
    echo "Created default config at /etc/httplb/loadbalancer.conf"
    echo ""
    echo "Please edit /etc/httplb/loadbalancer.conf with your backend servers:"
    echo "  sudo nano /etc/httplb/loadbalancer.conf"
    echo ""
else
    echo "Config file already exists at /etc/httplb/loadbalancer.conf"
fi

# Create systemd service
echo "Step 4: Creating systemd service..."
sudo tee /etc/systemd/system/httplb.service > /dev/null <<EOF
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
EOF

echo "Service file created."
echo ""

# Configure firewall
echo "Step 5: Configuring firewall..."
if command -v ufw >/dev/null 2>&1; then
    sudo ufw allow 8080/tcp
    echo "UFW: Allowed port 8080"
elif command -v firewall-cmd >/dev/null 2>&1; then
    sudo firewall-cmd --permanent --add-port=8080/tcp
    sudo firewall-cmd --reload
    echo "firewalld: Allowed port 8080"
else
    echo "Warning: No firewall detected. Please configure manually."
fi

echo ""

# Reload systemd
echo "Step 6: Enabling service..."
sudo systemctl daemon-reload
sudo systemctl enable httplb

echo ""
echo "========================================="
echo "Deployment Complete!"
echo "========================================="
echo ""
echo "Next steps:"
echo "1. Edit configuration:"
echo "   sudo nano /etc/httplb/loadbalancer.conf"
echo ""
echo "2. Start the service:"
echo "   sudo systemctl start httplb"
echo ""
echo "3. Check status:"
echo "   sudo systemctl status httplb"
echo ""
echo "4. View logs:"
echo "   sudo journalctl -u httplb -f"
echo ""
echo "5. Test:"
echo "   curl http://localhost:8080/"
echo ""

