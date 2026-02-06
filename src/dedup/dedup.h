#pragma once

#include <string>
#include <unordered_set>
#include <memory>
#include <mutex>

namespace crawler {

class Deduplicator {
public:
    Deduplicator();
    ~Deduplicator();
    
    // Check if URL is duplicate
    bool is_url_seen(const std::string& url);
    
    // Mark URL as seen
    void mark_url_seen(const std::string& url);
    
    // Check if content is duplicate
    bool is_content_seen(const std::string& content_hash);
    
    // Mark content as seen
    void mark_content_seen(const std::string& content_hash, const std::string& doc_id);
    
    // Initialize Redis connection
    bool init_redis(const std::string& host, int port);
    
    // Fallback to local storage if Redis fails
    void enable_local_fallback(bool enable);
    
    // Statistics
    size_t url_duplicates() const { return url_duplicates_; }
    size_t content_duplicates() const { return content_duplicates_; }
    size_t redis_hits() const { return redis_hits_; }
    size_t redis_misses() const { return redis_misses_; }

private:
    bool check_redis_url(const std::string& url_hash);
    bool check_redis_content(const std::string& content_hash);
    void mark_redis_url(const std::string& url_hash);
    void mark_redis_content(const std::string& content_hash, const std::string& doc_id);
    
    // Local fallback
    std::unordered_set<uint64_t> local_url_set_;
    std::unordered_set<uint64_t> local_content_set_;
    mutable std::mutex local_mutex_;
    bool use_local_fallback_ = false;
    bool redis_available_ = false;
    
    void* redis_context_ = nullptr; // hiredis context
    
    std::atomic<size_t> url_duplicates_{0};
    std::atomic<size_t> content_duplicates_{0};
    std::atomic<size_t> redis_hits_{0};
    std::atomic<size_t> redis_misses_{0};
};

} // namespace crawler
