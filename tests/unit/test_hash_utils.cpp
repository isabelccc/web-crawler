#include <cassert>
#include "../../src/utils/hash_utils.h"

int main() {
    using namespace crawler;
    
    // Test hash consistency
    uint64_t hash1 = HashUtils::hash_url("https://example.com");
    uint64_t hash2 = HashUtils::hash_url("https://example.com");
    assert(hash1 == hash2);
    
    // Test different URLs have different hashes
    uint64_t hash3 = HashUtils::hash_url("https://example.org");
    assert(hash1 != hash3);
    
    return 0;
}
