#include <iostream>
#include <string>
#include <vector>

#include "Repository.h"


int main(int argc, char* argv[]) 
{
    if (argc < 2) 
    {
        std::cout << "Usage: minigit <command> [options]\n";
        return 1;
    }

    std::string command = argv[1];
    Repository repository;

    if (command == "init") 
    {
        std::cout << "Initializing MiniGit repository...\n";
        repository.init();
    }

    else if (command == "add") 
    {
        if (argc < 3) 
        {
            std::cout << "Usage: minigit add <file1> <file2> <file3> ... \n";
            return 1;
        }

        std::vector<std::string> filenames;

        for(int i = 2; i < argc; i++)
        {
             filenames.push_back(argv[i]);
        }
        repository.add(filenames);
    }

    else if (command == "commit") 
    {
        std::string message;
        for (int i = 2; i < argc; i++) 
        {
            if ((std::string(argv[i]) == "-m") && (i + 1 < argc)) 
            {
                message = argv[i + 1];
                i++;
            }
        }

        if (message.empty()) 
        {
            std::cout << "Usage: minigit commit -m \"message\"\n";
            return 1;
        }

        std::cout << "Committing changes: \"" << message << "\"\n";
        repository.commit(message);
    }

    else if (command == "status") 
    {
        repository.status();
    }

    else if (command == "log")
    {
        repository.print_log();
    }

    else if (command == "revert")
    {
        if (argc != 3) 
        {
            std::cout << "Usage: minigit revert <commit_id>";
            return 1;
        }
        else
        {
            std::string commit_id = argv[2];
            repository.revert(commit_id);
        } 
    }

    else if (command == "checkout")
    {
        if (argc != 3) 
        {
            std::cout << "Usage: minigit checkout <branch name>";
            return 1;
        }
        else
        {
            std::string branch = argv[2];
            repository.checkout(branch);
        }         
    }

    else 
    {
        std::cout << "Unknown command: " << command << "\n";
        std::cout << "Available commands: init, add, commit, status\n";
        return 1;
    }

    return 0;
}