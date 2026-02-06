#!/bin/bash
# Run web crawler locally with docker-compose

set -e

echo "Starting web crawler services..."

# Check if docker-compose is available
if ! command -v docker-compose &> /dev/null; then
    echo "docker-compose not found. Please install docker-compose."
    exit 1
fi

# Start Redis
echo "Starting Redis..."
docker-compose up -d redis

# Wait for Redis to be ready
echo "Waiting for Redis..."
sleep 3

# Build the crawler
echo "Building web crawler..."
make release

# Run the crawler
echo "Starting web crawler..."
./build/release/web-crawler configs/config.yaml
