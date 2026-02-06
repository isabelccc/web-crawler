#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace crawler {

struct ParsedDocument {
    std::string url;
    std::string title;
    std::string text_content;
    std::vector<std::string> links;
    std::vector<std::pair<std::string, std::string>> links_with_anchor; // (url, anchor_text)
    std::unordered_map<std::string, std::string> metadata; // key-value pairs
    
    // For indexing
    std::vector<std::string> tokens;
    std::unordered_map<std::string, std::vector<size_t>> term_positions; // term -> positions
};

class Parser {
public:
    Parser();
    ~Parser();
    
    // Parse HTML content
    ParsedDocument parse(const std::string& url, const std::string& html_content);
    
    // Extract text from HTML
    std::string extract_text(const std::string& html);
    
    // Extract links from HTML
    std::vector<std::pair<std::string, std::string>> extract_links(
        const std::string& html, const std::string& base_url);
    
    // Tokenize text
    std::vector<std::string> tokenize(const std::string& text);
    
    // Normalize token (lowercase, remove punctuation)
    std::string normalize_token(const std::string& token);

private:
    void extract_text_recursive(void* node, std::string& text);
    void extract_links_recursive(void* node, const std::string& base_url, 
                                 std::vector<std::pair<std::string, std::string>>& links);
};

} // namespace crawler
