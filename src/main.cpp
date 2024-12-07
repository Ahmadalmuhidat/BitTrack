#include "branch.cpp"
#include "stage.cpp"
#include "commit.cpp"

bool insideBitTrack()
{
  if (!std::filesystem::exists(".bittrack"))
  {
    return false;
  }
  return true;
}

void init()
{
  if (insideBitTrack())
  {
    std::cerr << "Repository already exists!" << std::endl;
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
  std::cout << "staged files:" << std::endl;
  for (std::string fileName: getStagedFiles())
  {
    std::cout << "\033[32m" << fileName << "\033[0m" << std::endl;
  }

  std::cout << "\n" << std::endl;

  std::cout << "unstaged files:" << std::endl;
  for (std::string fileName: getUnstagedFiles())
  {
    std::cout << "\033[31m" << fileName << "\033[0m" << std::endl;
  }
}

void stageFile(int argc, const char *argv[], int &i)
{
  if (i + 1 < argc)
  {
    std::string fileToAdd = argv[++i];
    stage(fileToAdd);
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
    unstage(fileToRemove);
  }
  else
  {
    std::cerr << "Error: --stage requires a file path." << std::endl;
  }
}

void commit()
{
  std::cout << "message: ";
  std::string message;
  getline(std::cin, message);

  commitChanges("almuhidat", message);
}

void showStagedFilesHashes()
{
  getIndexHashes();
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
      if (i + 1 < argc)
      {
        std::string name = argv[++i];
        addBranch(name);
      }
      else
      {
        std::cerr << "branch name missing" << std::endl;
      }
    }
  }
  else
  {
    std::cerr << "sub flag missing" << std::endl;
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
      break;
    }

    if (insideBitTrack())
    {
      if (arg == "--status")
      {
        status();
        break;
      }
      else if (arg == "--stage")
      {
        stageFile(argc, argv, i);
        break;
      }
      else if (arg == "--unstage")
      {
        unstageFiles(argc, argv, i);
        break;
      }
      else if (arg == "--commit")
      {
        commit();
        break;
      }
      else if (arg == "--staged-files-hashes")
      {
        showStagedFilesHashes();
        break;
      }
      else if (arg == "--current-commit")
      {
        showCurrentCommit();
        break;
      }
      else if (arg == "--log")
      {
        commitHistory();
        break;
      }
      else if (arg == "--remove-repo")
      {
        removeCurrentRepo();
        break;
      }
      else if (arg == "--branch")
      {
        branchOperations(argc, argv, i);
        break;
      }
      else if (arg == "--checkout")
      {
        checkout(argv, i);
        // std::cout << "checkout feature will be here soon!" << std::endl;
        break;
      }
      else
      {
        std::cerr << "Unknown flag: " << arg << std::endl;
        break;
      } 
    }
    else
    {
      std::cerr << "Not inside a BitTrack repository!" << std::endl;
    }
  }

  return 0;
}
