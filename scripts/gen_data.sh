#!/bin/bash
# Generate synthetic SKU data for testing

set -e

OUTPUT_DIR="${1:-./data/generated}"
NUM_SKUS="${2:-1000000}"  # Default 1M SKUs

echo "Generating $NUM_SKUS SKUs to $OUTPUT_DIR..."

mkdir -p "$OUTPUT_DIR"

python3 << EOF
import json
import random
import string
from pathlib import Path

output_dir = Path("$OUTPUT_DIR")
num_skus = int("$NUM_SKUS")

# Categories and brands for realistic distribution
categories = ["Electronics", "Clothing", "Home", "Sports", "Books", "Toys", "Beauty", "Automotive"]
brands = ["BrandA", "BrandB", "BrandC", "BrandD", "BrandE", "BrandF", "BrandG", "BrandH"]

# Zipf-like distribution for terms (simulate long-tail)
def generate_title(category):
    words = {
        "Electronics": ["smartphone", "laptop", "tablet", "headphones", "camera", "speaker"],
        "Clothing": ["shirt", "pants", "jacket", "shoes", "hat", "dress"],
        "Home": ["furniture", "lamp", "cushion", "curtain", "rug", "mirror"],
        "Sports": ["bike", "ball", "racket", "dumbbell", "yoga", "running"],
        "Books": ["novel", "textbook", "guide", "magazine", "comic", "dictionary"],
        "Toys": ["doll", "car", "puzzle", "board", "action", "building"],
        "Beauty": ["cream", "lipstick", "perfume", "shampoo", "brush", "mirror"],
        "Automotive": ["tire", "battery", "oil", "filter", "brake", "light"]
    }
    
    base_words = words.get(category, ["item"])
    title_words = random.sample(base_words, min(3, len(base_words)))
    title_words.extend([random.choice(string.ascii_lowercase) for _ in range(2)])
    return " ".join(title_words).title()

def generate_description(title, category):
    return f"High quality {title.lower()} in {category} category. Perfect for your needs."

# Generate SKUs
output_file = output_dir / "skus.jsonl"
with open(output_file, 'w') as f:
    for i in range(1, num_skus + 1):
        category = random.choice(categories)
        brand = random.choice(brands)
        title = generate_title(category)
        
        sku = {
            "sku_id": f"SKU{i:08d}",
            "title": title,
            "description": generate_description(title, category),
            "category": category,
            "price": round(random.uniform(9.99, 999.99), 2),
            "brand": brand,
            "url": f"https://example.com/products/{i}"
        }
        
        f.write(json.dumps(sku) + "\n")
        
        if (i + 1) % 100000 == 0:
            print(f"Generated {i + 1} SKUs...")

print(f"Generated {num_skus} SKUs to {output_file}")
EOF

echo "Data generation complete!"
