#pragma once

#include <string>

namespace crawler {

class UrlUtils {
public:
    // Canonicalize URL: remove fragment, sort query params
    static std::string canonicalize(const std::string& url);
    
    // Extract domain from URL
    static std::string extract_domain(const std::string& url);
    
    // Normalize URL (lowercase, remove trailing slash)
    static std::string normalize(const std::string& url);
    
    // Check if URL is valid
    static bool is_valid(const std::string& url);
    
    // Resolve relative URL
    static std::string resolve(const std::string& base_url, const std::string& relative_url);
};

} // namespace crawler
