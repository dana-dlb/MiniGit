#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stack>
#include <unordered_map>

#include <nlohmann/json.hpp>
#include <openssl/sha.h>

#include "Log.h"
#include "MiniGit.h"
#include "Repository.h"

void Repository::init()
// Initializes the .minigit repository and subdirectories
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
// Adds filenames to the staging area. 
// The repository must be initialized.
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

                    std::filesystem::copy_file(filename, 
                        MINIGIT_BLOBS_PATH + current_hash, 
                        std::filesystem::copy_options::none);

                    // Make sure to copy timestamp as well, otherwise the hash will differ 
                    auto timestamp = std::filesystem::last_write_time(filename);
                    std::filesystem::last_write_time(MINIGIT_BLOBS_PATH + current_hash,  timestamp);                    
                    
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
// Save a snapshot of the files in the staging area. The commit data is saved in the log.
// Repository must be initialized.
{
    bool is_initialized = initialized();
    if(!is_initialized)
    {
        std::cout << "Error: Repository not initialized." << std::endl;
    }
    else
    {
        // Only commit if there is something staged
        std::vector<std::string> staged;
        std::vector<std::string> modified;
        std::vector<std::string> untracked;

        get_working_directory_files_statuses(staged, modified, untracked);

        if(!staged.size())
        {
            std::cout << "Nothing to commit." << std::endl;
        }
        else // There are files to be commited
        {
            // Assemble commit info and log entry
            CommitInfo commit;
            LogEntry log_entry;
            
            load_tracked_files(commit.file_hashes);
            // TODO: Read author name from config file               
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

            // Write commit ID in corresponding branch file
            std::ofstream branch_file(MINIGIT_BRANCHES_PATH + get_current_branch());
            branch_file << commit.id;
            branch_file.close();

            // Write JSON file containing commit info 
            write_commit_info(commit);

            // log commit both in logs/HEAD and in logs/refs/heads/<branch_id>
            write_log_entry(MINIGIT_HEAD_LOG_PATH, log_entry);
            write_log_entry(MINIGIT_BRANCHES_LOG_PATH + get_current_branch(), log_entry);

            std::cout << "Committed: " << std::endl;
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
        }
    }
}

void Repository::revert(const std::string& commit_id)
// Revert to an old commit id. Files in the working directory are replaced with the versions
// associated with the commit id. The history is kept intact and a new commit is generated
// and logged for this change.
// Repository must be initialized.
// Revert not allowed if there are staged or unmodified changes.
// Revert only allowed with a commit id from the history of the current branch.
{
    bool is_initialized = initialized();
    if(!is_initialized)
    {
        std::cout << "Error: Repository not initialized." << std::endl;
    }
    else
    {
        // Block checkout if there are any staged or unstaged modified files 
        std::vector<std::string> staged;
        std::vector<std::string> modified;
        std::vector<std::string> untracked;

        get_working_directory_files_statuses(staged, modified, untracked);

        if(staged.size() || modified.size())
        {
            std::cout << "ERROR: Cannot revert while there are modified or staged (uncommitted) files." << std::endl;
            
            if(staged.size())
            {
                std::cout << "Changes to be committed:" << std::endl;
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
        }
        else // There are no staged or modified files
        {

            if(!is_revert_commit_id_valid(commit_id))
            {
                std::cout << "ERROR: commit id is not valid for this branch." << std::endl; 
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
                            std::filesystem::copy_options::none);

                        // Make sure to copy timestamp as well, otherwise the hash will differ
                        auto timestamp = std::filesystem::last_write_time(MINIGIT_BLOBS_PATH + pair.second);
                        std::filesystem::last_write_time(pair.first,  timestamp);
                        
                    }        
                }

                // Write index file to match old commit info file hashes
                write_tracked_files(old_commit_info.file_hashes);

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
    }   
}

void Repository::print_log() const
// Print log information for the current branch (list of commits) in reverse chronological order. 
// Repository must be initialized.
{
    bool is_initialized = initialized();
    if(!is_initialized)
    {
        std::cout << "Error: Repository not initialized." << std::endl;
    }
    else
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
}

