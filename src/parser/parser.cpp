#include "parser.h"
#include "../utils/url_utils.h"
#include <gumbo.h>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <regex>

namespace crawler {

Parser::Parser() = default;
Parser::~Parser() = default;

ParsedDocument Parser::parse(const std::string& url, const std::string& html_content) {
    ParsedDocument doc;
    doc.url = url;
    
    GumboOutput* output = gumbo_parse(html_content.c_str());
    
    // Extract title
    GumboNode* root = output->root;
    if (root->type == GUMBO_NODE_ELEMENT) {
        GumboVector* children = &root->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            GumboNode* child = static_cast<GumboNode*>(children->data[i]);
            if (child->type == GUMBO_NODE_ELEMENT && child->v.element.tag == GUMBO_TAG_TITLE) {
                if (child->v.element.children.length > 0) {
                    GumboNode* title_text = static_cast<GumboNode*>(child->v.element.children.data[0]);
                    if (title_text->type == GUMBO_NODE_TEXT) {
                        doc.title = title_text->v.text.text;
                    }
                }
                break;
            }
        }
    }
    
    // Extract text content
    doc.text_content = extract_text(html_content);
    
    // Extract links
    doc.links_with_anchor = extract_links(html_content, url);
    for (const auto& link : doc.links_with_anchor) {
        doc.links.push_back(link.first);
    }
    
    // Tokenize
    doc.tokens = tokenize(doc.text_content);
    
    // Build term positions
    for (size_t i = 0; i < doc.tokens.size(); i++) {
        std::string normalized = normalize_token(doc.tokens[i]);
        if (!normalized.empty()) {
            doc.term_positions[normalized].push_back(i);
        }
    }
    
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return doc;
}

std::string Parser::extract_text(const std::string& html) {
    GumboOutput* output = gumbo_parse(html.c_str());
    std::string text;
    extract_text_recursive(output->root, text);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return text;
}

void Parser::extract_text_recursive(void* node_ptr, std::string& text) {
    GumboNode* node = static_cast<GumboNode*>(node_ptr);
    
    if (node->type == GUMBO_NODE_TEXT) {
        text += node->v.text.text;
        text += " ";
    } else if (node->type == GUMBO_NODE_ELEMENT) {
        // Skip script and style tags
        if (node->v.element.tag != GUMBO_TAG_SCRIPT &&
            node->v.element.tag != GUMBO_TAG_STYLE) {
            GumboVector* children = &node->v.element.children;
            for (unsigned int i = 0; i < children->length; ++i) {
                extract_text_recursive(children->data[i], text);
            }
        }
    }
}

std::vector<std::pair<std::string, std::string>> Parser::extract_links(
    const std::string& html, const std::string& base_url) {
    
    GumboOutput* output = gumbo_parse(html.c_str());
    std::vector<std::pair<std::string, std::string>> links;
    extract_links_recursive(output->root, base_url, links);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return links;
}

void Parser::extract_links_recursive(void* node_ptr, const std::string& base_url,
                                    std::vector<std::pair<std::string, std::string>>& links) {
    GumboNode* node = static_cast<GumboNode*>(node_ptr);
    
    if (node->type == GUMBO_NODE_ELEMENT && node->v.element.tag == GUMBO_TAG_A) {
        GumboAttribute* href = gumbo_get_attribute(&node->v.element.attributes, "href");
        if (href) {
            std::string url = href->value;
            std::string resolved_url = UrlUtils::resolve(base_url, url);
            
            // Extract anchor text
            std::string anchor_text;
            GumboVector* children = &node->v.element.children;
            for (unsigned int i = 0; i < children->length; ++i) {
                GumboNode* child = static_cast<GumboNode*>(children->data[i]);
                if (child->type == GUMBO_NODE_TEXT) {
                    anchor_text += child->v.text.text;
                }
            }
            
            links.emplace_back(resolved_url, anchor_text);
        }
    }
    
    if (node->type == GUMBO_NODE_ELEMENT) {
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            extract_links_recursive(children->data[i], base_url, links);
        }
    }
}

std::vector<std::string> Parser::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::regex word_regex(R"(\b\w+\b)");
    std::sregex_iterator iter(text.begin(), text.end(), word_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        tokens.push_back(iter->str());
    }
    
    return tokens;
}

std::string Parser::normalize_token(const std::string& token) {
    std::string normalized = token;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Remove non-alphanumeric (keep only letters and numbers)
    normalized.erase(std::remove_if(normalized.begin(), normalized.end(),
                                    [](char c) { return !std::isalnum(c); }),
                    normalized.end());
    
    return normalized;
}

} // namespace crawler
