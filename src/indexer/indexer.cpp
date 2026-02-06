#include "indexer.h"
#include "../utils/config.h"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <sstream>

namespace crawler {

Indexer::Indexer(const std::string& index_dir) : index_dir_(index_dir) {
    auto& config = Config::instance();
    // Load config values if available
}

Indexer::~Indexer() {
    flush_segment();
}

bool Indexer::index_document(const ParsedDocument& parsed_doc) {
    return index_document(parsed_doc, {});
}

bool Indexer::index_document(const ParsedDocument& parsed_doc,
                            const std::unordered_map<std::string, std::string>& metadata) {
    std::lock_guard<std::mutex> lock(index_mutex_);
    
    Document doc;
    doc.doc_id = next_doc_id_++;
    doc.url = parsed_doc.url;
    doc.title = parsed_doc.title;
    doc.text_content = parsed_doc.text_content;
    doc.term_positions = parsed_doc.term_positions;
    
    // Extract metadata
    if (metadata.count("category")) {
        doc.category = metadata.at("category");
    }
    if (metadata.count("price")) {
        doc.price = std::stod(metadata.at("price"));
    }
    if (metadata.count("brand")) {
        doc.brand = metadata.at("brand");
    }
    
    // Update inverted index
    size_t doc_length = 0;
    for (const auto& [term, positions] : parsed_doc.term_positions) {
        if (term.empty()) continue;
        
        Posting posting;
        posting.doc_id = doc.doc_id;
        posting.positions = positions;
        posting.tf = static_cast<double>(positions.size());
        
        inverted_index_[term].push_back(posting);
        doc_length += positions.size();
    }
    
    doc_lengths_[doc.doc_id] = doc_length;
    forward_index_[doc.doc_id] = doc;
    
    total_documents_++;
    current_segment_size_++;
    
    // Update average document length
    if (total_documents_ > 0) {
        avg_doc_length_ = (avg_doc_length_ * (total_documents_ - 1) + doc_length) / total_documents_;
    }
    
    // Flush if segment is full
    if (current_segment_size_ >= max_docs_per_segment_) {
        flush_segment();
    }
    
    return true;
}

std::vector<SearchResult> Indexer::search(const std::string& query, int topk) {
    std::lock_guard<std::mutex> lock(index_mutex_);
    
    // Tokenize query
    std::istringstream iss(query);
    std::vector<std::string> query_terms;
    std::string term;
    while (iss >> term) {
        std::transform(term.begin(), term.end(), term.begin(), ::tolower);
        query_terms.push_back(term);
    }
    
    // Score documents
    std::unordered_map<uint64_t, double> doc_scores;
    
    for (const auto& query_term : query_terms) {
        auto it = inverted_index_.find(query_term);
        if (it == inverted_index_.end()) continue;
        
        // Compute IDF
        double idf = std::log(static_cast<double>(total_documents_) / it->second.size());
        
        // Score each document containing this term
        for (const auto& posting : it->second) {
            double bm25 = calculate_bm25(query_term, posting.doc_id, posting.positions);
            doc_scores[posting.doc_id] += bm25 * idf;
        }
    }
    
    // Sort by score
    std::vector<std::pair<uint64_t, double>> scored_docs;
    for (const auto& [doc_id, score] : doc_scores) {
        scored_docs.emplace_back(doc_id, score);
    }
    
    std::sort(scored_docs.begin(), scored_docs.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Build results
    std::vector<SearchResult> results;
    for (size_t i = 0; i < std::min(static_cast<size_t>(topk), scored_docs.size()); i++) {
        uint64_t doc_id = scored_docs[i].first;
        auto doc_it = forward_index_.find(doc_id);
        if (doc_it == forward_index_.end()) continue;
        
        SearchResult result;
        result.doc_id = doc_id;
        result.url = doc_it->second.url;
        result.title = doc_it->second.title;
        result.score = scored_docs[i].second;
        
        // Generate snippet (first 200 chars)
        result.snippet = doc_it->second.text_content.substr(0, 200);
        if (doc_it->second.text_content.length() > 200) {
            result.snippet += "...";
        }
        
        results.push_back(result);
    }
    
    return results;
}

double Indexer::calculate_bm25(const std::string& term, uint64_t doc_id,
                               const std::vector<size_t>& positions) {
    double tf = static_cast<double>(positions.size());
    auto doc_len_it = doc_lengths_.find(doc_id);
    if (doc_len_it == doc_lengths_.end()) return 0.0;
    
    double doc_length = static_cast<double>(doc_len_it->second);
    double normalized_length = doc_length / avg_doc_length_;
    
    double numerator = tf * (k1_ + 1);
    double denominator = tf + k1_ * (1 - b_ + b_ * normalized_length);
    
    return numerator / denominator;
}

void Indexer::flush_segment() {
    if (current_segment_size_ == 0) return;
    
    // Write segment to disk (simplified)
    std::string segment_file = index_dir_ + "/segment_" + std::to_string(segment_count_++) + ".idx";
    std::ofstream out(segment_file, std::ios::binary);
    
    // TODO: Serialize index structures
    // For now, just mark as flushed
    
    current_segment_size_ = 0;
}

void Indexer::merge_segments() {
    // TODO: Implement segment merging
    // For now, just flush current segment
    flush_segment();
}

size_t Indexer::total_terms() const {
    std::lock_guard<std::mutex> lock(index_mutex_);
    return inverted_index_.size();
}

} // namespace crawler
