#ifndef _COMMIT_H_
#define _COMMIT_H_

#include <chrono>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <unordered_map>





typedef struct CommitInfo
{
    std::string id;
    std::string author; 
    std::string message;
    std::string timestamp;
    std::string parent_1_id; // current branch
    std::string parent_2_id; // merged branch
    std::unordered_map<std::string, std::string> file_hashes; // filename -> blob hash
} CommitInfo;

void to_json(nlohmann::json& json_data, const CommitInfo& commit);
void from_json(const nlohmann::json& json_data, CommitInfo& commit);
std::string timepointToString(const std::chrono::system_clock::time_point& tp);
std::chrono::system_clock::time_point stringToTimepoint(const std::string& s);


#endif