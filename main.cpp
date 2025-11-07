#include <iostream>
#include <string>
#include <vector>

#include "Repository.h"


int main(int argc, char* argv[]) 
{
    if (argc < 2) 
    {
        std::cout << "Usage: mygit <command> [options]\n";
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
            std::cout << "Usage: mygit add <file1> <file2> <file3> ... \n";
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
            std::cout << "Usage: mygit commit -m \"message\"\n";
            return 1;
        }

        std::cout << "Committing changes: \"" << message << "\"\n";
        // TODO: write commit object
    }

    else if (command == "status") 
    {
        std::cout << "On branch main\n";
        std::cout << "Changes to be committed:\n";
        std::cout << "  modified: example.cpp\n";
        // TODO: check file states
    }

    // Unknown command
    else {
        std::cout << "Unknown command: " << command << "\n";
        std::cout << "Available commands: init, add, commit, status\n";
        return 1;
    }

    return 0;
}