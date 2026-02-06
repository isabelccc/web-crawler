#include "fetcher.h"
#include "../utils/config.h"
#include "../utils/hash_utils.h"
#include <curl/curl.h>
#include <sstream>
#include <mutex>

namespace crawler {

static std::once_flag curl_init_flag;

struct CurlWriteData {
    std::string data;
};

static size_t curl_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    CurlWriteData* data = static_cast<CurlWriteData*>(userp);
    data->data.append(static_cast<char*>(contents), total_size);
    return total_size;
}

Fetcher::Fetcher() {
    std::call_once(curl_init_flag, []() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    });
    
    auto& config = Config::instance();
    connect_timeout_ms_ = config.fetcher_connect_timeout_ms();
    read_timeout_ms_ = config.fetcher_read_timeout_ms();
    max_redirects_ = config.fetcher_max_redirects();
    user_agent_ = config.fetcher_user_agent();
}

Fetcher::~Fetcher() {
    // curl_global_cleanup(); // Don't cleanup, might be used elsewhere
}

FetchResult Fetcher::fetch(const std::string& url) {
    total_fetches_++;
    auto start = std::chrono::steady_clock::now();
    
    FetchResult result = fetch_impl(url, 0);
    
    auto end = std::chrono::steady_clock::now();
    result.latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    total_latency_ms_ += result.latency.count();
    
    if (result.success) {
        successful_fetches_++;
        result.content_hash = HashUtils::hash_content(result.content);
        result.content_size = result.content.size();
    } else {
        failed_fetches_++;
    }
    
    return result;
}

FetchResult Fetcher::fetch_impl(const std::string& url, int redirect_count) {
    if (redirect_count > max_redirects_) {
        FetchResult result;
        result.success = false;
        result.error_message = "Too many redirects";
        return result;
    }
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        FetchResult result;
        result.success = false;
        result.error_message = "Failed to initialize CURL";
        return result;
    }
    
    CurlWriteData write_data;
    FetchResult result;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_data);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L); // Manual redirect handling
    curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent_.c_str());
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, connect_timeout_ms_);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, read_timeout_ms_);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // For testing
    
    CURLcode res = curl_easy_perform(curl);
    
    if (res == CURLE_OK) {
        long http_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        result.http_status = http_code;
        
        char* final_url = nullptr;
        curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &final_url);
        if (final_url) {
            result.final_url = final_url;
        }
        
        char* content_type = nullptr;
        curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
        if (content_type) {
            result.content_type = content_type;
        }
        
        if (http_code >= 200 && http_code < 300) {
            result.success = true;
            result.content = write_data.data;
        } else if (http_code >= 300 && http_code < 400) {
            // Handle redirect
            char* location = nullptr;
            curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &location);
            if (location) {
                result.redirects.push_back(location);
                // Recursively follow redirect
                FetchResult redirect_result = fetch_impl(location, redirect_count + 1);
                redirect_result.redirects.insert(redirect_result.redirects.begin(), 
                                                result.redirects.begin(), 
                                                result.redirects.end());
                curl_easy_cleanup(curl);
                return redirect_result;
            }
        }
    } else {
        result.success = false;
        result.error_message = curl_easy_strerror(res);
    }
    
    curl_easy_cleanup(curl);
    return result;
}

void Fetcher::set_connect_timeout(int ms) {
    connect_timeout_ms_ = ms;
}

void Fetcher::set_read_timeout(int ms) {
    read_timeout_ms_ = ms;
}

void Fetcher::set_max_redirects(int max) {
    max_redirects_ = max;
}

void Fetcher::set_user_agent(const std::string& ua) {
    user_agent_ = ua;
}

double Fetcher::average_latency_ms() const {
    size_t total = total_fetches_.load();
    if (total == 0) return 0.0;
    return static_cast<double>(total_latency_ms_.load()) / total;
}

} // namespace crawler
