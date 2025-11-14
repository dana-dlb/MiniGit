#ifndef _COMMIT_H_
#define _COMMIT_H_

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

class Commit
{
};


#endif