#!/bin/bash
# Benchmark crawling performance

set -e

OUTPUT_FILE="${1:-./results/crawl_benchmark.txt}"
NUM_URLS="${2:-10000}"
CONCURRENCY="${3:-8}"

echo "Running crawl benchmark..."
echo "URLs: $NUM_URLS, Concurrency: $CONCURRENCY"

mkdir -p "$(dirname "$OUTPUT_FILE")"

# Generate test URLs
python3 << EOF
import json
urls = [f"https://example.com/page/{i}" for i in range($NUM_URLS)]
with open("/tmp/test_urls.txt", "w") as f:
    for url in urls:
        f.write(url + "\n")
EOF

# Run benchmark
START_TIME=$(date +%s.%N)

./build/release/web-crawler configs/config.yaml < /tmp/test_urls.txt 2>&1 | tee "$OUTPUT_FILE"

END_TIME=$(date +%s.%N)
ELAPSED=$(echo "$END_TIME - $START_TIME" | bc)

echo "Benchmark Results:" | tee -a "$OUTPUT_FILE"
echo "Total URLs: $NUM_URLS" | tee -a "$OUTPUT_FILE"
echo "Elapsed time: ${ELAPSED}s" | tee -a "$OUTPUT_FILE"
echo "Throughput: $(echo "scale=2; $NUM_URLS / $ELAPSED" | bc) URLs/sec" | tee -a "$OUTPUT_FILE"
