#pragma once

#include <string>
#include <cstdint>

namespace crawler {

class HashUtils {
public:
    // Compute xxhash (fast, non-cryptographic)
    static uint64_t xxhash(const std::string& data);
    
    // Compute SHA256 (slower, cryptographic)
    static std::string sha256(const std::string& data);
    
    // Hash URL for deduplication
    static uint64_t hash_url(const std::string& url);
    
    // Hash content for deduplication
    static uint64_t hash_content(const std::string& content);
};

} // namespace crawler
