#!/bin/bash

# Test script to send multiple requests and show backend distribution
# Usage: ./test_load.sh [number_of_requests]

NUM_REQUESTS=${1:-10}

echo "Sending $NUM_REQUESTS requests to http://localhost:8080/"
echo "=========================================="

# Send requests in parallel (or sequential if you prefer)
for i in $(seq 1 $NUM_REQUESTS); do
    curl -s "http://localhost:8080/" > /dev/null &
done

# Wait for all background jobs to complete
wait

echo "All requests sent!"
echo ""
echo "Checking server.log for backend distribution..."
echo "=========================================="

# Extract backend info from JSON logs (if using JSON format)
if grep -q '"type": "request"' server.log 2>/dev/null; then
    echo "Backend distribution (from JSON logs):"
    grep '"type": "request"' server.log | tail -n $NUM_REQUESTS | \
        grep -o '"backend": "[^"]*"' | \
        sed 's/"backend": "//' | sed 's/"//' | \
        awk '{print NR": "$0}'
else
    # Fallback: look for "Selected backend" or "Trying backend" messages
    echo "Backend distribution (from log messages):"
    grep -E "(Selected backend|Trying backend|backend.*for request)" server.log | tail -n $NUM_REQUESTS | \
        grep -oE "127\.0\.0\.1:[0-9]+" | \
        awk '{print NR": "$0}'
fi

echo ""
echo "Summary:"
grep -E "127\.0\.0\.1:(9001|9002|9003)" server.log | tail -n $NUM_REQUESTS | \
    grep -oE "127\.0\.0\.1:900[123]" | sort | uniq -c | \
    awk '{print "  Backend "$2": "$1" requests"}'

