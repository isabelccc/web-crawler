#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <memory>

namespace crawler {

struct FetchResult {
    bool success = false;
    int http_status = 0;
    std::string content;
    std::string final_url;
    std::string content_type;
    std::chrono::milliseconds latency{0};
    std::vector<std::string> redirects;
    std::string error_message;
    
    // Metadata
    std::string content_hash;
    size_t content_size = 0;
};

class Fetcher {
public:
    Fetcher();
    ~Fetcher();
    
    // Fetch URL
    FetchResult fetch(const std::string& url);
    
    // Set timeout
    void set_connect_timeout(int ms);
    void set_read_timeout(int ms);
    
    // Set max redirects
    void set_max_redirects(int max);
    
    // Set user agent
    void set_user_agent(const std::string& ua);
    
    // Statistics
    size_t total_fetches() const { return total_fetches_; }
    size_t successful_fetches() const { return successful_fetches_; }
    size_t failed_fetches() const { return failed_fetches_; }
    double average_latency_ms() const;

private:
    FetchResult fetch_impl(const std::string& url, int redirect_count);
    std::string make_request(const std::string& url);
    
    int connect_timeout_ms_ = 5000;
    int read_timeout_ms_ = 10000;
    int max_redirects_ = 5;
    std::string user_agent_ = "WebCrawler/1.0";
    
    std::atomic<size_t> total_fetches_{0};
    std::atomic<size_t> successful_fetches_{0};
    std::atomic<size_t> failed_fetches_{0};
    std::atomic<uint64_t> total_latency_ms_{0};
};

} // namespace crawler
