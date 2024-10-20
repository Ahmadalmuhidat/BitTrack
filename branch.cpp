#include "branch.hpp"

std::string getCurrentBranch()
{
  if (!std::filesystem::exists(".bittrack"))
  {
    std::cout << "Not inside a BitTrack repository!" << std::endl;
    return "";
  }

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
  std::vector<std::string> branches;
  const std::string prefix = ".bittrack/refs/heads/";

  for (const auto &entry : std::filesystem::recursive_directory_iterator(".bittrack/refs/heads"))
  {
    if (entry.is_regular_file())
    {
      std::string branch = entry.path().string();
      branches.push_back(branch.substr(prefix.length()));
    }
  }
  return branches;
}

void printBranshesList()
{
  for (const auto &entry : std::filesystem::recursive_directory_iterator(".bittrack/refs/heads"))
  {
    if (entry.is_regular_file())
    {
      std::string filePath = entry.path().string();
      const std::string prefix = ".bittrack/refs/heads/";

      if (filePath.find(prefix) == 0)
      {
        std::string head = getCurrentBranch();
        std::string branch = filePath.substr(prefix.length());

        if (branch == head)
        {
          std::cout << "\033[32m" << filePath.substr(prefix.length()) << "\033[0m" << std::endl;
        }
        else
        {
          std::cout << filePath.substr(prefix.length()) << std::endl;
        }
      }
    }
  }
}

void addBranch(std::string name)
{
  std::vector<std::string> branches = getBranchesList();

  if (std::find(branches.begin(), branches.end(), name) == branches.end())
  {
    std::ofstream NewBranch(".bittrack/refs/heads/" + name);
    NewBranch.close();

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
    std::cout << "create the branch first" << std::endl;
  }
  else if (getCurrentBranch() == name)
  {
    std::cout << name << " is already the main branch" << std::endl;
  }
  else
  {
    std::ofstream HeadFile(".bittrack/HEAD", std::ios::trunc);
    HeadFile << name << std::endl;

    std::cout << "switch to branch " << name  << std::endl;
  }

}