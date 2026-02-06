#include <cassert>
#include "../../src/utils/url_utils.h"

int main() {
    using namespace crawler;
    
    // Test canonicalize
    assert(UrlUtils::canonicalize("https://example.com/page#fragment") == "https://example.com/page");
    
    // Test extract_domain
    assert(UrlUtils::extract_domain("https://example.com/page") == "example.com");
    
    // Test normalize
    std::string normalized = UrlUtils::normalize("HTTPS://EXAMPLE.COM/PAGE/");
    assert(normalized.find("https://example.com/page") != std::string::npos);
    
    // Test is_valid
    assert(UrlUtils::is_valid("https://example.com"));
    assert(!UrlUtils::is_valid("not a url"));
    
    return 0;
}
