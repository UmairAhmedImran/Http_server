# How to Run the Load Balancer

After extracting the distribution, follow these steps:

## Step 1: Configure and Build

```bash
cd http-loadbalancer-1.0.0
./configure
make
```

This will create the executable at `build/server`.

## Step 2: Set Up Backend Servers

Before running the load balancer, you need backend servers to balance. 

### Option A: Use the Test Backends (Quick Test)

In separate terminals, start the test backends:

```bash
# Terminal 1
python3 testing/backends/backend_9001.py

# Terminal 2  
python3 testing/backends/backend_9002.py

# Terminal 3
python3 testing/backends/backend_9003.py
```

### Option B: Use Your Own Backends

Edit the config file with your backend servers:

```bash
cp config.example config
nano config  # or use your preferred editor
```

Format: `host:port:weight`
```
192.168.1.10:8080:5
192.168.1.11:8080:5
192.168.1.12:8080:2
```

## Step 3: Run the Load Balancer

### From Build Directory (Development)

```bash
./build/server -c config
```

Or specify a port:

```bash
./build/server -p 8080 -c config
```

### After Installation (Production)

If you installed with `sudo make install`:

```bash
# Copy and edit config
sudo cp /usr/local/etc/loadbalancer.conf.example /usr/local/etc/loadbalancer.conf
sudo nano /usr/local/etc/loadbalancer.conf

# Run
http-loadbalancer -c /usr/local/etc/loadbalancer.conf
```

## Step 4: Test It

In another terminal:

```bash
# Make requests
curl http://localhost:8080/

# Or multiple requests to see load distribution
for i in {1..10}; do curl http://localhost:8080/; done
```

## Command-Line Options

```bash
./build/server [OPTIONS]

Options:
  -p, --port PORT     Server port (default: 8080)
  -c, --config FILE   Configuration file (default: config)
  -h, --help          Show help message
```

## Quick Example

```bash
# 1. Extract and build
tar -xzf http-loadbalancer-1.0.0.tar.gz
cd http-loadbalancer-1.0.0
./configure && make

# 2. Start backends (in separate terminals)
python3 testing/backends/backend_9001.py &
python3 testing/backends/backend_9002.py &
python3 testing/backends/backend_9003.py &

# 3. Run load balancer
./build/server -c config.example

# 4. Test (in another terminal)
curl http://localhost:8080/
```

## Stopping the Load Balancer

Press `Ctrl+C` in the terminal where it's running.

## Troubleshooting

**Port already in use:**
```bash
# Find what's using port 8080
lsof -i :8080

# Use a different port
./build/server -p 3000 -c config
```

**No backends responding:**
- Check that backend servers are running
- Verify backend addresses in config file
- Check firewall rules

**Build errors:**
- Ensure you ran `./configure` first
- Check that you have GCC and pthread library installed

