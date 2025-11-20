#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include <nlohmann/json.hpp>
#include <openssl/sha.h>

#include "Log.h"
#include "MiniGit.h"
#include "Repository.h"

void Repository::init()
{
    bool repo_initialized = initialized();

    if(!repo_initialized)
    {
        try 
        {
            std::vector<std::string> directories { MINIGIT_FILES_PATH, 
                                                    MINIGIT_REFS_PATH,
                                                    MINIGIT_BRANCHES_PATH,
                                                    MINIGIT_OBJECTS_PATH,
                                                    MINIGIT_COMMITS_PATH,
                                                    MINIGIT_BLOBS_PATH,
                                                    MINIGIT_TREES_PATH,
                                                    MINIGIT_LOGS_PATH,
                                                    MINIGIT_LOG_REFS_PATH,
                                                    MINIGIT_BRANCHES_LOG_PATH    
                                                };

            for(auto dir_name : directories)
            {
                if (std::filesystem::create_directory(dir_name)) 
                {
                    std::cout << "Initialized MiniGit repository in: " << dir_name << std::endl;
                } 
                else 
                {
                    std::cout << "Error: Directory " << dir_name << " already exists or failed to create.\n";
                }
            }
        } 
        catch (const std::filesystem::filesystem_error& e) 
        {
            std::cerr << "Error: " << e.what() << '\n';
        }

        // We are now on branch master, so write this information into HEAD
        std::ofstream head(MINIGIT_HEAD_PATH);
        head << MINIGIT_MASTER_BRANCH_NAME;
        head.close();

    }
    else
    {
        std::cout << "Repository already initialized." << std::endl;
    }
}

void Repository::add(const std::vector<std::string>& filenames)
{
    

    bool is_initialized = initialized();

    if(!is_initialized)
    {
        std::cout << "Error: Repository not initialized." << std::endl;
    }
    else
    {
        // First load the existing index, then update any entries if applicable

        std::unordered_map<std::string, std::string> tracked_files;
        load_tracked_files(tracked_files);

        for(auto filename : filenames)
        {
            if (std::filesystem::exists(filename))
            {
                // First try to see if this file is already in the index, if so check if hash has changed
                auto search = tracked_files.find(filename);
                
                // If the file is not in the index or is in the index but the hash has changed 
                // add the file to the index and copy the file
                std::string current_hash = get_file_hash(filename);
                if((search == tracked_files.end()) || 
                        ((search != tracked_files.end()) && (search->second != current_hash)))
                {
                    tracked_files[filename] = current_hash;
                    // Save blob for files that are staged, since this is the version that should be commited even
                    // the file is modified before the next commit.

                    std::filesystem::copy_file(filename, MINIGIT_BLOBS_PATH + current_hash);  
                    std::cout << "Added " << filename << std::endl;                
                }             
            }
            else
            {
                std::cout << "ERROR: file " << filename << " did not match any files." << std::endl;
            }
            
        }

        write_tracked_files(tracked_files);
    }
}


void Repository::commit(const std::string& message)
{
    // TODO: Read author name from config file

    bool is_initialized = initialized();
    if(!is_initialized)
    {
        std::cout << "Error: Repository not initialized." << std::endl;
    }
    else
    {
        // Assemble commit info and log entry
        CommitInfo commit;
        LogEntry log_entry;
        
        load_tracked_files(commit.file_hashes);               
        commit.author = "Author";
        log_entry.author = commit.author;
        auto now = std::chrono::system_clock::now();
        commit.timestamp = timepointToString(now);
        log_entry.timestamp = commit.timestamp;
        commit.message = message;    
        log_entry.message = commit.message;    
        commit.id = sha1(commit.author + commit.timestamp + commit.message);
        log_entry.new_commit_id = commit.id;
        // Retrieve parent commit info
        CommitInfo parent_commit_info;
        get_previous_commit_info(parent_commit_info);
        commit.parent_1_id = parent_commit_info.id;
        log_entry.old_commit_id = parent_commit_info.id;


        std::cout << "Files to be commited: " << std::endl;
        // List only the files that are in the index but are not in the previous commit or the hash has changed.
        // Under the hood all staged files hashes are saved in the commit info JSON file. 
      
        for(auto const& pair : commit.file_hashes)
        {
            auto search = parent_commit_info.file_hashes.find(pair.first);
            if(search == parent_commit_info.file_hashes.end() ||
                    (search != parent_commit_info.file_hashes.end() && 
                        search->second != pair.second))
            {
                std::cout << "\t" << pair.first << std::endl;
            }  
        }

        // Write commit ID in corresponding branch file
        std::ofstream branch_file(MINIGIT_BRANCHES_PATH + get_current_branch());
        branch_file << commit.id;
        branch_file.close();

        // Write JSON file containing commit info 
        write_commit_info(commit);

        // log commit both in logs/HEAD and in logs/refs/heads/<branch_id>
        write_log_entry(MINIGIT_HEAD_LOG_PATH, log_entry);
        write_log_entry(MINIGIT_BRANCHES_LOG_PATH + get_current_branch(), log_entry);
    }
}

