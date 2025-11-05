#include <iostream>
#include <string>
#include <vector>

using namespace std;


int main(int argc, char* argv[]) 
{
    if (argc < 2) 
    {
        cout << "Usage: mygit <command> [options]\n";
        return 1;
    }

    string command = argv[1];

    // Handle "init"
    if (command == "init") 
    {
        cout << "Initializing repository...\n";
        // TODO: create .mygit folder, etc.
    }

    // Handle "add"
    else if (command == "add") 
    {
        if (argc < 3) 
        {
            cout << "Usage: mygit add <file>\n";
            return 1;
        }
        string filename = argv[2];
        cout << "Adding file: " << filename << "\n";
        // TODO: stage file changes
    }

    // Handle "commit"
    else if (command == "commit") 
    {
        string message;
        for (int i = 2; i < argc; i++) 
        {
            if ((string(argv[i]) == "-m") && (i + 1 < argc)) 
            {
                message = argv[i + 1];
                i++;
            }
        }

        if (message.empty()) {
            cout << "Usage: mygit commit -m \"message\"\n";
            return 1;
        }

        cout << "Committing changes: \"" << message << "\"\n";
        // TODO: write commit object
    }

    // Handle "status"
    else if (command == "status") 
    {
        cout << "On branch main\n";
        cout << "Changes to be committed:\n";
        cout << "  modified: example.cpp\n";
        // TODO: check file states
    }

    // Unknown command
    else {
        cout << "Unknown command: " << command << "\n";
        cout << "Available commands: init, add, commit, status\n";
        return 1;
    }

    return 0;
}