#include "dedup.h"
#include "../utils/hash_utils.h"
#include "../utils/url_utils.h"
#include <hiredis/hiredis.h>
#include <sstream>
#include <stdexcept>

namespace crawler {

Deduplicator::Deduplicator() = default;

Deduplicator::~Deduplicator() {
    if (redis_context_) {
        redisFree(static_cast<redisContext*>(redis_context_));
    }
}

bool Deduplicator::init_redis(const std::string& host, int port) {
    redisContext* ctx = redisConnect(host.c_str(), port);
    if (ctx == nullptr || ctx->err) {
        if (ctx) {
            redisFree(ctx);
        }
        redis_available_ = false;
        return false;
    }
    
    redis_context_ = ctx;
    redis_available_ = true;
    return true;
}

bool Deduplicator::is_url_seen(const std::string& url) {
    std::string canonical = UrlUtils::canonicalize(url);
    uint64_t url_hash = HashUtils::hash_url(canonical);
    
    // Try Redis first
    if (redis_available_) {
        std::string key = "dedup:url:" + std::to_string(url_hash);
        bool seen = check_redis_url(key);
        if (seen) {
            redis_hits_++;
            url_duplicates_++;
            return true;
        }
        redis_misses_++;
    }
    
    // Fallback to local
    if (use_local_fallback_ || !redis_available_) {
        std::lock_guard<std::mutex> lock(local_mutex_);
        if (local_url_set_.find(url_hash) != local_url_set_.end()) {
            url_duplicates_++;
            return true;
        }
    }
    
    return false;
}

void Deduplicator::mark_url_seen(const std::string& url) {
    std::string canonical = UrlUtils::canonicalize(url);
    uint64_t url_hash = HashUtils::hash_url(canonical);
    
    // Try Redis first
    if (redis_available_) {
        std::string key = "dedup:url:" + std::to_string(url_hash);
        mark_redis_url(key);
    }
    
    // Fallback to local
    if (use_local_fallback_ || !redis_available_) {
        std::lock_guard<std::mutex> lock(local_mutex_);
        local_url_set_.insert(url_hash);
    }
}

bool Deduplicator::is_content_seen(const std::string& content_hash) {
    uint64_t hash;
    try {
        hash = std::stoull(content_hash);
    } catch (...) {
        // If content_hash is not a number, hash it
        hash = HashUtils::hash_content(content_hash);
    }
    
    // Try Redis first
    if (redis_available_) {
        std::string key = "dedup:content:" + content_hash;
        bool seen = check_redis_content(key);
        if (seen) {
            redis_hits_++;
            content_duplicates_++;
            return true;
        }
        redis_misses_++;
    }
    
    // Fallback to local
    if (use_local_fallback_ || !redis_available_) {
        std::lock_guard<std::mutex> lock(local_mutex_);
        if (local_content_set_.find(hash) != local_content_set_.end()) {
            content_duplicates_++;
            return true;
        }
    }
    
    return false;
}

void Deduplicator::mark_content_seen(const std::string& content_hash, const std::string& doc_id) {
    uint64_t hash;
    try {
        hash = std::stoull(content_hash);
    } catch (...) {
        hash = HashUtils::hash_content(content_hash);
    }
    
    // Try Redis
    if (redis_available_) {
        std::string key = "dedup:content:" + content_hash;
        mark_redis_content(key, doc_id);
    }
    
    // Fallback to local
    if (use_local_fallback_ || !redis_available_) {
        std::lock_guard<std::mutex> lock(local_mutex_);
        local_content_set_.insert(hash);
    }
}

void Deduplicator::enable_local_fallback(bool enable) {
    use_local_fallback_ = enable;
}

bool Deduplicator::check_redis_url(const std::string& key) {
    if (!redis_context_) return false;
    
    redisContext* ctx = static_cast<redisContext*>(redis_context_);
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx, "EXISTS %s", key.c_str()));
    
    if (reply == nullptr) {
        redis_available_ = false;
        return false;
    }
    
    bool exists = reply->integer == 1;
    freeReplyObject(reply);
    return exists;
}

bool Deduplicator::check_redis_content(const std::string& key) {
    if (!redis_context_) return false;
    
    redisContext* ctx = static_cast<redisContext*>(redis_context_);
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx, "EXISTS %s", key.c_str()));
    
    if (reply == nullptr) {
        redis_available_ = false;
        return false;
    }
    
    bool exists = reply->integer == 1;
    freeReplyObject(reply);
    return exists;
}

void Deduplicator::mark_redis_url(const std::string& key) {
    if (!redis_context_) return;
    
    redisContext* ctx = static_cast<redisContext*>(redis_context_);
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx, "SETEX %s 86400 1", key.c_str()));
    
    if (reply == nullptr) {
        redis_available_ = false;
        return;
    }
    
    freeReplyObject(reply);
}

void Deduplicator::mark_redis_content(const std::string& key, const std::string& doc_id) {
    if (!redis_context_) return;
    
    redisContext* ctx = static_cast<redisContext*>(redis_context_);
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx, "SETEX %s 86400 %s", 
                                                              key.c_str(), doc_id.c_str()));
    
    if (reply == nullptr) {
        redis_available_ = false;
        return;
    }
    
    freeReplyObject(reply);
}

} // namespace crawler
