#include "scheduler.h"
#include "../utils/config.h"
#include "../utils/url_utils.h"
#include <algorithm>
#include <thread>
#include <chrono>

namespace crawler {

Scheduler::Scheduler() {
    auto& config = Config::instance();
    max_retries_ = config.scheduler_max_retries();
    retry_backoff_ms_ = config.scheduler_retry_backoff_ms();
    worker_threads_ = config.scheduler_worker_threads();
}

Scheduler::~Scheduler() {
    stop();
}

bool Scheduler::add_url(const std::string& url, int priority) {
    std::string normalized = UrlUtils::canonicalize(url);
    if (!UrlUtils::is_valid(normalized)) {
        return false;
    }
    
    CrawlTask task;
    task.url = normalized;
    task.priority = priority;
    task.retry_count = 0;
    task.next_retry_time = std::chrono::steady_clock::now();
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.push(task);
        total_scheduled_++;
    }
    
    return true;
}

bool Scheduler::add_seed_urls(const std::vector<std::string>& urls) {
    for (const auto& url : urls) {
        if (!add_url(url)) {
            return false;
        }
    }
    return true;
}

bool Scheduler::get_next_task(CrawlTask& task) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    
    while (task_queue_.empty() && running_) {
        // Wait for tasks
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (!running_ && task_queue_.empty()) {
        return false;
    }
    
    if (task_queue_.empty()) {
        return false;
    }
    
    task = task_queue_.top();
    task_queue_.pop();
    
    // Check if it's time to retry
    if (std::chrono::steady_clock::now() < task.next_retry_time) {
        // Put it back and try another
        task_queue_.push(task);
        return false;
    }
    
    // Check domain backoff
    std::string domain = UrlUtils::extract_domain(task.url);
    if (!can_fetch_domain(domain)) {
        // Put it back
        task_queue_.push(task);
        return false;
    }
    
    return true;
}

void Scheduler::mark_completed(const std::string& url) {
    total_completed_++;
    if (task_callback_) {
        CrawlTask task;
        task.url = url;
        task_callback_(task);
    }
}

void Scheduler::mark_failed(const std::string& url, bool will_retry) {
    if (!will_retry) {
        total_failed_++;
        return;
    }
    
    // Find task and retry
    CrawlTask task;
    task.url = url;
    task.retry_count = 1; // Simplified
    task.next_retry_time = std::chrono::steady_clock::now() + 
                          std::chrono::milliseconds(retry_backoff_ms_);
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.push(task);
    }
    
    update_domain_backoff(UrlUtils::extract_domain(url));
}

void Scheduler::start() {
    running_ = true;
    for (int i = 0; i < worker_threads_; i++) {
        workers_.emplace_back(&Scheduler::worker_thread, this);
    }
}

void Scheduler::stop() {
    running_ = false;
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
}

void Scheduler::set_task_callback(std::function<void(const CrawlTask&)> callback) {
    task_callback_ = callback;
}

size_t Scheduler::queue_size() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return task_queue_.size();
}

void Scheduler::worker_thread() {
    // Worker threads can process tasks
    // This is a placeholder for future async processing
}

void Scheduler::update_domain_backoff(const std::string& domain) {
    std::lock_guard<std::mutex> lock(backoff_mutex_);
    domain_backoff_[domain] = std::chrono::steady_clock::now() + 
                              std::chrono::seconds(1);
}

bool Scheduler::can_fetch_domain(const std::string& domain) const {
    std::lock_guard<std::mutex> lock(backoff_mutex_);
    auto it = domain_backoff_.find(domain);
    if (it == domain_backoff_.end()) {
        return true;
    }
    return std::chrono::steady_clock::now() >= it->second;
}

} // namespace crawler
