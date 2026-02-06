#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace crawler {

class Storage {
public:
    Storage(const std::string& data_dir);
    ~Storage();
    
    // Save document
    bool save_document(uint64_t doc_id, const std::string& url, 
                      const std::string& content, 
                      const std::unordered_map<std::string, std::string>& metadata);
    
    // Load document
    bool load_document(uint64_t doc_id, std::string& content);
    
    // Save checkpoint
    bool save_checkpoint(const std::unordered_map<std::string, std::string>& state);
    
    // Load checkpoint
    bool load_checkpoint(std::unordered_map<std::string, std::string>& state);
    
    // List all documents
    std::vector<uint64_t> list_documents();

private:
    std::string data_dir_;
    std::string get_document_path(uint64_t doc_id);
};

} // namespace crawler
