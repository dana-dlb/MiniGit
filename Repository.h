#ifndef _REPOSITORY_H_
#define _REPOSITORY_H_

#include <string>
#include <vector>

#include "Commit.h"

class Repository
{
    public:
        void init();
        void status();
        void add(const std::vector<std::string>& filenames);
        void remove(const std::vector<std::string>& filenames);
        void commit(const std::string& message);
        void log() const;
        void checkout(const std::string& commit_id);
        void createBranch(const std::string& name);
        void merge(const std::string& branch);

    private:

        bool initialized() const;


        Commit* head;
        std::unordered_map<std::string, Commit*> branches;
        std::string current_branch;
};

#endif