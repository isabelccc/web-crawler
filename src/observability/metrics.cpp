#include "metrics.h"
#include <sstream>
#include <algorithm>
#include <numeric>

namespace crawler {

Metrics& Metrics::instance() {
    static Metrics instance;
    return instance;
}

void Metrics::increment_counter(const std::string& name, int value) {
    counters_[name] += value;
}

int64_t Metrics::get_counter(const std::string& name) const {
    auto it = counters_.find(name);
    if (it != counters_.end()) {
        return it->second.load();
    }
    return 0;
}

void Metrics::set_gauge(const std::string& name, double value) {
    gauges_[name] = value;
}

double Metrics::get_gauge(const std::string& name) const {
    auto it = gauges_.find(name);
    if (it != gauges_.end()) {
        return it->second.load();
    }
    return 0.0;
}

void Metrics::record_histogram(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(histograms_mutex_);
    histograms_[name].push_back(value);
    
    // Keep only last 1000 values
    if (histograms_[name].size() > 1000) {
        histograms_[name].erase(histograms_[name].begin());
    }
}

std::string Metrics::to_prometheus() const {
    std::ostringstream oss;
    
    // Counters
    for (const auto& [name, value] : counters_) {
        oss << "# TYPE " << name << " counter\n";
        oss << name << " " << value.load() << "\n";
    }
    
    // Gauges
    for (const auto& [name, value] : gauges_) {
        oss << "# TYPE " << name << " gauge\n";
        oss << name << " " << value.load() << "\n";
    }
    
    // Histograms (simplified)
    std::lock_guard<std::mutex> lock(histograms_mutex_);
    for (const auto& [name, values] : histograms_) {
        if (values.empty()) continue;
        
        double sum = std::accumulate(values.begin(), values.end(), 0.0);
        double avg = sum / values.size();
        double min = *std::min_element(values.begin(), values.end());
        double max = *std::max_element(values.begin(), values.end());
        
        oss << "# TYPE " << name << "_avg gauge\n";
        oss << name << "_avg " << avg << "\n";
        oss << "# TYPE " << name << "_min gauge\n";
        oss << name << "_min " << min << "\n";
        oss << "# TYPE " << name << "_max gauge\n";
        oss << name << "_max " << max << "\n";
    }
    
    return oss.str();
}

std::string Metrics::to_json() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"counters\": {\n";
    
    bool first = true;
    for (const auto& [name, value] : counters_) {
        if (!first) oss << ",\n";
        oss << "    \"" << name << "\": " << value.load();
        first = false;
    }
    
    oss << "\n  },\n";
    oss << "  \"gauges\": {\n";
    
    first = true;
    for (const auto& [name, value] : gauges_) {
        if (!first) oss << ",\n";
        oss << "    \"" << name << "\": " << value.load();
        first = false;
    }
    
    oss << "\n  }\n";
    oss << "}\n";
    
    return oss.str();
}

} // namespace crawler
