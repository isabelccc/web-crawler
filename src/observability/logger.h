#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <memory>

namespace crawler {

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger {
public:
    static Logger& instance();
    
    void init(const std::string& level, const std::string& format, const std::string& output);
    
    void log(LogLevel level, const std::string& message, const std::string& request_id = "");
    void debug(const std::string& message, const std::string& request_id = "");
    void info(const std::string& message, const std::string& request_id = "");
    void warn(const std::string& message, const std::string& request_id = "");
    void error(const std::string& message, const std::string& request_id = "");

private:
    Logger() = default;
    
    std::string format_message(LogLevel level, const std::string& message, const std::string& request_id);
    std::string level_to_string(LogLevel level);
    
    LogLevel min_level_ = LogLevel::INFO;
    bool json_format_ = false;
    std::unique_ptr<std::ofstream> file_output_;
    std::mutex log_mutex_;
};

} // namespace crawler
