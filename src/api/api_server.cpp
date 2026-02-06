#include "api_server.h"
#include "../observability/logger.h"
#include "../observability/metrics.h"
#include <crow.h>
#include <nlohmann/json.hpp>

namespace crawler {

ApiServer::ApiServer() = default;

ApiServer::~ApiServer() {
    stop();
}

bool ApiServer::init(const std::string& host, int port, int threads) {
    host_ = host;
    port_ = port;
    threads_ = threads;
    return true;
}

void ApiServer::set_search_handler(std::function<std::string(const std::string&, int)> handler) {
    search_handler_ = handler;
}

void ApiServer::set_recommend_handler(std::function<std::string(const std::string&)> handler) {
    recommend_handler_ = handler;
}

void ApiServer::set_metrics_handler(std::function<std::string()> handler) {
    metrics_handler_ = handler;
}

void ApiServer::setup_routes() {
    // This will be called from start() to set up Crow routes
}

void ApiServer::start() {
    crow::SimpleApp app;
    
    // Search endpoint
    CROW_ROUTE(app, "/search")
    .methods("GET"_method)
    ([](const crow::request& req) {
        auto& logger = Logger::instance();
        auto& metrics = Metrics::instance();
        
        std::string query = req.url_params.get("q") ? req.url_params.get("q") : "";
        int topk = req.url_params.get("topk") ? std::stoi(req.url_params.get("topk")) : 10;
        
        if (query.empty()) {
            crow::response res(400);
            res.body = R"({"error": "Missing query parameter 'q'"})";
            res.set_header("Content-Type", "application/json");
            return res;
        }
        
        metrics.increment_counter("api_search_requests");
        auto start = std::chrono::steady_clock::now();
        
        // TODO: Call actual search handler
        nlohmann::json result;
        result["query"] = query;
        result["results"] = nlohmann::json::array();
        result["total"] = 0;
        
        auto end = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        metrics.record_histogram("api_search_latency_ms", latency);
        
        crow::response res(200);
        res.body = result.dump();
        res.set_header("Content-Type", "application/json");
        return res;
    });
    
    // Recommend endpoint
    CROW_ROUTE(app, "/recommend")
    .methods("GET"_method)
    ([](const crow::request& req) {
        auto& metrics = Metrics::instance();
        
        std::string sku = req.url_params.get("sku") ? req.url_params.get("sku") : "";
        
        if (sku.empty()) {
            crow::response res(400);
            res.body = R"({"error": "Missing parameter 'sku'"})";
            res.set_header("Content-Type", "application/json");
            return res;
        }
        
        metrics.increment_counter("api_recommend_requests");
        
        // TODO: Call actual recommend handler
        nlohmann::json result;
        result["sku"] = sku;
        result["recommendations"] = nlohmann::json::array();
        
        crow::response res(200);
        res.body = result.dump();
        res.set_header("Content-Type", "application/json");
        return res;
    });
    
    // Metrics endpoint
    CROW_ROUTE(app, "/metrics")
    .methods("GET"_method)
    ([]() {
        auto& metrics = Metrics::instance();
        crow::response res(200);
        res.body = metrics.to_prometheus();
        res.set_header("Content-Type", "text/plain");
        return res;
    });
    
    // Health check
    CROW_ROUTE(app, "/health")
    .methods("GET"_method)
    ([]() {
        nlohmann::json result;
        result["status"] = "healthy";
        crow::response res(200);
        res.body = result.dump();
        res.set_header("Content-Type", "application/json");
        return res;
    });
    
    app.port(port_).multithreaded().run();
    running_ = true;
}

void ApiServer::stop() {
    running_ = false;
}

} // namespace crawler
