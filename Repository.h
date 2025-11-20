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
        void revert(const std::string& commit_id);
        void print_log() const;
        void checkout(const std::string& commit_id);
        void create_branch(const std::string& name);
        void merge(const std::string& branch);

    private:

        bool initialized() const;
        void load_working_directory_files(std::vector<std::string>& working_directory_files) const;
        bool load_commit_info(std::string id, CommitInfo& head) const;
        void write_commit_info(const CommitInfo& head) const;
        bool load_tracked_files(std::unordered_map<std::string, std::string>& tracked_files) const;
        void write_tracked_files(std::unordered_map<std::string, std::string>& tracked_files) const;
        std::string sha1(const std::string &input) const;
        std::string get_current_branch() const;
        std::string get_file_hash(std::string filename) const;
        void get_previous_commit_info(CommitInfo& commit_info) const;

        std::unordered_map<std::string, CommitInfo> branches;
};

#endif