#include "hash_utils.h"
#include <sstream>
#include <iomanip>
#include <cstring>

// xxhash fallback if not available
#ifndef XXHASH_H
extern "C" {
    uint64_t XXH64(const void* input, size_t length, uint64_t seed);
}
#endif

// OpenSSL SHA256
#include <openssl/sha.h>

namespace crawler {

uint64_t HashUtils::xxhash(const std::string& data) {
#ifdef XXHASH_H
    return XXH64(data.data(), data.size(), 0);
#else
    // Fallback: simple hash
    uint64_t hash = 5381;
    for (char c : data) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
#endif
}

std::string HashUtils::sha256(const std::string& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.data(), data.size());
    SHA256_Final(hash, &sha256);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

uint64_t HashUtils::hash_url(const std::string& url) {
    return xxhash(url);
}

uint64_t HashUtils::hash_content(const std::string& content) {
    return xxhash(content);
}

} // namespace crawler
