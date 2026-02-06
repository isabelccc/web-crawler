#pragma once

#include <string>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <chrono>

namespace crawler {

class Metrics {
public:
    static Metrics& instance();
    
    // Counter metrics
    void increment_counter(const std::string& name, int value = 1);
    int64_t get_counter(const std::string& name) const;
    
    // Gauge metrics
    void set_gauge(const std::string& name, double value);
    double get_gauge(const std::string& name) const;
    
    // Histogram metrics (simplified: just track min/max/avg)
    void record_histogram(const std::string& name, double value);
    
    // Get all metrics as Prometheus format
    std::string to_prometheus() const;
    
    // Get metrics as JSON
    std::string to_json() const;

private:
    Metrics() = default;
    
    std::unordered_map<std::string, std::atomic<int64_t>> counters_;
    std::unordered_map<std::string, std::atomic<double>> gauges_;
    std::unordered_map<std::string, std::vector<double>> histograms_;
    mutable std::mutex histograms_mutex_;
};

} // namespace crawler
