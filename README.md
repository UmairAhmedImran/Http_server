# HTTP Load Balancer

A high-performance HTTP load balancer written in C with support for weighted round-robin distribution, health checking, and reverse proxy functionality.

## Features

- **Weighted Round-Robin Load Balancing**: Distribute traffic proportionally based on backend weights
- **Health Checking**: Automatic backend health monitoring with automatic failover
- **Reverse Proxy**: Full HTTP reverse proxy with proper header handling
- **Proxy Compatibility**: Supports X-Forwarded-For, X-Real-IP, and Host headers
- **Multi-threaded**: Handles multiple concurrent client connections
- **Configurable**: Easy configuration via config file or command-line arguments
- **Memory Safe**: Comprehensive memory leak prevention and buffer overflow protection

## Requirements

- GCC compiler (or any C99-compatible compiler)
- POSIX-compatible operating system (Linux, macOS, BSD)
- Make
- pthread library (usually included with system)

### Platform Support

- **Linux**: Full support, tested on Ubuntu, Debian, CentOS
- **macOS**: Full support, see [BUILD-MACOS.md](BUILD-MACOS.md)
- **Windows**: Use WSL (recommended) or MinGW, see [BUILD-WINDOWS.md](BUILD-WINDOWS.md)

## Installation

### From Source Distribution

**Linux/macOS:**
```bash
tar -xzf http-loadbalancer-1.0.0.tar.gz
cd http-loadbalancer-1.0.0
```

**Windows:**
```bash
# Extract the .zip file
unzip http-loadbalancer-1.0.0.zip
cd http-loadbalancer-1.0.0
# See BUILD-WINDOWS.md for Windows-specific instructions
```

2. **Configure the build:**
   ```bash
   ./configure
   ```
   
   Options:
   - `--prefix=/usr/local` (default) - Installation prefix
   - `--sysconfdir=/etc` - Configuration directory
   - `--help` - Show all options

3. **Build:**
   ```bash
   make
   ```

4. **Install:**
   ```bash
   sudo make install
   ```

5. **Configure backends:**
   ```bash
   sudo cp /usr/local/etc/loadbalancer.conf.example /usr/local/etc/loadbalancer.conf
   sudo nano /usr/local/etc/loadbalancer.conf
   ```

6. **Run:**
   ```bash
   httplb -c /usr/local/etc/loadbalancer.conf
   ```

### From Git Repository

```bash
git clone <your-repo-url>
cd Http_server
./configure
make
sudo make install
```

See [INSTALL.md](INSTALL.md) for detailed installation instructions.

**Platform-specific guides:**
- [BUILD-MACOS.md](BUILD-MACOS.md) - macOS build instructions
- [BUILD-WINDOWS.md](BUILD-WINDOWS.md) - Windows build instructions

### Build Options

```bash
make          # Standard build
make debug    # Build with debug symbols
make release  # Optimized release build
make clean    # Clean build artifacts
```

## Configuration

### Configuration File

Create a `config` file in the project root directory with your backend servers:

```
# Format: host:port:weight
# Lines starting with # are comments
# Empty lines are ignored

127.0.0.1:9001:3
127.0.0.1:9002:2
127.0.0.1:9003:1
```

**Format**: `host:port:weight`
- **host**: Backend server IP address or hostname
- **port**: Backend server port number
- **weight**: Traffic weight (1-100). Higher weight = more requests

**Example**:
```
# Production backends
192.168.1.10:8080:5
192.168.1.11:8080:5
192.168.1.12:8080:2

# Staging backend (lower weight)
192.168.1.20:8080:1
```

A sample configuration file (`config.example`) is provided. Copy it to `config` and modify as needed:

```bash
cp config.example config
```

### Command-Line Options

After installation:
```bash
httplb [OPTIONS]
```

Or if running from build directory:
```bash
./build/server [OPTIONS]
```

**Options**:
- `-p, --port PORT`: Server port (default: 8080)
- `-c, --config FILE`: Configuration file path (default: config)
- `-h, --help`: Show help message

