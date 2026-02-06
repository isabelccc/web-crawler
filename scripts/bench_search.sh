#!/bin/bash
# Benchmark search performance

set -e

OUTPUT_FILE="${1:-./results/search_benchmark.txt}"
NUM_QUERIES="${2:-1000}"
TOPK="${3:-10}"

echo "Running search benchmark..."
echo "Queries: $NUM_QUERIES, TopK: $TOPK"

mkdir -p "$(dirname "$OUTPUT_FILE")"

# Generate test queries
python3 << EOF
queries = [
    "smartphone", "laptop", "shirt", "furniture", "bike",
    "novel", "doll", "cream", "tire", "camera"
] * ($NUM_QUERIES // 10 + 1)
queries = queries[:$NUM_QUERIES]

with open("/tmp/test_queries.txt", "w") as f:
    for q in queries:
        f.write(q + "\n")
EOF

# Run benchmark
START_TIME=$(date +%s.%N)

while IFS= read -r query; do
    curl -s "http://localhost:8080/search?q=$query&topk=$TOPK" > /dev/null
done < /tmp/test_queries.txt

END_TIME=$(date +%s.%N)
ELAPSED=$(echo "$END_TIME - $START_TIME" | bc)

echo "Benchmark Results:" | tee "$OUTPUT_FILE"
echo "Total Queries: $NUM_QUERIES" | tee -a "$OUTPUT_FILE"
echo "Elapsed time: ${ELAPSED}s" | tee -a "$OUTPUT_FILE"
echo "QPS: $(echo "scale=2; $NUM_QUERIES / $ELAPSED" | bc) queries/sec" | tee -a "$OUTPUT_FILE"
echo "P50 latency: <calculated>" | tee -a "$OUTPUT_FILE"
echo "P95 latency: <calculated>" | tee -a "$OUTPUT_FILE"
echo "P99 latency: <calculated>" | tee -a "$OUTPUT_FILE"
