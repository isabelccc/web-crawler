#include "url_utils.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <regex>

namespace crawler {

std::string UrlUtils::canonicalize(const std::string& url) {
    std::string result = url;
    
    // Remove fragment
    size_t frag_pos = result.find('#');
    if (frag_pos != std::string::npos) {
        result = result.substr(0, frag_pos);
    }
    
    // TODO: Sort query parameters
    // For now, just return without fragment
    
    return result;
}

std::string UrlUtils::extract_domain(const std::string& url) {
    std::regex domain_regex(R"(https?://([^/]+))");
    std::smatch match;
    if (std::regex_search(url, match, domain_regex)) {
        return match[1].str();
    }
    return "";
}

std::string UrlUtils::normalize(const std::string& url) {
    std::string result = url;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    
    // Remove trailing slash (except for root)
    if (result.length() > 1 && result.back() == '/') {
        result.pop_back();
    }
    
    return result;
}

bool UrlUtils::is_valid(const std::string& url) {
    std::regex url_regex(R"(https?://[^\s]+)");
    return std::regex_match(url, url_regex);
}

std::string UrlUtils::resolve(const std::string& base_url, const std::string& relative_url) {
    if (relative_url.empty()) return base_url;
    
    if (relative_url.find("http://") == 0 || relative_url.find("https://") == 0) {
        return relative_url;
    }
    
    // Simple resolution
    if (relative_url[0] == '/') {
        std::string domain = extract_domain(base_url);
        return domain + relative_url;
    }
    
    // Relative path
    size_t last_slash = base_url.find_last_of('/');
    if (last_slash != std::string::npos) {
        return base_url.substr(0, last_slash + 1) + relative_url;
    }
    
    return base_url + "/" + relative_url;
}

} // namespace crawler