void Repository::revert(const std::string& commit_id)
{
    bool is_initialized = initialized();
    if(!is_initialized)
    {
        std::cout << "Error: Repository not initialized." << std::endl;
    }
    else
    {
        // Assemble commit info and log entry
        CommitInfo commit;
        LogEntry log_entry;
                    
        commit.author = "Author";
        log_entry.author = commit.author;
        auto now = std::chrono::system_clock::now();
        commit.timestamp = timepointToString(now);
        log_entry.timestamp = commit.timestamp;
        commit.message = "Reverting to " + commit_id;    
        log_entry.message = commit.message;    
        commit.id = sha1(commit.author + commit.timestamp + commit.message);
        log_entry.new_commit_id = commit.id;
        // Retrieve parent commit info
        CommitInfo parent_commit_info;
        get_previous_commit_info(parent_commit_info);
        commit.parent_1_id = parent_commit_info.id;
        log_entry.old_commit_id = parent_commit_info.id;

        // First retrieve old commit info 
        CommitInfo old_commit_info;
        load_commit_info(commit_id, old_commit_info);

        // Retrieve file hashes from the old commit
        for(auto const& pair : old_commit_info.file_hashes)
        {
            commit.file_hashes[pair.first] = pair.second;
            
            // If the current hash is different from the old hash, 
            // move blobs associated with old commit id back to working directory

            std::string current_hash = get_file_hash(pair.first);

            if(current_hash != pair.second)
            {
                // remove file from working directory first
                std::filesystem::remove(pair.first);
                // now replace it with old version
                std::filesystem::copy_file(
                    MINIGIT_BLOBS_PATH + pair.second, 
                    pair.first, 
                    std::filesystem::copy_options::overwrite_existing);
            }        
        }

        // Write commit ID in corresponding branch file
        std::ofstream branch_file(MINIGIT_BRANCHES_PATH + get_current_branch());
        branch_file << commit.id;
        branch_file.close();

        // Write JSON file containing commit info 
        write_commit_info(commit);

        // log commit both in logs/HEAD and in logs/refs/heads/<branch_id>
        write_log_entry(MINIGIT_HEAD_LOG_PATH, log_entry);
        write_log_entry(MINIGIT_BRANCHES_LOG_PATH + get_current_branch(), log_entry);
    }   
}

void Repository::print_log() const
{
    std::vector<LogEntry> entries;
    read_log(MINIGIT_BRANCHES_LOG_PATH + get_current_branch(), entries);

    // Print in reverse order (newest entry first)
    while(entries.size())
    {
        auto const& entry = entries.back();
        std::cout <<"commit " << entry.new_commit_id << std::endl;
        std::cout <<"Author: " << entry.author << std::endl;
        std::cout <<"Date: " << entry.timestamp << std::endl;
        std::cout << std::endl;
        std::cout << entry.message;
        std::cout << std::endl << std::endl;

        //Remove the last element
        entries.pop_back();
    }
}

void Repository::status()
{
    bool is_initialized = initialized();

    if(!is_initialized)
    {
        std::cout << "Error: Repository not initialized." << std::endl;
    }
    else
    {
        std::cout<< "On branch " << get_current_branch() << std::endl;

        std::vector<std::string> working_directory_files;
        load_working_directory_files(working_directory_files);

        std::unordered_map<std::string, std::string> tracked_files;
        bool index_exists = load_tracked_files(tracked_files);

        if(!index_exists)
        {
            std::cout << "Untracked files:" << std::endl;
            for(auto file : working_directory_files)
            {
                std::cout << "\t" << file << std::endl;
            }
        }
        else
        {
            
            std::vector<std::string> staged;
            std::vector<std::string> modified;
            std::vector<std::string> untracked;
            CommitInfo head;
            get_previous_commit_info(head);

            for(auto file : working_directory_files)
            {
                // A file that is in the working directory but not in the index is untracked.
                // A file that is in the index but has a different hash than current hash is modified.
                // A file that is in the index but not in the HEAD 
                //  (or has a different hash in the index than in the HEAD, or if HEAD has not been commited yet) is staged.

                if(auto search = tracked_files.find(file); search != tracked_files.end())
                {
                    std::string current_hash = get_file_hash(file);
                    if(search->second != current_hash)
                    {
                        modified.push_back(file);
                    }

                    if(auto search_head = head.file_hashes.find(file); search_head != head.file_hashes.end())
                    {
                        if(search_head->second != search->second)
                        {
                            staged.push_back(file);
                        }
                    }
                    else // File not found in HEAD
                    {
                        staged.push_back(file);
                    }
                }
                else // File not found in index
                {
                    untracked.push_back(file);
                }
            }

            if(staged.size())
            {
                std::cout << "Changes to be commited:" << std::endl;
                for(auto file : staged)
                {
                    std::cout << "\t" << file << std::endl;
                }
            }

            if(modified.size())
            {
                std::cout << "Changes not staged for commit:" << std::endl;
                for(auto file : modified)
                {
                    std::cout << "\t" << file << std::endl;
                }
            }

            if(untracked.size())
            {
                std::cout << "Untracked files:" << std::endl;
                for(auto file : untracked)
                {
                    std::cout << "\t" << file << std::endl;
                }
            }
        }
    }    
}

