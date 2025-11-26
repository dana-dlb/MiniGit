#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "Log.h"

void to_json(nlohmann::json& json_data, const LogEntry& log_entry)
{
    json_data = nlohmann::json {
        {"old_commit_id",   log_entry.old_commit_id},
        {"new_commit_id",   log_entry.new_commit_id},
        {"author",          log_entry.author},  
        {"timestamp",       log_entry.timestamp},
        {"message",         log_entry.message},
        {"merge",           log_entry.merge},
        {"other_commit_id", log_entry.other_commit_id}      
    };
}

void from_json(const nlohmann::json& json_data, LogEntry& log_entry)
{
    json_data.at("old_commit_id").get_to(log_entry.old_commit_id);
    json_data.at("new_commit_id").get_to(log_entry.new_commit_id);
    json_data.at("author").get_to(log_entry.author); 
    json_data.at("timestamp").get_to(log_entry.timestamp);
    json_data.at("message").get_to(log_entry.message);
    json_data.at("merge").get_to(log_entry.merge);
    json_data.at("other_commit_id").get_to(log_entry.other_commit_id);
}

void read_log(std::string log_filename, std::vector<LogEntry>& entries)
{
    bool index_exists = std::filesystem::exists(log_filename);
    if(index_exists)
    {
        std::ifstream file(log_filename);
        nlohmann::json json_data;
        file >> json_data;
        entries = json_data["log"].get<std::vector<LogEntry>>();
    }
}

void write_log_entry(std::string log_filename, const LogEntry& log_entry)
{
    // Read log file and append newest entry
    std::vector<LogEntry> entries;
    read_log(log_filename, entries);
    entries.push_back(log_entry);
    
    // Save file 
    nlohmann::json json_data;
    json_data["log"] = entries;
    std::ofstream file(log_filename);
    file << json_data.dump(4); 
    file.close();
}
