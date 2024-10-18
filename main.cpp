#include "includes.hpp"
#include "stage.cpp"
#include "commit.cpp"
#include "branch.cpp"

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
        AddToStage(fileToAdd);
      }
      else
      {
        std::cerr << "Error: --stage requires a file path." << std::endl;
      }
    }
    else if (arg == "--unstage")
    {
      std::string fileToRemove = argv[++i];
      RemoveFromStage(fileToRemove);
    }
    else if (arg == "--commit")
    {
      std::cout << "commit message: ";
      std::string message;
      getline(std::cin, message);

      CommitChanges("almuhidat", message);
    }
    else if (arg == "--staged-files-hashes")
    {
      index();
    }
    else if (arg == "--current-commit")
    {
      std::cout << GetCurrentCommit() << std::endl;
    }
    else if (arg == "--commit-history")
    {
      CommitHistory();
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
    else if (arg == "branch")
    {
      listBranches();
    }
    else
    {
      std::cerr << "Unknown flag: " << arg << std::endl;
    }
  }

  return 0;
}
