#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <cstdint>

namespace crawler {

class Config {
public:
    static Config& instance();
    
    bool load(const std::string& config_path);
    
    // Scheduler
    int scheduler_worker_threads() const { return scheduler_worker_threads_; }
    int scheduler_queue_size() const { return scheduler_queue_size_; }
    int scheduler_max_retries() const { return scheduler_max_retries_; }
    int scheduler_retry_backoff_ms() const { return scheduler_retry_backoff_ms_; }
    
    // Fetcher
    int fetcher_connect_timeout_ms() const { return fetcher_connect_timeout_ms_; }
    int fetcher_read_timeout_ms() const { return fetcher_read_timeout_ms_; }
    int fetcher_max_redirects() const { return fetcher_max_redirects_; }
    std::string fetcher_user_agent() const { return fetcher_user_agent_; }
    
    // Rate limit
    bool rate_limit_enabled() const { return rate_limit_enabled_; }
    int rate_limit_per_domain(const std::string& domain) const;
    
    // Redis
    std::string redis_host() const { return redis_host_; }
    int redis_port() const { return redis_port_; }
    int redis_connection_pool_size() const { return redis_connection_pool_size_; }
    
    // API
    std::string api_host() const { return api_host_; }
    int api_port() const { return api_port_; }
    int api_threads() const { return api_threads_; }
    
    // Memory
    int64_t max_memory_mb() const { return max_memory_mb_; }
    int flush_threshold_percent() const { return flush_threshold_percent_; }

private:
    Config() = default;
    
    int scheduler_worker_threads_ = 8;
    int scheduler_queue_size_ = 10000;
    int scheduler_max_retries_ = 3;
    int scheduler_retry_backoff_ms_ = 1000;
    
    int fetcher_connect_timeout_ms_ = 5000;
    int fetcher_read_timeout_ms_ = 10000;
    int fetcher_max_redirects_ = 5;
    std::string fetcher_user_agent_ = "WebCrawler/1.0";
    
    bool rate_limit_enabled_ = true;
    std::unordered_map<std::string, int> rate_limit_per_domain_;
    int rate_limit_default_ = 10;
    
    std::string redis_host_ = "localhost";
    int redis_port_ = 6379;
    int redis_connection_pool_size_ = 10;
    
    std::string api_host_ = "0.0.0.0";
    int api_port_ = 8080;
    int api_threads_ = 4;
    
    int64_t max_memory_mb_ = 2048;
    int flush_threshold_percent_ = 80;
};

} // namespace crawler
