#!/bin/bash

# Test script to verify weighted load balancing
# Backend weights: 9001=3, 9002=2, 9003=1
# Expected distribution: ~50% to 9001, ~33% to 9002, ~17% to 9003

NUM_REQUESTS=${1:-30}

echo "=========================================="
echo "Testing Weighted Load Balancing"
echo "Sending $NUM_REQUESTS requests..."
echo "Expected distribution:"
echo "  Backend 9001 (weight 3): ~50% of requests"
echo "  Backend 9002 (weight 2): ~33% of requests"
echo "  Backend 9003 (weight 1): ~17% of requests"
echo "=========================================="
echo ""

# Send requests
for i in $(seq 1 $NUM_REQUESTS); do
    curl -s http://localhost:8080/ > /dev/null &
done

# Wait for all requests to complete
wait

echo "Request distribution from server logs:"
echo "---"

# Count backend selections from logs
grep -E "Selected backend.*for request" server.log | tail -n $NUM_REQUESTS | \
    grep -oE "127\.0\.0\.1:900[123]" | sort | uniq -c | \
    awk '{printf "  Backend %s: %d requests (%.1f%%)\n", $2, $1, ($1/'$NUM_REQUESTS')*100}'

echo ""
echo "=========================================="
echo "Test completed!"
echo "=========================================="

