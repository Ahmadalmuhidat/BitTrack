#include "../include/branch.hpp"

std::string getCurrentBranch()
{
  // check if HEAD file is available
  std::ifstream headFile(".bittrack/HEAD");

  if (!headFile.is_open())
  {
    std::cerr << "Error: Could not open HEAD file." << std::endl;
    return "";
  }

  // read the first line in HEAD file and return it
  std::string line;
  std::getline(headFile, line);
  headFile.close();

  return line;
}

std::vector<std::string> getBranchesList()
{
  std::vector<std::string> branchesList;
  const std::string prefix = ".bittrack/refs/heads/";

  // get the names of the files inside the heads folder and store them in the branchesList array
  for (const auto &entry: std::filesystem::recursive_directory_iterator(".bittrack/refs/heads"))
  {
    if (entry.is_regular_file())
    {
      std::string branch = entry.path().string(); // get the file path
      branchesList.push_back(branch.substr(prefix.length())); // sustract the file name from the path
    }
  }

  return branchesList;
}

void printBranshesList()
{
  std::vector<std::string> branchesList = getBranchesList();
  std::string head = getCurrentBranch();

  for (int i = 0; i < branchesList.size(); i++)
  {
    std::string branch = branchesList[i];

    if (branch == head)
    {
      // if the branch is the current head print it in green color
      std::cout << "\033[32m" << branch << "\033[0m" << std::endl;
    }
    else
    {
      std::cout << branch << std::endl;
    }
  }
}

void addBranch(std::string name)
{
  std::vector<std::string> branches = getBranchesList();

  // check if the branch not exist
  if (std::find(branches.begin(), branches.end(), name) == branches.end())
  {
    // create a file with the new branch name
    std::ofstream newBranch(".bittrack/refs/heads/" + name);
    newBranch.close();

    // create a directory with the new branch name
    std::filesystem::create_directory(".bittrack/objects/" + name);

    std::string currentCommit = getCurrentCommit();
    std::string currentBranch = getCurrentBranch();

    if (currentBranch != "" && currentCommit != "")
    {
      std::string source = ".bittrack/objects/" + currentBranch  + "/" + currentCommit;
      std::string destination = ".bittrack/objects/" + name  + "/" + currentCommit;

      std::filesystem::copy(source, destination, std::filesystem::copy_options::recursive);
    }

    // insert in the head file the random hash as last commit
    std::ofstream headFile(".bittrack/refs/heads/" + name, std::ios::trunc);
    headFile << currentCommit << std::endl;
    headFile.close();
  }
  else
  {
    std::cout << name << " is already there" << std::endl;
  }
}

void switchBranch(std::string name)
{
  std::vector<std::string> branches = getBranchesList();

  // check if the branch exist
  if (std::find(branches.begin(), branches.end(), name) == branches.end())
  {
    std::cout << "you must create the branch first" << std::endl;
  }
  else if (getCurrentBranch() == name) // check if it is not already the head branch
  {
    std::cout << "you are already in " << name << std::endl;
  }
  else
  {
    // clear the HEAD file content and insert the new head branch
    std::ofstream HeadFile(".bittrack/HEAD", std::ios::trunc);
    HeadFile << name << std::endl;
  }
}