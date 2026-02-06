#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <chrono>
#include <functional>

namespace crawler {

struct CrawlTask {
    std::string url;
    int priority = 0;
    int retry_count = 0;
    std::chrono::steady_clock::time_point next_retry_time;
    
    bool operator<(const CrawlTask& other) const {
        return priority < other.priority;
    }
};

class Scheduler {
public:
    Scheduler();
    ~Scheduler();
    
    // Add URL to crawl queue
    bool add_url(const std::string& url, int priority = 0);
    
    // Add URLs from seed list
    bool add_seed_urls(const std::vector<std::string>& urls);
    
    // Get next URL to crawl (blocking)
    bool get_next_task(CrawlTask& task);
    
    // Mark task as completed
    void mark_completed(const std::string& url);
    
    // Mark task as failed (will retry if retries left)
    void mark_failed(const std::string& url, bool will_retry = true);
    
    // Start worker threads
    void start();
    
    // Stop scheduler
    void stop();
    
    // Set callback for completed tasks
    void set_task_callback(std::function<void(const CrawlTask&)> callback);
    
    // Statistics
    size_t queue_size() const;
    size_t total_scheduled() const { return total_scheduled_; }
    size_t total_completed() const { return total_completed_; }
    size_t total_failed() const { return total_failed_; }

private:
    void worker_thread();
    void update_domain_backoff(const std::string& domain);
    bool can_fetch_domain(const std::string& domain) const;
    
    std::priority_queue<CrawlTask> task_queue_;
    mutable std::mutex queue_mutex_;
    
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> domain_backoff_;
    mutable std::mutex backoff_mutex_;
    
    std::vector<std::thread> workers_;
    std::atomic<bool> running_{false};
    
    std::atomic<size_t> total_scheduled_{0};
    std::atomic<size_t> total_completed_{0};
    std::atomic<size_t> total_failed_{0};
    
    std::function<void(const CrawlTask&)> task_callback_;
    
    int max_retries_ = 3;
    int retry_backoff_ms_ = 1000;
    int worker_threads_ = 8;
};

} // namespace crawler