**Examples**:
```bash
# Run on default port 8080 with system config (after installation)
httplb -c /usr/local/etc/loadbalancer.conf

# Run on custom port
httplb -p 3000 -c /usr/local/etc/loadbalancer.conf

# Use custom config file
httplb -c /path/to/myconfig.conf

# Run from build directory (development)
./build/server -p 3000 -c config
```

## Usage

### 1. Start Backend Servers

First, start your backend servers. Example using Python:

```bash
# Terminal 1
python3 testing/backends/backend_9001.py

# Terminal 2
python3 testing/backends/backend_9002.py

# Terminal 3
python3 testing/backends/backend_9003.py
```

### 2. Configure Backends

Create or edit the `config` file with your backend servers:

```
127.0.0.1:9001:3
127.0.0.1:9002:2
127.0.0.1:9003:1
```

### 3. Start Load Balancer

```bash
./build/server
```

The load balancer will:
- Load backend configuration from `config` file
- Start listening on port 8080 (or specified port)
- Begin health checking backends every 10 seconds
- Accept client connections and distribute requests

### 4. Test the Load Balancer

```bash
# Make requests to the load balancer
curl http://localhost:8080/

# Or use wget
wget http://localhost:8080/ -O -
```

## How It Works

### Load Balancing Algorithm

The load balancer uses a **Weighted Round-Robin** algorithm:

1. Each backend is assigned a weight (1-100)
2. Requests are distributed proportionally based on weights
3. Example: With weights 3:2:1, backend 1 gets 50%, backend 2 gets 33%, backend 3 gets 17%

### Health Checking

- Health checks run every 10 seconds in a background thread
- Each backend is tested via TCP connection
- Unhealthy backends are automatically removed from rotation
- Recovered backends are automatically re-added

### Request Flow

1. Client connects to load balancer
2. Load balancer selects backend using weighted round-robin
3. Request is modified with proxy headers (X-Forwarded-For, Host, etc.)
4. Request is forwarded to selected backend
5. Backend response is forwarded back to client
6. If backend fails, load balancer tries next available backend

## Logging

Logs are written to:
- **Console**: Real-time log output
- **File**: `server.log` (appended, not overwritten)

Log levels:
- `INFO`: General information
- `WARNING`: Non-critical issues
- `ERROR`: Errors that need attention
- `DEBUG`: Detailed debugging information

## Testing

### Test Scripts

The `testing/` directory contains test scripts:

```bash
# Test weighted load distribution
./testing/test_weighted_load.sh

# Test proxy headers
./testing/test_proxy_headers.sh
```

### Manual Testing

1. Start multiple backend servers on different ports
2. Configure them in `config` file
3. Start load balancer
4. Make multiple requests and observe distribution in logs

## Architecture

```
┌─────────────┐
│   Client    │
└──────┬──────┘
       │
       ▼
┌─────────────────┐
│  Load Balancer  │◄─── Health Check Thread
│   (Port 8080)   │
└──────┬──────────┘
       │
       ├──► Backend 1 (Weight: 3)
       ├──► Backend 2 (Weight: 2)
       └──► Backend 3 (Weight: 1)
```

## Performance Considerations

- **Concurrency**: Each client connection is handled in a separate thread
- **Memory**: Response buffers are dynamically allocated and freed
- **Timeouts**: Backend connections timeout after 5 seconds
- **Health Checks**: Non-blocking, run in background thread

## Troubleshooting

### Port Already in Use

```bash
# Find process using port 8080
lsof -i :8080

# Kill the process or use different port
./build/server -p 3000
```

### Backends Not Responding

- Check that backend servers are running
- Verify backend addresses in config file
- Check firewall rules
- Review logs in `server.log`

### Configuration Not Loading

- Ensure `config` file exists in project root
- Check file format (host:port:weight)
- Verify file permissions
- Check logs for parsing errors

## Contributing

Contributions are welcome! Please ensure:
- Code follows existing style
- Memory safety is maintained
- All allocations are properly freed
- Error handling is comprehensive

## License

[Specify your license here]

## Author

[Your name/contact information]

## Acknowledgments

- GNU C Library Manual for socket programming reference
- University course materials for foundational knowledge
