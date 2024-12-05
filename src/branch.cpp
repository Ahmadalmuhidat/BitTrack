#include "../include/branch.hpp"

std::string getCurrentBranch()
{
  std::ifstream headFile(".bittrack/HEAD");

  if (!headFile.is_open())
  {
    std::cerr << "Error: Could not open HEAD file." << std::endl;
    return "";
  }
  std::string line;
  std::getline(headFile, line);
  headFile.close();

  return line;
}

std::vector<std::string> getBranchesList()
{
  std::vector<std::string> branchesList;
  const std::string prefix = ".bittrack/refs/heads/";

  for (const auto &entry : std::filesystem::recursive_directory_iterator(".bittrack/refs/heads"))
  {
    if (entry.is_regular_file())
    {
      std::string branch = entry.path().string();
      branchesList.push_back(branch.substr(prefix.length()));
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

  if (std::find(branches.begin(), branches.end(), name) == branches.end())
  {
    std::ofstream newBranch(".bittrack/refs/heads/" + name);
    newBranch.close();

    std::filesystem::create_directory(".bittrack/objects/" + name);
  }
  else
  {
    std::cout << name << " is already there" << std::endl;
  }
}

void switchBranch(std::string name)
{
  std::vector<std::string> branches = getBranchesList();

  if (std::find(branches.begin(), branches.end(), name) == branches.end())
  {
    std::cerr << "create the branch first" << std::endl;
  }
  else if (getCurrentBranch() == name)
  {
    std::cerr << name << " is already the main branch" << std::endl;
  }
  else
  {
    std::ofstream HeadFile(".bittrack/HEAD", std::ios::trunc);
    HeadFile << name << std::endl;

    std::cout << "current branch " << name  << std::endl;
  }
}