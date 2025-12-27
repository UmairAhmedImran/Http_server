#!/bin/bash

# Test script to verify proxy headers (X-Forwarded-For, X-Real-IP, Host) are being sent

echo "=========================================="
echo "Testing Proxy Headers Implementation"
echo "=========================================="
echo ""

echo "Test 1: Request without X-Forwarded-For header"
echo "Expected: X-Forwarded-For should be added with client IP"
echo "---"
curl -s http://localhost:8080/ | python3 -m json.tool | grep -A 5 "headers_received"
echo ""
echo ""

echo "Test 2: Request with existing X-Forwarded-For header"
echo "Expected: X-Forwarded-For should append client IP to existing chain"
echo "---"
curl -s -H "X-Forwarded-For: 192.168.1.100" http://localhost:8080/ | python3 -m json.tool | grep -A 5 "headers_received"
echo ""
echo ""

echo "Test 3: Verify Host header is modified"
echo "Expected: Host should be changed to backend address (127.0.0.1:9001)"
echo "---"
curl -s -H "Host: example.com" http://localhost:8080/ | python3 -m json.tool | grep -A 5 "headers_received"
echo ""
echo ""

echo "Test 4: Verify X-Real-IP header is added"
echo "Expected: X-Real-IP should contain client IP"
echo "---"
curl -s http://localhost:8080/ | python3 -m json.tool | grep -A 5 "headers_received" | grep "X-Real-IP"
echo ""
echo ""

echo "=========================================="
echo "All tests completed!"
echo "=========================================="

