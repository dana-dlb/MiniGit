#ifndef _REPOSITORY_H_
#define _REPOSITORY_H_

#include <string>
#include <vector>
#include <unordered_map>
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
        void load_working_directory_files(std::vector<std::string>& working_directory_files) const;
        bool load_head(CommitInfo& head) const;
        void write_head(const CommitInfo& head) const;
        bool load_tracked_files(std::unordered_map<std::string, std::string>& tracked_files) const;
        void write_tracked_files(std::unordered_map<std::string, std::string>& tracked_files) const;

        Commit* head;
        std::unordered_map<std::string, Commit*> branches;
        std::string current_branch;
};

#endif