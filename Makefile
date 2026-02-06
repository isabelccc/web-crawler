.PHONY: all debug release clean test bench install docker-build

BUILD_DIR ?= build
CMAKE_FLAGS ?=

all: release

debug:
	@mkdir -p $(BUILD_DIR)/debug
	@cd $(BUILD_DIR)/debug && cmake -DCMAKE_BUILD_TYPE=Debug $(CMAKE_FLAGS) ../..
	@cmake --build $(BUILD_DIR)/debug

release:
	@mkdir -p $(BUILD_DIR)/release
	@cd $(BUILD_DIR)/release && cmake -DCMAKE_BUILD_TYPE=Release $(CMAKE_FLAGS) ../..
	@cmake --build $(BUILD_DIR)/release

clean:
	@rm -rf $(BUILD_DIR)

test: debug
	@cd $(BUILD_DIR)/debug && ctest --output-on-failure

bench: release
	@./scripts/bench_crawl.sh
	@./scripts/bench_search.sh

install: release
	@cd $(BUILD_DIR)/release && cmake --install .

docker-build:
	@docker build -t web-crawler:latest .

sanitizers:
	@mkdir -p $(BUILD_DIR)/sanitizers
	@cd $(BUILD_DIR)/sanitizers && cmake -DCMAKE_BUILD_TYPE=Debug -DUSE_SANITIZERS=ON $(CMAKE_FLAGS) ../..
	@cmake --build $(BUILD_DIR)/sanitizers

format:
	@find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i

check-format:
	@find src -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run --Werror
