#pragma once

#include <string>
#include <memory>
#include <functional>

namespace crawler {

class ApiServer {
public:
    ApiServer();
    ~ApiServer();
    
    // Initialize server
    bool init(const std::string& host, int port, int threads);
    
    // Start server (blocking)
    void start();
    
    // Stop server
    void stop();
    
    // Set search handler
    void set_search_handler(std::function<std::string(const std::string& query, int topk)> handler);
    
    // Set recommend handler
    void set_recommend_handler(std::function<std::string(const std::string& sku)> handler);
    
    // Set metrics handler
    void set_metrics_handler(std::function<std::string()> handler);

private:
    void setup_routes();
    
    std::string host_;
    int port_;
    int threads_;
    bool running_ = false;
    
    std::function<std::string(const std::string&, int)> search_handler_;
    std::function<std::string(const std::string&)> recommend_handler_;
    std::function<std::string()> metrics_handler_;
    
    void* app_; // Crow app pointer
};

} // namespace crawler
