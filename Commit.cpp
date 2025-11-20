#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include "Commit.h"

void to_json(nlohmann::json& json_data, const CommitInfo& commit)
{
    json_data = nlohmann::json {
        {"id",  commit.id},
        {"author",  commit.author},
        {"message", commit.message},  
        {"timestamp",   commit.timestamp},
        {"parent_1_id",   commit.parent_1_id},
        {"parent_2_id",   commit.parent_2_id},
        {"file_hashes",   commit.file_hashes} // unordered_map serializes automatically
    };
}

void from_json(const nlohmann::json& json_data, CommitInfo& commit)
{
    json_data.at("id").get_to(commit.id);
    json_data.at("author").get_to(commit.author);
    json_data.at("message").get_to(commit.message); 
    json_data.at("timestamp").get_to(commit.timestamp);
    json_data.at("parent_1_id").get_to(commit.parent_1_id);
    json_data.at("parent_2_id").get_to(commit.parent_2_id);
    json_data.at("file_hashes").get_to(commit.file_hashes);  // works for unordered_map<string,string>
}

std::string timepointToString(const std::chrono::system_clock::time_point& tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::gmtime(&t); 
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::chrono::system_clock::time_point stringToTimepoint(const std::string& s) {
    std::tm tm = {};
    std::istringstream iss(s);
    iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

    std::time_t t = _mkgmtime(&tm); 
    return std::chrono::system_clock::from_time_t(t);
}

