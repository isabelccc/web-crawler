#include "logger.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace crawler {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::init(const std::string& level, const std::string& format, const std::string& output) {
    if (level == "debug") min_level_ = LogLevel::DEBUG;
    else if (level == "info") min_level_ = LogLevel::INFO;
    else if (level == "warn") min_level_ = LogLevel::WARN;
    else if (level == "error") min_level_ = LogLevel::ERROR;
    
    json_format_ = (format == "json");
    
    if (output != "stdout" && !output.empty()) {
        file_output_ = std::make_unique<std::ofstream>(output, std::ios::app);
    }
}

void Logger::log(LogLevel level, const std::string& message, const std::string& request_id) {
    if (level < min_level_) return;
    
    std::lock_guard<std::mutex> lock(log_mutex_);
    std::string formatted = format_message(level, message, request_id);
    
    if (file_output_ && file_output_->is_open()) {
        *file_output_ << formatted << std::endl;
    } else {
        std::cout << formatted << std::endl;
    }
}

void Logger::debug(const std::string& message, const std::string& request_id) {
    log(LogLevel::DEBUG, message, request_id);
}

void Logger::info(const std::string& message, const std::string& request_id) {
    log(LogLevel::INFO, message, request_id);
}

void Logger::warn(const std::string& message, const std::string& request_id) {
    log(LogLevel::WARN, message, request_id);
}

void Logger::error(const std::string& message, const std::string& request_id) {
    log(LogLevel::ERROR, message, request_id);
}

std::string Logger::format_message(LogLevel level, const std::string& message, const std::string& request_id) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    std::tm tm_buf;
    localtime_r(&time_t, &tm_buf);
    
    if (json_format_) {
        oss << "{"
            << "\"timestamp\":\"" << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S")
            << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z\","
            << "\"level\":\"" << level_to_string(level) << "\",";
        if (!request_id.empty()) {
            oss << "\"request_id\":\"" << request_id << "\",";
        }
        oss << "\"message\":\"" << message << "\""
            << "}";
    } else {
        oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
            << "." << std::setfill('0') << std::setw(3) << ms.count()
            << " [" << level_to_string(level) << "]";
        if (!request_id.empty()) {
            oss << " [req:" << request_id << "]";
        }
        oss << " " << message;
    }
    
    return oss.str();
}

std::string Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

} // namespace crawler
