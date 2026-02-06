# Multi-stage build for web-crawler

# Build stage
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    pkg-config \
    libcurl4-openssl-dev \
    libhiredis-dev \
    libgumbo-dev \
    libxxhash-dev \
    libssl-dev \
    libyaml-cpp-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

COPY . .

RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -GNinja .. && \
    ninja

# Runtime stage
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libcurl4 \
    libhiredis1.0 \
    libgumbo1 \
    libxxhash0 \
    libssl3 \
    libyaml-cpp0.8 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /build/build/web-crawler .
COPY --from=builder /build/configs ./configs

EXPOSE 8080 9090

CMD ["./web-crawler", "configs/config.yaml"]
