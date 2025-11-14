#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "MiniGit.h"
#include "Repository.h"

void Repository::init()
{
    bool repo_initialized = initialized();

    if(!repo_initialized)
    {
        try 
        {
            if (std::filesystem::create_directory(MINIGIT_FILES_PATH)) 
            {
                std::cout << "Initialized empty MiniGit repository in: " << MINIGIT_FILES_PATH << std::endl;
            } 
            else 
            {
                std::cout << "Error: Directory " << MINIGIT_FILES_PATH << " already exists or failed to create.\n";
            }
        } 
        catch (const std::filesystem::filesystem_error& e) 
        {
            std::cerr << "Error: " << e.what() << '\n';
        }
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
        for(auto filename : filenames)
        {
            std::cout << "Added " << filename << std::endl;
            //TODO: Add to staging area
        }
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
        std::vector<std::string> working_directory_files;
        load_working_directory_files(working_directory_files);

        std::unordered_map<std::string, std::string> tracked_files;
        bool index_exists = load_tracked_files(tracked_files);

        if(!index_exists)
        {
            std::cout<<"Untracked files:"<<std::endl;
            for(auto file : working_directory_files)
            {
                std::cout << "\t" << file << std::endl;
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

bool Repository::load_head(CommitInfo& head) const
{

    return false;
}

void Repository::write_head(const CommitInfo& head) const
{

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

        std::cout << "Tracked files loaded:\n";
        for (auto const& p : tracked_files)
            std::cout << p.first << " " << p.second;
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