bool Repository::initialized() const
{
    const std::filesystem::path files_path {MINIGIT_FILES_PATH};

    return std::filesystem::exists(files_path);
}

void Repository::load_working_directory_files(std::vector<std::string>& working_directory_files) const
{
    for(auto const& dir_entry : std::filesystem::directory_iterator {".\\"})
    {
        if(dir_entry.is_regular_file())
        {
            working_directory_files.push_back(dir_entry.path().filename().string());
        }     
    }
}

bool Repository::load_commit_info(std::string id, CommitInfo& head) const
{
    nlohmann::json json_data;
    bool head_exists = std::filesystem::exists(MINIGIT_COMMITS_PATH + id);
    
    if(head_exists)
    {
        std::ifstream file(MINIGIT_COMMITS_PATH + id);
        file >> json_data;
        head = json_data.get<CommitInfo>();
    }
    
    return head_exists;
}

void Repository::write_commit_info(const CommitInfo& head) const
{
    nlohmann::json json_data;
    json_data = head;
    std::ofstream(MINIGIT_COMMITS_PATH + head.id) << json_data.dump(4);
}

bool Repository::load_tracked_files(std::unordered_map<std::string, std::string>& tracked_files) const
{ 
    bool index_exists = std::filesystem::exists(MINIGIT_INDEX_PATH);
    if(index_exists)
    {
        std::ifstream file(MINIGIT_INDEX_PATH);
        nlohmann::json json_data;
        file >> json_data;
        tracked_files = json_data["tracked_files"].get<std::unordered_map<std::string, std::string>>();
    }
    return index_exists;
}

void Repository::write_tracked_files(std::unordered_map<std::string, std::string>& tracked_files) const
{
    nlohmann::json json_data;
    json_data["tracked_files"] = tracked_files;
    std::ofstream file(MINIGIT_INDEX_PATH);
    file << json_data.dump(4); 
    file.close();
}

std::string Repository::sha1(const std::string &input) const 
{
    unsigned char hash[MINIGIT_SHA_DIGEST_LENGTH]; 

    SHA1(reinterpret_cast<const unsigned char*>(input.c_str()),
         input.size(),
         hash);

    // Convert the resulting bytes to hex
    std::ostringstream result;
    result << std::hex << std::setfill('0');

    for (int i = 0; i < MINIGIT_SHA_DIGEST_LENGTH; i++) {
        result << std::setw(2) << static_cast<int>(hash[i]);
    }

    return result.str();
}

std::string Repository::get_current_branch() const 
{
    // Get current branch name from the HEAD file
    std::ifstream head(MINIGIT_HEAD_PATH);  
    std::stringstream buffer;
    buffer << head.rdbuf();
    std::string branch = buffer.str();
    head.close();
    return branch;
}

std::string Repository::get_file_hash(std::string filename) const
{
    std::filesystem::path file {filename};
    std::filesystem::file_time_type timestamp = std::filesystem::last_write_time(file);
    auto size = std::filesystem::file_size(file);

    std::string hash = sha1(filename + std::to_string(timestamp.time_since_epoch().count()) + std::to_string(size));
    return hash;
}

void Repository::get_previous_commit_info(CommitInfo& commit_info) const
{
    // Instead of using the HEAD info to get the current branch, read
    // last commit id from the log, since the branch could have changed 
    // in the meantime.

    std::vector<LogEntry> entries;
    read_log(MINIGIT_HEAD_LOG_PATH, entries);
    if(entries.size() > 0)
    {
        load_commit_info(entries.back().new_commit_id, commit_info);
    }
}