void Repository::create_branch(const std::string& branch)
// Creates a new branch but does not switch to it.  
// Precondition: There is at least a commit on the current branch
{
    // First check if repository is initialized
    bool is_initialized = initialized();
    if(!is_initialized)
    {
        std::cout << "Error: Repository not initialized." << std::endl;
    }
    else
    { 
        // Can only create a new branch if there is at least a commit on the current branch

        if(!std::filesystem::exists(MINIGIT_BRANCHES_PATH + get_current_branch()))
        {
            std::cout << "ERROR: Cannot create new branch since there are no commits on the current branch: " 
                << get_current_branch() 
                << std::endl;
        }
        else
        {
            // Copy head commit id to the branch head file
            std::ifstream head(MINIGIT_BRANCHES_PATH + get_current_branch());  
            std::stringstream buffer;
            buffer << head.rdbuf();
            std::string commit_id = buffer.str();
            head.close();
            std::ofstream branch_file(MINIGIT_BRANCHES_PATH + branch);
            branch_file << commit_id;
            branch_file.close();        

            // Copy last log entry for the current branch to the new branch log file
            std::vector<LogEntry> entries;
            read_log(MINIGIT_BRANCHES_LOG_PATH + get_current_branch(), entries);   
            write_log_entry(MINIGIT_BRANCHES_LOG_PATH + branch, entries.back());
        }
    }
}

