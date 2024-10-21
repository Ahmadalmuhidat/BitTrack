
#include "includes.hpp"
#include "branch.cpp"
#include "stage.cpp"
#include "commit.cpp"

int main(int argc, const char *argv[])
{
  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];

    if (arg == "init")
    {
      if (std::filesystem::exists(".bittrack"))
      {
        std::cout << "Repository already exists!" << std::endl;
        return 0;
      }

      std::filesystem::create_directories(".bittrack/objects");
      std::filesystem::create_directories(".bittrack/commits");
      std::filesystem::create_directories(".bittrack/refs/heads");

      std::ofstream HeadFile(".bittrack/HEAD");
      HeadFile.close();

      std::ofstream HistoryFile(".bittrack/commits/history");
      HistoryFile.close();

      addBranch("master");
      switchBranch("master");

      std::cout << "Initialized empty BitTrack repository." << std::endl;
    }
    else if (arg == "--status")
    {
      DisplayStagedFiles();
      std::cout << "\n" << std::endl;
      DisplayUnstagedFiles();
    }
    else if (arg == "--stage")
    {
      if (i + 1 < argc)
      {
        std::string fileToAdd = argv[++i];
        stage(fileToAdd); // make sure it is there
      }
      else
      {
        std::cerr << "Error: --stage requires a file path." << std::endl;
      }
    }
    else if (arg == "--unstage")
    {
      std::string fileToRemove = argv[++i];
      unstage(fileToRemove); // make sure it is there
    }
    else if (arg == "--commit")
    {
      std::cout << "commit message: ";
      std::string message;
      getline(std::cin, message);

      commitChanges("almuhidat", message);
    }
    else if (arg == "--staged-files-hashes")
    {
      index();
    }
    else if (arg == "--current-commit")
    {
      std::cout << getCurrentCommit() << std::endl;
    }
    else if (arg == "--commit-history")
    {
      commitHistory();
    }
    else if (arg == "--remove-repo")
    {
      if (!std::filesystem::exists(".bittrack"))
      {
        std::cout << "No repository found." << std::endl;
        return 0;
      }

      std::filesystem::remove_all(".bittrack");
      std::cout << "Repository removed." << std::endl;
    }
    else if (arg == "--branch")
    {
      if (i + 1 < argc)
      {
        std::string subFlag = argv[++i];

        if (subFlag == "-l")
        {
          printBranshesList();
        }
        else if (subFlag == "-c")
        {
          std::string name = argv[++i];
          addBranch(name);
        }
      }
      else
      {
        std::cout << "sub flag missing" << std::endl;
      }
    }
    else if (arg == "--checkout")
    {
      std::string name = argv[2];
      switchBranch(name);
    }
    else
    {
      std::cerr << "Unknown flag: " << arg << std::endl;
    }
  }

  return 0;
}
