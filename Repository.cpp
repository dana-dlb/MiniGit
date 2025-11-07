#include <filesystem>
#include <iostream>

#include "MiniGit.h"
#include "Repository.h"

void Repository::init()
{
    std::string dir_name = std::filesystem::current_path().string() + "\\" + MINIGIT_FILES_PATH;
    bool repo_initialized = initialized();

    if(!repo_initialized)
    {
        try 
        {
            if (std::filesystem::create_directory(dir_name)) 
            {
                std::cout << "Initialized empty MiniGit repository in: " << dir_name << std::endl;
            } 
            else 
            {
                std::cout << "Error: Directory " << dir_name << " already exists or failed to create.\n";
            }
        } catch (const std::filesystem::filesystem_error& e) 
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
    for(auto filename : filenames)
    {
        std::cout << "Added " << filename << std::endl;
        //TODO: Add to staging area
    }
}





bool Repository::initialized() const
{
    const std::filesystem::path files_path {MINIGIT_FILES_PATH};

    return std::filesystem::exists(files_path);
}