void Repository::checkout(const std::string& branch)
// Checkout a branch (the index is reset to the last commit of the new branch, so is the working directory)
// Preconditions: - repository is initialized
//                - branch must exist
//                - there are no staged or modified files
{
    // First check if repository is initialized
    bool is_initialized = initialized();
    if(!is_initialized)
    {
        std::cout << "Error: Repository not initialized." << std::endl;
    }
    else
    { 
        // Check if the branch exists
        const std::filesystem::path branch_path {MINIGIT_BRANCHES_PATH + branch};
        if(!std::filesystem::exists(branch_path))
        {
            std::cout << "ERROR: Branch does not exist." << std::endl;
        }
        else // branch exists
        {
            // Block checkout if there are any staged or unstaged modified files 
            std::vector<std::string> staged;
            std::vector<std::string> modified;
            std::vector<std::string> untracked;

            get_working_directory_files_statuses(staged, modified, untracked);

            if(staged.size() || modified.size())
            {
                std::cout << "ERROR: Cannot checkout another branch while there are modified or staged (uncommitted) files." << std::endl;
                
                if(staged.size())
                {
                    std::cout << "Changes to be committed:" << std::endl;
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
            }
            else // Preconditions are met, branch can be checked out
            {
                // Retrieve old HEAD id 
                std::ifstream branch_head(MINIGIT_BRANCHES_PATH + get_current_branch());  
                std::stringstream buffer;
                buffer << branch_head.rdbuf();
                std::string old_commit_id = buffer.str();
                branch_head.close();

                // Point HEAD to the new branch
                std::ofstream head(MINIGIT_HEAD_PATH);
                head << branch;
                head.close();            

                // Reset the index to the latest commit of the new branch
                CommitInfo commit_info;
                get_previous_commit_info(commit_info);
                write_tracked_files(commit_info.file_hashes);

                // Replace the working directory to the latest commit of the new branch
                
                for(auto const& pair : commit_info.file_hashes)
                {
                    // remove file from working directory first, if it exists
                    if(std::filesystem::exists(pair.first))
                    {
                        std::filesystem::remove(pair.first);
                    }
                   
                    // now replace it with latest version in the new branch
                    std::filesystem::copy_file(
                        MINIGIT_BLOBS_PATH + pair.second, 
                        pair.first, 
                        std::filesystem::copy_options::none);

                    // Make sure to copy timestamp as well, otherwise the hash will differ
                    auto timestamp = std::filesystem::last_write_time(MINIGIT_BLOBS_PATH + pair.second);
                    std::filesystem::last_write_time(pair.first, timestamp);
                }

                // Now log this HEAD change in the HEAD log
                LogEntry log_entry;
                // TODO: Read author name from config file               
                log_entry.author = "Author";
                auto now = std::chrono::system_clock::now();
                log_entry.timestamp = timepointToString(now);
                log_entry.message = "Switched to branch " + branch;    
                log_entry.new_commit_id = commit_info.id;
                log_entry.old_commit_id = old_commit_id;

                // log commit both in logs/HEAD and in logs/refs/heads/<branch_id>
                write_log_entry(MINIGIT_HEAD_LOG_PATH, log_entry);

            }         
        }
    }
}

void Repository::print_branches()
// Prints the list of existing branches
{
    bool is_initialized = initialized();

    if(!is_initialized)
    {
        std::cout << "Error: Repository not initialized." << std::endl;
    }
    else
    {
      for(auto const& dir_entry : std::filesystem::directory_iterator {MINIGIT_BRANCHES_PATH})
        {
            if(dir_entry.is_regular_file())
            {
                std::cout << dir_entry.path().filename().string() << std::endl;
            }     
        } 
    } 
}

void Repository::merge(const std::string& branch)
// Merges the branch into the current branch
{
    bool is_initialized = initialized();

    if(!is_initialized)
    {
        std::cout << "Error: Repository not initialized." << std::endl;
    }
    else
    {
        // Check if branch name is valid
        bool branch_valid = false;

        for(auto const& dir_entry : std::filesystem::directory_iterator {MINIGIT_BRANCHES_PATH})
        {
            if(dir_entry.is_regular_file() && dir_entry.path().filename().string() == branch )
            {
                branch_valid = true;
                break;
            }     
        } 
        if(!branch_valid)
        {
            std::cout << "ERROR: no such branch: " + branch << std::endl;
        }
        else
        {
            // Check if there are staged or modified files
            std::vector<std::string> staged;
            std::vector<std::string> modified;
            std::vector<std::string> untracked;

            get_working_directory_files_statuses(staged, modified, untracked);

            if(staged.size() || modified.size())
            {
                std::cout << "ERROR: Cannot merge in branch while there are modified or staged (uncommitted) files." << std::endl;
                
                if(staged.size())
                {
                    std::cout << "Changes to be committed:" << std::endl;
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
            }
            else // All preconditions are met, proceed with merge
            {
                // First find the common ancestor
                bool ancestor_found = false;
                std::vector<LogEntry> entries_branch_1;
                read_log(MINIGIT_BRANCHES_LOG_PATH + get_current_branch(), entries_branch_1);
                std::string last_commit_branch_1  = entries_branch_1.back().new_commit_id;
                std::vector<LogEntry> entries_branch_2;
                read_log(MINIGIT_BRANCHES_LOG_PATH + branch, entries_branch_2);
                std::string last_commit_branch_2  = entries_branch_2.back().new_commit_id;
                std::string ancestor_id;
                std::stack<LogEntry> new_commits;

                while(!ancestor_found && entries_branch_2.size())
                {
                    auto entry_2 = entries_branch_2.back();
                    entries_branch_2.pop_back();

                    for(int i = entries_branch_1.size(); i > 0; i--)
                    {
                        if(entry_2.new_commit_id == entries_branch_1[i].new_commit_id)
                        {
                            ancestor_found = true;
                            ancestor_id = entry_2.new_commit_id;
                        }
                        else
                        {
                            new_commits.push(entry_2);
                        }
                    }
                }
                if (!ancestor_found)
                {
                    std::cout <<"ERROR: common ancestor not found." << std::endl;
                }
                else if (ancestor_id == last_commit_branch_2)
                {
                    std::cout << "Already up to date." << std::endl;
                }
                else
                {
                    perform_merge(ancestor_id, last_commit_branch_1, last_commit_branch_2);
                }
            }
        }
    } 
}

void Repository::status()
// Prints branch name and the list of staged, modified and untracked files.
// Repository must be initialized.
{
    bool is_initialized = initialized();

    if(!is_initialized)
    {
        std::cout << "Error: Repository not initialized." << std::endl;
    }
    else
    {
        std::cout<< "On branch " << get_current_branch() << std::endl;

        std::vector<std::string> staged;
        std::vector<std::string> modified;
        std::vector<std::string> untracked;

        get_working_directory_files_statuses(staged, modified, untracked);

        if(staged.size())
        {
            std::cout << "Changes to be committed:" << std::endl;
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

bool Repository::initialized() const
// Returns true if the repository has been initialized, false otherwise.
{
    const std::filesystem::path files_path {MINIGIT_FILES_PATH};

    return std::filesystem::exists(files_path);
}

void Repository::load_working_directory_files(std::vector<std::string>& working_directory_files) const
// Load working directory files into working_directory_files.
{
    for(auto const& dir_entry : std::filesystem::directory_iterator {".\\"})
    {
        if(dir_entry.is_regular_file())
        {
            working_directory_files.push_back(dir_entry.path().filename().string());
        }     
    }
}

bool Repository::load_commit_info(std::string id, CommitInfo& commit_info) const
// Load commit information from file.
{
    nlohmann::json json_data;
    bool file_exists = std::filesystem::exists(MINIGIT_COMMITS_PATH + id);
    
    if(file_exists)
    {
        std::ifstream file(MINIGIT_COMMITS_PATH + id);
        file >> json_data;
        commit_info = json_data.get<CommitInfo>();
    }
    
    return file_exists;
}

void Repository::write_commit_info(const CommitInfo& commit_info) const
// Write commit info to file.
{
    nlohmann::json json_data;
    json_data = commit_info;
    std::ofstream(MINIGIT_COMMITS_PATH + commit_info.id) << json_data.dump(4);
}

bool Repository::load_tracked_files(std::unordered_map<std::string, std::string>& tracked_files) const
// Load tracked files from index.
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
// Write tracked files to index (JSON file).
{
    nlohmann::json json_data;
    json_data["tracked_files"] = tracked_files;
    std::ofstream file(MINIGIT_INDEX_PATH);
    file << json_data.dump(4); 
    file.close();
}

std::string Repository::sha1(const std::string &input) const 
// Returns the SHA-1 hashed input string. 
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
// Returns the current branch name.
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
// Returns the hash for a file using its name, last modified timestamp and size.
{
    std::filesystem::path file {filename};
    std::filesystem::file_time_type timestamp = std::filesystem::last_write_time(file);
    auto size = std::filesystem::file_size(file);

    std::string hash = sha1(filename + std::to_string(timestamp.time_since_epoch().count()) + std::to_string(size));
    return hash;
}

void Repository::get_previous_commit_info(CommitInfo& commit_info) const
// Retrieves the last commit information from the log.
{
    // Instead of using the HEAD info to get the current branch, read
    // last commit id from the log, since the branch could have changed 
    // in the meantime.

    std::vector<LogEntry> entries;

    read_log(MINIGIT_BRANCHES_LOG_PATH + get_current_branch(), entries);
    if(entries.size() > 0)
    {
        load_commit_info(entries.back().new_commit_id, commit_info);
    }
}

void Repository::get_working_directory_files_statuses(
    std::vector<std::string>& staged, 
    std::vector<std::string>& modified, 
    std::vector<std::string>& untracked) const
{
    std::vector<std::string> working_directory_files;
    load_working_directory_files(working_directory_files);

    std::unordered_map<std::string, std::string> tracked_files;
    load_tracked_files(tracked_files);

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
}

bool Repository::is_revert_commit_id_valid(std::string commit_id) const
// Checks in the branch log to see if the commit id is found
{
    bool is_valid = false;
    std::vector<LogEntry> entries;

    read_log(MINIGIT_BRANCHES_LOG_PATH + get_current_branch(), entries);
    for(auto const& entry : entries)
    {
        if(entry.new_commit_id == commit_id)
        {
            is_valid = true;
            break;
        } 
    }
    return is_valid;
}

void Repository::perform_merge(const std::string& base_commit_id, 
    const std::string& branch_1_commit_id, 
    const std::string& branch_2_commit_id) const
{
    CommitInfo base_commit_info;
    CommitInfo branch_1_commit_info;
    CommitInfo branch_2_commit_info;
    load_commit_info(base_commit_id, base_commit_info);
    load_commit_info(branch_1_commit_id, branch_1_commit_info);
    load_commit_info(branch_2_commit_id, branch_2_commit_info);

    std::unordered_map<std::string, std::string> merged_content = branch_1_commit_info.file_hashes;

    for(auto const& [filename, hash] : branch_2_commit_info.file_hashes)
    {
        // Check if the file exists in branch 1, otherwise copy the file
        if(auto search_1 = branch_1_commit_info.file_hashes.find(filename); 
                search_1 == branch_1_commit_info.file_hashes.end())
        {
            // File not found in branch 1, so add it to the merged content
            merged_content[filename] = hash;
        }
        else // File is found in both branches
        {
            if(auto search_base = base_commit_info.file_hashes.find(filename); 
                search_base == base_commit_info.file_hashes.end())            
            {
                // File found in both branches, but not in base, so mark conflict
                // TODO: mark conflict or try to automerge? 
            }
            else // File found in base as well
            {
                if(hash != search_1->second) 
                {
                    // Hashes are different in the two branches, so check if any of them matches base
                    if(search_1->second == base_commit_info.file_hashes[filename]) 
                    {
                        // Branch 1 hash matches base, so take the branch 2 version in merged_content
                        merged_content[filename] = hash; 
                    }
                    else if (hash == base_commit_info.file_hashes[filename])
                    {
                        // Branch 2 hash matches base, so take the branch 1 version in merged_content
                        merged_content[filename] = hash; 
                    }
                    else
                    {
                        // File has been changed in both branches, so try line by line merge of file
                        // TODO: line by line 3-way merge
                    }
                }
            }
        }
    }   
}