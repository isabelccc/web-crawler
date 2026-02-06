#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "utils/config.h"
#include "scheduler/scheduler.h"
#include "fetcher/fetcher.h"
#include "parser/parser.h"
#include "dedup/dedup.h"
#include "indexer/indexer.h"
#include "storage/storage.h"
#include "api/api_server.h"
#include "observability/logger.h"
#include "observability/metrics.h"
#include <nlohmann/json.hpp>

using namespace crawler;

int main(int argc, char* argv[]) {
    std::string config_path = "configs/config.yaml";
    if (argc > 1) {
        config_path = argv[1];
    }
    
    // Load configuration
    auto& config = Config::instance();
    if (!config.load(config_path)) {
        std::cerr << "Failed to load config from " << config_path << std::endl;
        return 1;
    }
    
    // Initialize logger
    Logger::instance().init("info", "json", "stdout");
    Logger::instance().info("Starting web crawler service");
    
    // Initialize components
    Scheduler scheduler;
    Fetcher fetcher;
    Parser parser;
    Deduplicator dedup;
    Storage storage("./data");
    Indexer indexer("./data/index");
    
    // Initialize Redis
    if (!dedup.init_redis(config.redis_host(), config.redis_port())) {
        Logger::instance().warn("Redis connection failed, using local fallback");
        dedup.enable_local_fallback(true);
    }
    
    // Initialize API server
    ApiServer api_server;
    api_server.init(config.api_host(), config.api_port(), config.api_threads());
    
    // Set up search handler
    api_server.set_search_handler([&indexer](const std::string& query, int topk) {
        auto results = indexer.search(query, topk);
        nlohmann::json json_results = nlohmann::json::array();
        for (const auto& result : results) {
            nlohmann::json item;
            item["doc_id"] = result.doc_id;
            item["url"] = result.url;
            item["title"] = result.title;
            item["snippet"] = result.snippet;
            item["score"] = result.score;
            json_results.push_back(item);
        }
        nlohmann::json response;
        response["results"] = json_results;
        response["total"] = results.size();
        return response.dump();
    });
    
    // Start scheduler
    scheduler.start();
    
    // Start API server in separate thread
    std::thread api_thread([&api_server]() {
        api_server.start();
    });
    
    // Main crawl loop
    Logger::instance().info("Starting crawl loop");
    
    // Add seed URLs (example)
    std::vector<std::string> seed_urls = {
        "https://example.com",
        "https://github.com"
    };
    scheduler.add_seed_urls(seed_urls);
    
    CrawlTask task;
    while (scheduler.get_next_task(task)) {
        Metrics::instance().increment_counter("crawl_attempts");
        
        // Check deduplication
        if (dedup.is_url_seen(task.url)) {
            Metrics::instance().increment_counter("crawl_duplicates");
            scheduler.mark_completed(task.url);
            continue;
        }
        
        // Fetch
        FetchResult fetch_result = fetcher.fetch(task.url);
        
        if (!fetch_result.success) {
            Logger::instance().warn("Failed to fetch: " + task.url);
            scheduler.mark_failed(task.url, task.retry_count < config.scheduler_max_retries());
            continue;
        }
        
        // Check content deduplication
        if (dedup.is_content_seen(fetch_result.content_hash)) {
            Metrics::instance().increment_counter("content_duplicates");
            scheduler.mark_completed(task.url);
            continue;
        }
        
        // Parse
        ParsedDocument doc = parser.parse(task.url, fetch_result.content);
        
        // Index
        std::unordered_map<std::string, std::string> metadata;
        indexer.index_document(doc, metadata);
        
        // Save to storage
        storage.save_document(indexer.total_documents(), task.url, 
                             fetch_result.content, metadata);
        
        // Mark as seen
        dedup.mark_url_seen(task.url);
        dedup.mark_content_seen(fetch_result.content_hash, 
                                std::to_string(indexer.total_documents()));
        
        // Extract and add new links
        for (const auto& link : doc.links) {
            scheduler.add_url(link, 0);
        }
        
        scheduler.mark_completed(task.url);
        Metrics::instance().increment_counter("crawl_success");
        
        // Update metrics
        Metrics::instance().set_gauge("scheduler_queue_size", scheduler.queue_size());
        Metrics::instance().set_gauge("indexer_total_docs", indexer.total_documents());
        
        // Small delay to avoid overwhelming
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Cleanup
    scheduler.stop();
    api_server.stop();
    if (api_thread.joinable()) {
        api_thread.join();
    }
    
    Logger::instance().info("Web crawler service stopped");
    return 0;
}
