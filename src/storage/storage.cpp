#include "storage.h"
#include <fstream>
#include <filesystem>
#include <sstream>

namespace crawler {

Storage::Storage(const std::string& data_dir) : data_dir_(data_dir) {
    std::filesystem::create_directories(data_dir);
    std::filesystem::create_directories(data_dir + "/docs");
    std::filesystem::create_directories(data_dir + "/checkpoints");
}

Storage::~Storage() = default;

std::string Storage::get_document_path(uint64_t doc_id) {
    return data_dir_ + "/docs/" + std::to_string(doc_id) + ".doc";
}

bool Storage::save_document(uint64_t doc_id, const std::string& url,
                           const std::string& content,
                           const std::unordered_map<std::string, std::string>& metadata) {
    std::string path = get_document_path(doc_id);
    std::ofstream out(path);
    if (!out.is_open()) {
        return false;
    }
    
    // Simple format: URL\nMETADATA\nCONTENT
    out << url << "\n";
    for (const auto& [key, value] : metadata) {
        out << key << ":" << value << "\n";
    }
    out << "---\n";
    out << content;
    
    return true;
}

bool Storage::load_document(uint64_t doc_id, std::string& content) {
    std::string path = get_document_path(doc_id);
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }
    
    std::string line;
    bool in_content = false;
    while (std::getline(in, line)) {
        if (line == "---") {
            in_content = true;
            continue;
        }
        if (in_content) {
            content += line + "\n";
        }
    }
    
    return true;
}

bool Storage::save_checkpoint(const std::unordered_map<std::string, std::string>& state) {
    std::string path = data_dir_ + "/checkpoints/latest.ckpt";
    std::ofstream out(path);
    if (!out.is_open()) {
        return false;
    }
    
    for (const auto& [key, value] : state) {
        out << key << "=" << value << "\n";
    }
    
    return true;
}

bool Storage::load_checkpoint(std::unordered_map<std::string, std::string>& state) {
    std::string path = data_dir_ + "/checkpoints/latest.ckpt";
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(in, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            state[key] = value;
        }
    }
    
    return true;
}

std::vector<uint64_t> Storage::list_documents() {
    std::vector<uint64_t> doc_ids;
    std::string docs_dir = data_dir_ + "/docs";
    
    if (!std::filesystem::exists(docs_dir)) {
        return doc_ids;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(docs_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".doc") {
            std::string filename = entry.path().stem().string();
            try {
                uint64_t doc_id = std::stoull(filename);
                doc_ids.push_back(doc_id);
            } catch (...) {
                // Skip invalid filenames
            }
        }
    }
    
    return doc_ids;
}

} // namespace crawler
