#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include "parser.h"

namespace crawler {

struct Document {
    uint64_t doc_id;
    std::string url;
    std::string title;
    std::string text_content;
    std::unordered_map<std::string, std::vector<size_t>> term_positions;
    
    // Metadata for recommendation
    std::string category;
    double price = 0.0;
    std::string brand;
};

struct Posting {
    uint64_t doc_id;
    std::vector<size_t> positions;
    double tf = 0.0;
    double idf = 0.0;
    double bm25_score = 0.0;
};

struct SearchResult {
    uint64_t doc_id;
    std::string url;
    std::string title;
    std::string snippet;
    double score;
};

class Indexer {
public:
    Indexer(const std::string& index_dir);
    ~Indexer();
    
    // Index a document
    bool index_document(const ParsedDocument& parsed_doc);
    
    // Index a document with metadata
    bool index_document(const ParsedDocument& parsed_doc, 
                       const std::unordered_map<std::string, std::string>& metadata);
    
    // Search
    std::vector<SearchResult> search(const std::string& query, int topk = 10);
    
    // Flush current segment to disk
    void flush_segment();
    
    // Merge segments
    void merge_segments();
    
    // Statistics
    size_t total_documents() const { return total_documents_; }
    size_t total_terms() const;
    size_t segment_count() const { return segment_count_; }

private:
    void compute_tf_idf();
    void compute_bm25();
    double calculate_bm25(const std::string& term, uint64_t doc_id, 
                         const std::vector<size_t>& positions);
    
    std::string index_dir_;
    
    // In-memory index structures
    std::unordered_map<std::string, std::vector<Posting>> inverted_index_; // term -> postings
    std::unordered_map<uint64_t, Document> forward_index_; // doc_id -> document
    std::unordered_map<uint64_t, size_t> doc_lengths_; // doc_id -> length
    
    uint64_t next_doc_id_ = 1;
    size_t current_segment_size_ = 0;
    size_t segment_count_ = 0;
    
    mutable std::mutex index_mutex_;
    
    std::atomic<size_t> total_documents_{0};
    
    // BM25 parameters
    double k1_ = 1.5;
    double b_ = 0.75;
    double avg_doc_length_ = 0.0;
    
    int max_docs_per_segment_ = 100000;
    int segment_size_mb_ = 100;
};

} // namespace crawler
