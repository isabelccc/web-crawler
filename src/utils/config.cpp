#include "config.h"
#include <fstream>
#include <sstream>
#include <yaml-cpp/yaml.h>
#include <iostream>

namespace crawler {

Config& Config::instance() {
    static Config instance;
    return instance;
}

bool Config::load(const std::string& config_path) {
    try {
        YAML::Node config = YAML::LoadFile(config_path);
        
        // Scheduler
        if (config["scheduler"]) {
            auto sched = config["scheduler"];
            if (sched["worker_threads"]) scheduler_worker_threads_ = sched["worker_threads"].as<int>();
            if (sched["queue_size"]) scheduler_queue_size_ = sched["queue_size"].as<int>();
            if (sched["max_retries"]) scheduler_max_retries_ = sched["max_retries"].as<int>();
            if (sched["retry_backoff_ms"]) scheduler_retry_backoff_ms_ = sched["retry_backoff_ms"].as<int>();
        }
        
        // Fetcher
        if (config["fetcher"]) {
            auto fetch = config["fetcher"];
            if (fetch["connect_timeout_ms"]) fetcher_connect_timeout_ms_ = fetch["connect_timeout_ms"].as<int>();
            if (fetch["read_timeout_ms"]) fetcher_read_timeout_ms_ = fetch["read_timeout_ms"].as<int>();
            if (fetch["max_redirects"]) fetcher_max_redirects_ = fetch["max_redirects"].as<int>();
            if (fetch["user_agent"]) fetcher_user_agent_ = fetch["user_agent"].as<std::string>();
        }
        
        // Rate limit
        if (config["rate_limit"]) {
            auto rl = config["rate_limit"];
            if (rl["enabled"]) rate_limit_enabled_ = rl["enabled"].as<bool>();
            if (rl["per_domain"]) {
                for (const auto& item : rl["per_domain"]) {
                    rate_limit_per_domain_[item.first.as<std::string>()] = item.second.as<int>();
                }
            }
            if (rl["default"]) rate_limit_default_ = rl["default"].as<int>();
        }
        
        // Redis
        if (config["redis"]) {
            auto redis = config["redis"];
            if (redis["host"]) redis_host_ = redis["host"].as<std::string>();
            if (redis["port"]) redis_port_ = redis["port"].as<int>();
            if (redis["connection_pool_size"]) redis_connection_pool_size_ = redis["connection_pool_size"].as<int>();
        }
        
        // API
        if (config["api"]) {
            auto api = config["api"];
            if (api["host"]) api_host_ = api["host"].as<std::string>();
            if (api["port"]) api_port_ = api["port"].as<int>();
            if (api["threads"]) api_threads_ = api["threads"].as<int>();
        }
        
        // Memory
        if (config["memory"]) {
            auto mem = config["memory"];
            if (mem["max_memory_mb"]) max_memory_mb_ = mem["max_memory_mb"].as<int64_t>();
            if (mem["flush_threshold_percent"]) flush_threshold_percent_ = mem["flush_threshold_percent"].as<int>();
        }
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

int Config::rate_limit_per_domain(const std::string& domain) const {
    auto it = rate_limit_per_domain_.find(domain);
    if (it != rate_limit_per_domain_.end()) {
        return it->second;
    }
    return rate_limit_default_;
}

} // namespace crawler
