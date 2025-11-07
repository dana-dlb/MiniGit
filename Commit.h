#ifndef _COMMIT_H_
#define _COMMIT_H_

#include <string>
#include <unordered_map>

class Commit
{
    public:
        Commit(std::string author, 
            std::string message, 
            Commit* parent_1, 
            Commit* parent_2, 
            std::unordered_map<std::string, std::string> file_hashes);


    private:
        std::string id;
        std::string author; 
        std::string message;
        std::string timestamp;
        Commit* parent_1; // current branch
        Commit* parent_2; // merged branch
        std::unordered_map<std::string, std::string> file_hashes; // filename -> blob hash
};


#endif