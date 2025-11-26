#ifndef _LOG_H_
#define _LOG_H_

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

typedef struct LogEntry
{
    std::string old_commit_id;
    std::string new_commit_id;
    std::string author; 
    std::string timestamp;
    std::string message;
    bool merge;
    std::string other_commit_id; // HEAD commit id of the branch being merged into current branch
} LogEntry;


void to_json(nlohmann::json& json_data, const LogEntry& log_entry);
void from_json(const nlohmann::json& json_data, LogEntry& log_entry);
void read_log(std::string log_filename, std::vector<LogEntry>& entries);
void write_log_entry(std::string log_filename, const LogEntry& log_entry);



#endif