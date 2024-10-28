
#include "branch.cpp"
#include "stage.cpp"
#include "commit.cpp"

void init()
{
  if (std::filesystem::exists(".bittrack"))
  {
    std::cout << "Repository already exists!" << std::endl;
    return;
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

void status()
{
  DisplayStagedFiles();
  std::cout << "\n" << std::endl;
  DisplayUnstagedFiles();
}

void stageFile(int argc, const char *argv[], int &i)
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

void unstageFiles(int argc, const char *argv[], int &i)
{
  if (i + 1 < argc)
  {
    std::string fileToRemove = argv[++i];
    unstage(fileToRemove); // make sure it is there
  }
  else
  {
    std::cerr << "Error: --stage requires a file path." << std::endl;
  }
}

void commit()
{
  std::cout << "commit message: ";
  std::string message;
  getline(std::cin, message);

  commitChanges("almuhidat", message);
}

void showStagedFilesHashes()
{
  index();
}

void showCurrentCommit()
{
  std::cout << getCurrentCommit() << std::endl;
}

void showCommitHistory()
{
  commitHistory();
}

void removeCurrentRepo()
{
  if (!std::filesystem::exists(".bittrack"))
  {
    HASH_HPP
    std::cout << "No repository found." << std::endl;
    return;
  }

  std::filesystem::remove_all(".bittrack");
  std::cout << "Repository removed." << std::endl;
}

void branchOperations(int argc, const char *argv[], int &i)
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

void checkout(const char *argv[], int &i)
{
  std::string name = argv[++i];
  switchBranch(name);
}

int main(int argc, const char *argv[])
{
  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];

    if (arg == "init")
    {
      init();
    }
    else if (arg == "--status")
    {
      status();
    }
    else if (arg == "--stage")
    {
      stageFile(argc, argv, i);
    }
    else if (arg == "--unstage")
    {
      unstageFiles(argc, argv, i);
    }
    else if (arg == "--commit")
    {
      commit();
    }
    else if (arg == "--staged-files-hashes")
    {
      showStagedFilesHashes();
    }
    else if (arg == "--current-commit")
    {
      showCurrentCommit();
    }
    else if (arg == "--commit-history")
    {
      commitHistory();
    }
    else if (arg == "--remove-repo")
    {
      removeCurrentRepo();
    }
    else if (arg == "--branch")
    {
      branchOperations(argc, argv, i);
    }
    else if (arg == "--checkout")
    {
      checkout(argv, i);
    }
    else
    {
      std::cerr << "Unknown flag: " << arg << std::endl;
    }
  }

  return 0;
}
